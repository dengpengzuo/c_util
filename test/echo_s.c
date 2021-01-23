#include <string.h>
#include <ez_macro.h>
#include <ez_net.h>
#include <ez_log.h>
#include <ez_event.h>
#include <ez_malloc.h>
#include "cust_sign.h"

typedef struct client_s {
    int len;
    int cap;
    int fds[0];
} client_t;

typedef struct server_s {
    char *addr;
    int port;
    int s;
    ez_event_loop_t *ez_loop;
    client_t *clients;
} server_t;

static server_t *server = NULL;

void signal_quit_handler(struct ez_signal *sig) {
    EZ_NOTUSED(sig);
    if (server != NULL) {
        ez_stop_event_loop(server->ez_loop);
    }
}

void echo_client_handler(ez_event_loop_t *eventLoop, int c, void *data, int mask) {

}

void accept_handler(ez_event_loop_t *eventLoop, int s, void *data, int mask) {
    EZ_NOTUSED(eventLoop);
    EZ_NOTUSED(data);
    EZ_NOTUSED(mask);

    int c = ez_net_unix_accept(s);
    if (c < ANET_OK) {
        return;
    }
    log_info("server accept client [fd:%d] ... ", c);

    ez_net_set_non_block(c);
    ez_net_tcp_enable_nodelay(c);
    ez_net_tcp_keepalive(c, 300);

    if (ez_create_file_event(server->ez_loop, c, AE_READABLE, echo_client_handler, NULL) == AE_ERR) {
        log_error("server add new client [fd:%d](AE_READABLE) failed!", c);
        ez_net_close_socket(c);
        return;
    }

    log_info("server add new client [fd:%d] in event_loop.", c);
}

void run_echo_server(server_t *svr) {
    log_info("server %d bind %s:%d wait client ...", svr->s, svr->addr, svr->port);
    ez_create_file_event(svr->ez_loop, svr->s, AE_READABLE, &accept_handler, NULL);
    ez_run_event_loop(svr->ez_loop);
}

client_t *new_client_array(int size) {
    client_t *c= ez_malloc(sizeof(client_t) + sizeof(int) * size);
    c->len = 0;
    c->cap = size;
    return c;
}

void free_client_array(client_t *client) {
    ez_free(client);
}

int main(int argc, char **argv) {
    char *addr = NULL;
    int port = 9090;
    EZ_NOTUSED(argc);
    EZ_NOTUSED(argv);

    cust_signal_init();
    log_init(LOG_INFO, NULL);

    server = ez_malloc(sizeof(server_t));
    server->addr = addr;
    server->port = port;
    server->ez_loop = ez_create_event_loop(1024);
    server->clients = new_client_array(1024);

    server->s = ez_net_tcp_server(server->port, server->addr, 1024);
    if (server->s > 0) {
        run_echo_server(server);
        ez_net_close_socket(server->s);
    }

    ez_delete_event_loop(server->ez_loop);
    free_client_array(server->clients);
    ez_free(server);
    log_release();
    return 0;
}
