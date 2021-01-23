#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ez_net.h>
#include <ez_log.h>
#include <ez_macro.h>

#include "cust_sign.h"

void signal_quit_handler(struct ez_signal *sig) {
    EZ_NOTUSED(sig);
}

int read_char_from_stdin(char *buf, size_t bufsize) {
    ssize_t nread;

    nread = read(STDIN_FILENO, buf, bufsize);

    if (nread > 0) {
        // read 中有个回车符换行符.
        buf[nread--] = '\0';
        while (nread >= 0 && (buf[nread] == '\n' || buf[nread] == '\r')) {
            buf[nread] = '\0';
            --nread;
        }
        return (int) nread;
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
        r = read_char_from_stdin(cmd, 16);
        if (r <= 0)
            continue;
        else if (strcmp(cmd, "help") == 0) {
            log_info(
                    "help   - help \n"
                    "close  - client active close client socket\n"
                    "exit   - client exit.\n"
                    "send   - client send 16 bytes data.\n"
                    "recv   - client recv data.\n");
            continue;
        } else if (strcmp(cmd, "close") == 0) {
            log_info("client %d > 客户端主动关闭 ", c);
            ez_net_close_socket(c);
            c = 0;
            continue;
        } else if (strcmp(cmd, "exit") == 0) {
            if (c != 0) {
                ez_net_close_socket(c);
            }
            break;
        } else if (strcmp(cmd, "send") == 0) {
            r = ez_net_write(c, buf, sizeof(buf), &nwrite);
            log_info("client %d > 发送数据 %li bytes, result: %d .", c, nwrite, r);
            // 1.对端已经关闭,写入[result=>NET_OK  , nwrite => (0->N)].
            // 2.对端已经关闭,写入[result=>ANET_ERR, nwrite =>  0   ].
        } else if (strcmp(cmd, "recv") == 0) {
            r = ez_net_read(c, red, sizeof(red), &nread);
            log_hexdump(LOG_INFO, red, nread, "client %d > 接收数据 %li bytes, result: %d.", c, nread, r);
            // 对端已经关闭下，一直读取是[result=>NET_OK, nread => 0]
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    char *svr_addr = "localhost";
    int svr_port = 9090;
    char socket_name[256];
    int socket_port;

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
        ez_net_close_socket(c);
    }

    log_release();
    return 0;
}
