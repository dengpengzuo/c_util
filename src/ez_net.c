#include "ez_net.h"

#include "ez_bytebuf.h"
#include "ez_log.h"
#include "ez_string.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static const char* const SFNA[] = { "", "AF_UNIX", "AF_INET", "AF_INET6" };
static const char* const STNA[] = { "", "SOCK_STREAM", "SOCK_DGRAM", "SOCK_RAW" };
static const char* const SPNA[] = { "", "IPPROTO_IP", "IPPROTO_TCP", "IPPROTO_UDP", "IPPROTO_IPV6", "IPPROTO_RAW" };

const char* socket_family_name(int sf)
{
    if (AF_UNIX == sf)
        return SFNA[1];
    else if (AF_INET == sf)
        return SFNA[2];
    else if (AF_INET6 == sf)
        return SFNA[3];
    else
        return SFNA[0];
}

const char* socket_socktype_name(int st)
{
    if (SOCK_STREAM == st)
        return STNA[1];
    else if (SOCK_DGRAM == st)
        return STNA[2];
    else if (SOCK_RAW == st)
        return STNA[3];
    else
        return STNA[0];
}

const char* socket_protocol_name(int sp)
{
    if (IPPROTO_IP == sp)
        return SPNA[1];
    else if (IPPROTO_TCP == sp)
        return SPNA[2];
    else if (IPPROTO_UDP == sp)
        return SPNA[3];
    else if (IPPROTO_IPV6 == sp)
        return SPNA[4];
    else if (IPPROTO_RAW == sp)
        return SPNA[5];
    else
        return SPNA[0];
}

/* create server */
static int ez_net_bind(int s, struct sockaddr* sa, socklen_t len)
{
    if (bind(s, sa, len) == -1) {
        log_error("%d > bind: %s", s, strerror(errno));
        return ANET_ERR;
    }

    return ANET_OK;
}

static int ez_net_listen(int s, int backlog)
{
    if (listen(s, backlog) == -1) {
        log_error("%d > listen: %s", s, strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

static int _ez_net_tcp_server(int port, char* bindaddr, int af, int backlog)
{
    int s, rv;
    char _port[7]; /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    memset(_port, 0, sizeof(_port));
    ez_snprintf(_port, 6, "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af; // AF_INET, AF_INET6, AF_UNIX, AF_LOCAL
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; /* No effect if bindaddr != NULL */

    if ((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0) {
        log_error("getaddrinfo error:%s", gai_strerror(rv));
        return ANET_ERR;
    }
    s = 0;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        log_info("call sockt (%s,%s,%s) = %d", socket_family_name(p->ai_family), socket_socktype_name(p->ai_socktype),
            socket_protocol_name(p->ai_protocol), s);
        if (af == AF_INET6 && ez_net_set_ipv6_only(s) == ANET_ERR)
            goto error;
        if (ez_net_set_reuse_addr(s) == ANET_ERR)
            goto error;
        if (ez_net_set_non_block(s) == ANET_ERR)
            goto error;
        if (ez_net_set_reuse_port(s) == ANET_ERR)
            goto error;
        if (ez_net_bind(s, p->ai_addr, p->ai_addrlen) == ANET_ERR)
            goto error;
        if (ez_net_listen(s, backlog) == ANET_ERR)
            goto error;
        goto end;
    }

    if (p == NULL) {
        log_error("unable create TcpServer.");
        goto error;
    }

error:
    if (s != 0)
        ez_net_close_socket(s);
    s = ANET_ERR;
end:
    freeaddrinfo(servinfo);
    return s;
}

int ez_net_unix_server(char* path, int backlog)
{
    int s;
    struct sockaddr_un sa;

    // AF_LOCAL => AF_UNIX
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return ANET_ERR;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);

    if (ez_net_bind(s, (struct sockaddr*)&sa, sizeof(sa)) == ANET_ERR) {
        ez_net_close_socket(s);
        return ANET_ERR;
    }
    if (ez_net_listen(s, backlog) == ANET_ERR) {
        ez_net_close_socket(s);
        return ANET_ERR;
    }
    return s;
}

int ez_net_tcp_server(int port, char* bindaddr, int backlog)
{
    return _ez_net_tcp_server(port, bindaddr, AF_INET, backlog);
}

int ez_net_tcp6_server(int port, char* bindaddr, int backlog)
{
    return _ez_net_tcp_server(port, bindaddr, AF_INET6, backlog);
}

static int ez_net_tcp_generic_accept(int s, struct sockaddr* sa, socklen_t* len)
{
    int fd;
    int ezerrno;

#ifdef EZ_USE_ACCEPT4
    fd = accept4(s, sa, len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    fd = accept(s, sa, len);
#endif
    if (fd == -1) {
        ezerrno = errno;
        // linux 上 #define EWOULDBLOCK EAGAIN
        if (ezerrno == EAGAIN || ezerrno == EINTR /*|| ezerrno == EWOULDBLOCK */) {
            log_error("sever socket %d accept() not ready.", s);
            return ANET_EAGAIN;
        } else if (ezerrno == EMFILE || ezerrno == ENFILE) {
            // file handle over, disabled accept.
            log_warn("sever socket %d accept() error: %s \n You must disabled accept().", s, strerror(ezerrno));
            return ANET_EMEN_FILE;
        } else if (ezerrno == ECONNABORTED) {
            log_warn("sever socket %d accept() error: %s", s, strerror(ezerrno));
            return ANET_ERR;
        } else {
            log_error("sever socket %d accept() error: %s", s, strerror(ezerrno));
            return ANET_ERR;
        }
    }

#ifdef EZ_USE_ACCEPT4
    // 不作其他处理.
#else
    ez_net_set_closexec(fd);
    ez_net_set_non_block(fd);
#endif
    return fd;
}

int ez_net_unix_accept(int s)
{
    int fd;
    struct sockaddr_un sa;

    socklen_t salen = sizeof(sa);
    fd = ez_net_tcp_generic_accept(s, (struct sockaddr*)&sa, &salen);

    if (fd < ANET_OK) // accept error.
        return ANET_ERR;

    return fd;
}

int ez_net_tcp_accept(int fd)
{
    return ez_net_tcp_accept2(fd, NULL, 0, NULL);
}

int ez_net_tcp_accept2(int s, char* ip, size_t ip_len, int* port)
{
    int fd;
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);
    fd = ez_net_tcp_generic_accept(s, (struct sockaddr*)&sa, &salen);

    if (fd < ANET_OK) // accept error.
        return fd;

    if (sa.ss_family == AF_INET) {
        struct sockaddr_in* si4 = (struct sockaddr_in*)&sa;
        if (ip)
            inet_ntop(AF_INET, (void*)&(si4->sin_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(si4->sin_port);
    } else {
        struct sockaddr_in6* si6 = (struct sockaddr_in6*)&sa;
        if (ip)
            inet_ntop(AF_INET6, (void*)&(si6->sin6_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(si6->sin6_port);
    }
    return fd;
}

int ez_net_close_socket(int s)
{
    if (s > 0)
        close(s);
    return ANET_OK;
}

/* create client */
#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1

static int ez_net_unix_connect_ex(const char* path, int flags)
{
    int s;
    struct sockaddr_un sa;

    // AF_LOCAL => AF_UNIX
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return ANET_ERR;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path, path, sizeof(sa.sun_path) - 1);
    if ((flags & ANET_CONNECT_NONBLOCK) && ez_net_set_non_block(s) != ANET_OK) {
        ez_net_close_socket(s);
        s = ANET_ERR;
        return s;
    }

    if (connect(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        /* If the socket is non-blocking, it is ok for connect() to
         * return an EINPROGRESS error here. */
        if (errno == EINPROGRESS && (flags & ANET_CONNECT_NONBLOCK))
            goto end;
        else
            log_error("connect to unix:%s failed, cause:[%s]!", path, strerror(errno));
        // close
        if (s != ANET_ERR) {
            ez_net_close_socket(s);
            s = ANET_ERR;
        }
    }

end:
    return s;
}

int ez_net_unix_connect(const char* path)
{
    return ez_net_unix_connect_ex(path, ANET_CONNECT_NONE);
}

int ez_net_unix_connect_non_block(const char* path)
{
    return ez_net_unix_connect_ex(path, ANET_CONNECT_NONBLOCK);
}

static int ez_net_tcp_connect_ex(const char* addr, int port, int flags)
{
    int s = ANET_ERR, rv;
    char portstr[6]; /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *p;

    memset(portstr, 0, sizeof(portstr));
    snprintf(portstr, sizeof(portstr), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, portstr, &hints, &servinfo)) != 0) {
        log_error("%s", gai_strerror(rv));
        return ANET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Try to create the socket and to connect it.
         * If we fail in the socket() call, or on connect(), we retry with
         * the next entry in servinfo. */

        if ((s = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        log_info("call sockt (%s,%s,%s) = %d", socket_family_name(p->ai_family), socket_socktype_name(p->ai_socktype),
            socket_protocol_name(p->ai_protocol), s);

        if (ez_net_set_reuse_addr(s) == ANET_ERR)
            goto error;
        if ((flags & ANET_CONNECT_NONBLOCK) && ez_net_set_non_block(s) != ANET_OK)
            goto error;
        if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && (flags & ANET_CONNECT_NONBLOCK))
                goto end;
            else {
                log_error("connect to %s:%d failed, cause:[%s]!", addr, port, strerror(errno));
            }
            // close
            if (s != ANET_ERR) {
                ez_net_close_socket(s);
                s = ANET_ERR;
            }
            continue;
        }

        /* If we ended an iteration of the for loop without errors, we
         * have a connected socket. Let's return to the caller. */
        goto end;
    }

error:
    if (s != ANET_ERR) {
        ez_net_close_socket(s);
        s = ANET_ERR;
    }

end:
    freeaddrinfo(servinfo);
    return s;
}

int ez_net_tcp_connect(const char* addr, int port)
{
    return ez_net_tcp_connect_ex(addr, port, ANET_CONNECT_NONE);
}

int ez_net_tcp_connect_non_block(const char* addr, int port)
{
    return ez_net_tcp_connect_ex(addr, port, ANET_CONNECT_NONBLOCK);
}

/* socket option */
int ez_net_set_send_buf_size(int fd, int bufsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == -1) {
        log_error("setsockopt SO_SNDBUF: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int ez_net_set_recv_buf_size(int fd, int bufsize)
{
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == -1) {
        log_error("setsockopt SO_SNDBUF: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int ez_net_set_closexec(int fd)
{
    int flags;
#ifdef USE_IOCTL
    flags = 1;
    if (ioctl(fd, FIONBIO, &flags) == -1)
        return ANET_ERR;
#else
    /* Set the socket non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        log_error("fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }
    if (fcntl(fd, F_SETFL, flags | FD_CLOEXEC) == -1) {
        log_error("fcntl(F_SETFL,FD_CLOEXEC): %s", strerror(errno));
        return ANET_ERR;
    }
#endif
    return ANET_OK;
}

int ez_net_set_non_block(int fd)
{
    int flags;
#ifdef USE_IOCTL
    flags = 0;
    if (ioctl(fd, FIONBIO, &flags) == -1)
        return ANET_ERR;
#else
    /* Set the socket non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        log_error("fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_error("fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
#endif
    return ANET_OK;
}

int ez_net_set_reuse_port(int fd)
{
    int reuseport = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport)) == -1) {
        log_error("setsocopt SO_REUSEPORT :%s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int ez_net_set_reuse_addr(int fd)
{
    int yes = 1;
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        log_error("setsockopt SO_REUSEADDR: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int ez_net_set_tcp_nodelay(int fd, int val)
{
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
        log_error("setsockopt TCP_NODELAY: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int ez_net_tcp_keepalive(int fd, int interval)
{
    int val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        log_error("setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return ANET_ERR;
    }
#ifdef __linux__
    // linux是在空闲7200秒后，进行最大9次发送控测报文，每次间隔 75 秒。
    // net.ipv4.tcpkeepalivetime = 7200   Sokcet.TCP_KEEPIDLE
    // net.ipv4.tcpkeepaliveprobes = 9    Sokcet.TCP_KEEPCNT
    // net.ipv4.tcpkeepaliveintvl = 75    Socket.TCP_KEEPINTVL
    if (interval > 0) {
        /* Default settings are more or less garbage, with the keepalive time
         * set to 7200 by default on Linux. Modify settings to make the feature
         * actually useful. */

        /* Send first probe after interval. */
        val = interval;
        if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
            log_error("setsockopt linux TCP_KEEPIDLE: %s", strerror(errno));
            return ANET_ERR;
        }
        /* Send next probes after the specified interval. Note that we set the
         * delay as interval / 3, as we send three probes before detecting
         * an error (see the next setsockopt call). */
        val = interval / 3;
        if (val == 0)
            val = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
            log_error("setsockopt linux TCP_KEEPINTVL: %s", strerror(errno));
            return ANET_ERR;
        }

        /* Consider the socket in error state after three we send three ACK
         * probes without getting a reply. */
        val = 3;
        if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
            log_error("setsockopt linux TCP_KEEPCNT: %s", strerror(errno));
            return ANET_ERR;
        }
    }
#else
    ((void)interval); /* Avoid unused var warning for non Linux systems. */
#endif

    return ANET_OK;
}

int ez_net_set_ipv6_only(int fd)
{
    int yes = 1;
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes)) == -1) {
        log_error("setsockopt: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

/*
 * If flags is set to RESOLVE_IP_ONLY the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function.
 */
#define RESOLVE_NAME 0
#define RESOLVE_IP_ONLY 1
static int ez_net_resolve_generic(char* host, char* ipbuf, size_t ipbuf_len, int flags)
{
    struct addrinfo hints, *info;
    int rv;

    memset(&hints, 0, sizeof(hints));
    if (flags & RESOLVE_IP_ONLY)
        hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM; /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
        log_error("%s", gai_strerror(rv));
        return ANET_ERR;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in* sa = (struct sockaddr_in*)info->ai_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ipbuf, (socklen_t)ipbuf_len);
    } else {
        struct sockaddr_in6* sa = (struct sockaddr_in6*)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, (socklen_t)ipbuf_len);
    }

    freeaddrinfo(info);
    return ANET_OK;
}

int ez_net_resolve_host_name(char* host, char* ipbuf, size_t ipbuf_len)
{
    return ez_net_resolve_generic(host, ipbuf, ipbuf_len, RESOLVE_NAME);
}

int ez_net_resolve_host_ip(char* host, char* ipbuf, size_t ipbuf_len)
{
    return ez_net_resolve_generic(host, ipbuf, ipbuf_len, RESOLVE_IP_ONLY);
}

/* NonBlock net_read & net_write */
int ez_net_read(int fd, char* buf, size_t bufsize, ssize_t* nbytes)
{
    int ezerrno;
    ssize_t r = read(fd, buf, bufsize);
    if (r == 0) {
        *nbytes = 0;
        ezerrno = errno;
        if (ezerrno == EAGAIN || ezerrno == EINTR)
            return ANET_EAGAIN; // 才能继续读
        else
            return ANET_OK;
    } else if (r > 0) {
        *nbytes = r;
        return ANET_OK;
    } else {
        *nbytes = 0;
        ezerrno = errno;
        // linux define EWOULDBLOCK EAGAIN.
        if (ezerrno == EAGAIN || ezerrno == EINTR /*|| ezerrno == EWOULDBLOCK */)
            return ANET_EAGAIN; /* 非阻塞模式 */
        else
            return ANET_ERR;
    }
}

/* NonBlock net_read & net_write */
int ez_net_write(int fd, char* buf, size_t bufsize, ssize_t* nbytes)
{
    int ezerrno;
    ssize_t r = write(fd, buf, bufsize);
    if (r == 0) {
        *nbytes = 0;
        ezerrno = errno;
        if (ezerrno == EAGAIN || ezerrno == EINTR)
            return ANET_EAGAIN; // 才能继续写
        else
            return ANET_OK;
    } else if (r > 0) {
        *nbytes = r;
        return ANET_OK;
    } else {
        *nbytes = 0;
        ezerrno = errno;
        // linux define EWOULDBLOCK EAGAIN.
        if (ezerrno == EAGAIN || ezerrno == EINTR /*|| ezerrno == EWOULDBLOCK*/)
            return ANET_EAGAIN; /* 非阻塞模式 */
        else
            return ANET_ERR;
    }
}

int ez_net_peer_name(int fd, char* ip, size_t ip_len, int* port)
{
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getpeername(fd, (struct sockaddr*)&sa, &salen) == -1) {
        if (ip) {
            ip[0] = '?';
            ip[1] = '\0';
        }
        if (port)
            *port = 0;
        return -1;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&sa;
        if (ip)
            inet_ntop(AF_INET, (void*)&(s->sin_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&sa;
        if (ip)
            inet_ntop(AF_INET6, (void*)&(s->sin6_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(s->sin6_port);
    }
    return 0;
}

int ez_net_socket_name(int fd, char* ip, size_t ip_len, int* port)
{
    struct sockaddr_storage sa;
    socklen_t salen = sizeof(sa);

    if (getsockname(fd, (struct sockaddr*)&sa, &salen) == -1) {
        if (ip) {
            ip[0] = '?';
            ip[1] = '\0';
        }
        if (port)
            *port = 0;
        return -1;
    }
    if (sa.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&sa;
        if (ip)
            inet_ntop(AF_INET, (void*)&(s->sin_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&sa;
        if (ip)
            inet_ntop(AF_INET6, (void*)&(s->sin6_addr), ip, (socklen_t)ip_len);
        if (port)
            *port = ntohs(s->sin6_port);
    }
    return 0;
}

int ez_net_read_bf(int fd, bytebuf_t* buf, ssize_t* nbytes)
{
    size_t size = bytebuf_writeable_size(buf);
    uint8_t* p = bytebuf_writer_pos(buf);
    *nbytes = 0;
    int r = ez_net_read(fd, (char*)p, size, nbytes);
    if (r == ANET_OK) {
        buf->w += *nbytes;
    }
    return r;
}

int ez_net_write_bf(int fd, bytebuf_t* buf, ssize_t* nbytes)
{
    size_t size = bytebuf_readable_size(buf);
    uint8_t* p = bytebuf_reader_pos(buf);
    *nbytes = 0;
    int r = ez_net_write(fd, (char*)p, size, nbytes);
    if (r == ANET_OK) {
        buf->r += *nbytes;
    }
    return r;
}