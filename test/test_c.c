#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <ez_net.h>
#include <ez_log.h>
#include <ez_macro.h>

struct ez_signal {
    int  signo;
	int  flags;
	char *signame;
    void (*handler)(int signo);
    void (*cust_handler)(struct ez_signal *sig);
}; // 字节对齐, sizeof(struct ez_signal)=32 bytes.

static void dispatch_signal_handler(int signo);
static void cust_signal_handler(struct ez_signal *sig);

static struct ez_signal cust_signals[] = {
		{SIGHUP,  0, "SIGHUP",  SIG_IGN, NULL},
		{SIGPIPE, 0, "SIGPIPE", SIG_IGN, NULL},
		{SIGINT,  0, "SIGINT",  SIG_DFL, cust_signal_handler},
		{SIGQUIT, 0, "SIGQUIT", SIG_DFL, cust_signal_handler},
		{SIGTERM, 0, "SIGTERM", SIG_DFL, cust_signal_handler},
		{0,       0, "NULL",    NULL,    NULL}
};

static void cust_signal_handler(struct ez_signal *sig)
{
    EZ_NOTUSED(sig);
}

static void dispatch_signal_handler(int signo)
{
    struct ez_signal *sig;
    for (sig = cust_signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

	log_info("signal %d (%s) received", signo, sig->signame);
	if (sig->signo == 0 || sig->cust_handler == NULL ) return;

    sig->cust_handler(sig);
}

static void cust_signal_init(void)
{
    struct ez_signal *sig;
	int status;
	struct sigaction sa;

    for (sig = cust_signals; sig->signo != 0; sig++) {
        memset(&sa, 0, sizeof(sa));

        if(sig->handler == SIG_DFL)
            sa.sa_handler = dispatch_signal_handler;
        else
            sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            log_error("sigaction(%s) failed: %s", sig->signame, strerror(errno));
            return ;
        }
    }
}

int read_char_from_stdin(char *buf, size_t bufsize) {
    ssize_t nread;

    nread = read(STDIN_FILENO, buf, bufsize);

    if (nread > 0) {
        // read 中有个回车符换行符.
        buf[nread--] = '\0';
        while(nread >= 0 && (buf[nread] == '\n' || buf[nread] == '\r')) {
            buf[nread] = '\0';
            --nread;
        }
        return (int)nread;
    }
    return 0;
}

int wait_quit(int c) {
    char cmd[16];
    
    char red[16];
    char buf[16];
    int r;
    ssize_t nwrite, nread;

    while (1) {
        log_info("\ncmd:>");
        r = read_char_from_stdin(cmd, 32);
        if (r <= 0)
            continue;
        else if (strcmp(cmd, "help") == 0) {
            log_info(
            "help   - help \n"
            "sclose - client send 'server close socket' cmd\n"
            "close  - client active close client socket\n"
            "exit   - client exit.\n"
            "send   - client send 16 bytes data.\n"
            "recv   - client recv data.\n");
            continue;
        }
        else if (strcmp(cmd, "sclose") == 0) {
            r = ez_net_write(c, cmd, strlen(cmd), &nwrite);
            log_info("client %d > 要求服务器主动关闭 (Send: %li bytes, Result:%d)", c, nwrite, r);
            continue;
        }
        else if (strcmp(cmd, "close") == 0) {
            log_info("client %d > 客户端主动关闭 ", c);
            ez_net_close_socket(c);
            continue;
        } 
        else if (strcmp(cmd, "exit") == 0 ) {
            ez_net_close_socket(c);
            break;
        } 
        else if (strcmp(cmd, "send") == 0 ) {
            r = ez_net_write(c, buf, sizeof(buf), &nwrite);
            log_info("client %d > 发送数据 %li bytes, result: %d .", c, nwrite, r);
            // 1.对端已经关闭,写入[result=>NET_OK  , nwrite => (0->N)].
            // 2.对端已经关闭,写入[result=>ANET_ERR, nwrite =>  0   ].
        }
        else if (strcmp(cmd, "recv") == 0) {
            r = ez_net_read(c, red, sizeof(red), &nread);
            log_hexdump(LOG_INFO, red, nread, "client %d > 接收数据 %li bytes, result: %d.", c, nread, r);
            // 对端已经关闭下，一直读取是[result=>NET_OK, nread => 0]
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    char *svr_addr = "/tmp/test.socket";
    int   svr_port = 9090;
    char  socket_name[256];
    int   socket_port ;

    if (argc > 1) {
        svr_addr = argv[1];
    }

    cust_signal_init();

    log_init(LOG_VVERB, NULL);

    int c = ez_net_unix_connect(svr_addr/*, svr_port*/);
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
