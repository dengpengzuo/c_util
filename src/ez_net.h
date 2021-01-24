
#ifndef EZ_NET_H
#define EZ_NET_H

#include <unistd.h>
#include "ez_bytebuf.h"

#define ANET_OK            0
#define ANET_ERR          -1
#define ANET_EAGAIN       (-EAGAIN)
#define ANET_EINTR        (-EINTR)

const char * socket_family_name(int sf);
const char * socket_socktype_name(int st);
const char * socket_protocol_name(int sp);
/* create server */
int ez_net_tcp_server(int port, char *bindaddr, int backlog);
int ez_net_tcp6_server(int port, char *bindaddr, int backlog);
int ez_net_unix_server(char *path, int backlog);

/* 标准的accept模式 */
#define ANET_EMEN_FILE    -ENFILE

int ez_net_unix_accept(int fd);

int ez_net_tcp_accept(int fd);

int ez_net_tcp_accept2(int fd, char *ip, size_t ip_len, int *port);

int ez_net_close_socket(int s);

/* create client */
int ez_net_unix_connect(const char *path);
int ez_net_unix_connect_non_block(const char *path);

int ez_net_tcp_connect(const char *addr, int port);

int ez_net_tcp_connect_non_block(const char *addr, int port);

/* NonBlock net_read & net_write
   @return ANET_EAGAIN:(非阻塞模式)读写已经执行, 读写字节数在nbytes
   @return ANET_ERR   :读写错误
   @return ANET_OK    :读写成功
 */
int ez_net_read(int fd, char *buf, size_t bufsize, ssize_t *nbytes);
int ez_net_write(int fd, char *buf, size_t bufsize, ssize_t *nbytes);

int ez_net_read_bf(int fd, bytebuf_t *buf, ssize_t * nbytes);
int ez_net_write_bf(int fd, bytebuf_t *buf, ssize_t * nbytes);

/* socket option */
int ez_net_set_send_buf_size(int fd, int bufsize);
int ez_net_set_recv_buf_size(int fd, int bufsize);

int ez_net_set_non_block(int fd);
int ez_net_set_closexec(int fd);
int ez_net_set_reuse_addr(int fd);
int ez_net_set_reuse_port(int fd);

int ez_net_set_tcp_nodelay(int fd, int val);
#define ez_net_tcp_enable_nodelay(fd)          (ez_net_set_tcp_nodelay(fd, 1))
#define ez_net_tcp_disable_nodelay(fd)         (ez_net_set_tcp_nodelay(fd, 0))

int ez_net_tcp_keepalive(int fd, int interval);
int ez_net_set_ipv6_only(int fd);

/* get socket's peer connected client ip port info */
int ez_net_peer_name(int fd, char *ip, size_t ip_len, int *port);

/** get socket's address ip port info */
int ez_net_socket_name(int fd, char *ip, size_t ip_len, int *port);

int ez_net_resolve_host_name(char *host, char *ipbuf, size_t ipbuf_len);
int ez_net_resolve_host_ip(char *host, char *ipbuf, size_t ipbuf_len);
#endif				/* EZ_NET_H */
