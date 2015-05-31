#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <ez_net.h>
#include <ez_log.h>

struct ez_signal {
    int signo;
    char *signame;
    int flags;

    void (*handler)(int signo);

    void (*cust_handler)(struct ez_signal *sig);
};

static void dispatch_signal_handler(int signo);

static struct ez_signal cust_signals[] = {
        {SIGHUP,  "SIGHUP",  0, SIG_IGN, NULL},
        {SIGPIPE, "SIGPIPE", 0, SIG_IGN, NULL},

        {SIGINT,  "SIGINT",  0, SIG_IGN, NULL},
        {SIGQUIT, "SIGQUIT", 0, SIG_IGN, NULL},
        {SIGTERM, "SIGTERM", 0, SIG_IGN, NULL},
        {0,       "NULL",    0, NULL,    NULL}
};

static void dispatch_signal_handler(int signo) {
    struct ez_signal *sig;
    for (sig = cust_signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    log_info("signal %d (%s) received", signo, sig->signame);
    if (sig->signo == 0 || sig->cust_handler == NULL) return;

    sig->cust_handler(sig);
}

static void cust_signal_init(void) {
    struct ez_signal *sig;
    int status;

    for (sig = cust_signals; sig->signo != 0; sig++) {

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        if (sig->handler == SIG_DFL)
            sa.sa_handler = dispatch_signal_handler;
        else
            sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            log_error("sigaction(%s) failed: %s", sig->signame, strerror(errno));
            return;
        }
    }
}

int read_char_from_stdin(char *buf, size_t bufsize) {
    ssize_t nread;
    int r;

    r = ez_net_read(STDIN_FILENO, buf, bufsize, &nread);

    if (r == ANET_OK && nread > 0) {
        // read 中有个回车符换行符.
        buf[nread--] = '\0';
        while(nread >= 0 && (buf[nread] == '\n' || buf[nread] == '\r')) {
            buf[nread] = '\0';
            --nread;
        }
        return nread;
    } else if (r == ANET_ERR)
        return -1;
    else
        return 0;
}

int wait_quit(int c) {
    char con[16];
    
    char red[16];
    char buf[16];
    char s;
    int r;
    ssize_t nwrite, nread;

    while (1) {
        log_info("cmd:>");
        s = read_char_from_stdin(con, 32);
        if (s <= 0)
            continue;
        else if (strcmp(con, "aclose") == 0) {
            r = ez_net_write(c, con, sizeof(con), &nwrite); 
            log_info("client %d > 要求服务器主动关闭 (Send:%d bytes, Result:%d)", c, nwrite, r); 
            continue;
        }
        else if (strcmp(con, "close") == 0) {
            log_info("client %d > 客户端主动关闭 ", c);
            ez_net_close_socket(c);
            continue;
        } 
        else if (strcmp(con, "exit") == 0 ) {
            break;
        } 
        else if (strcmp(con, "send") == 0 ) {
            r = ez_net_write(c, buf, sizeof(buf), &nwrite);
            log_info("client %d > 发送数据 %d bytes, result: %d .", c, nwrite, r);
            // 1.对端已经关闭,写入[result=>NET_OK  , nwrite => (0-N)].
            // 2.对端已经关闭,写入[result=>ANET_ERR, nwrite =>  0   ].
        }
        else if (strcmp(con, "recv") == 0) {
            r = ez_net_read(c, red, sizeof(red), &nread);
            log_hexdump(LOG_INFO, red, nread, "client %d > 接收数据 %d bytes, result: %d.", c, nread, r);
            // 对端已经关闭下，读取一直是[result=>NET_OK, nread => 0]
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    char *svr_addr = "localhost";
    int   svr_port = 9090;
    char  socket_name[256];
    int   socket_port ;

    if (argc > 1) {
        svr_addr = argv[1];
    }

    cust_signal_init();

    log_init(LOG_VVERB, NULL);

    int c = ez_net_tcp_connect(svr_addr, svr_port);
    if (c > 0) {
        ez_net_socket_name(c, socket_name, 255, &socket_port);
        socket_name[255] = '\0';
        log_info("client[id:%d %s:%d => %s:%d] connect success.", c, socket_name, socket_port, svr_addr, svr_port);

        ez_net_set_non_block(c);
        wait_quit(c);
    }

    log_release();
    return 0;
}
