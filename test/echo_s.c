#include <string.h>
#include <ez_macro.h>
#include <ez_net.h>
#include <ez_log.h>
#include <ez_event.h>
#include <ez_malloc.h>
#include <ez_rbtree.h>
#include <ez_util.h>
#include <ez_bytebuf.h>
#include "cust_sign.h"

typedef struct client_s {
    int              fd;
    int              mask; // see @EVENT_MASK
    uint64_t         create_time;
    uint64_t         lastrec_time;
    bytebuf_t        *buf;
    ez_rbtree_node_t rbnode;
} client_t;

typedef struct server_s {
    char            *addr;
    int              port;
    int              fd;
    ez_event_loop_t *ez_loop;
    ez_rbtree_t      rb_clients;
    ez_rbtree_node_t rb_sentinel;
} server_t;

static int client_compare_proc(ez_rbtree_node_t *new_node, ez_rbtree_node_t *exists_node) {
    client_t *n = EZ_CONTAINER_OF(new_node, client_t, rbnode);
    client_t *o = EZ_CONTAINER_OF(exists_node, client_t, rbnode);
    return n->fd == o->fd ? 0 : (n->fd > o->fd ? 1 : -1);
}

extern server_t *server;

void signal_quit_handler(struct ez_signal *sig) {
    EZ_NOTUSED(sig);
    if (server != NULL) {
        ez_stop_event_loop(server->ez_loop);
    }
}

void echo_client_handler(ez_event_loop_t *eventLoop, int c, void *data, int mask) {
    EZ_NOTUSED(c);
    client_t *client = (client_t*)data;
    EZ_NOTUSED(mask);

    ssize_t nbytes;
    int r = ez_net_read_bf(client->fd, client->buf, &nbytes);
    log_debug("server read client [fd:%d] %d bytes, result: %d ", client->fd, nbytes, r);

    if (r == ANET_OK && nbytes == 0) {
        log_info("client [%d] 已经关闭.", client->fd);
        ez_delete_file_event(eventLoop, client->fd, client->mask);
        rbtree_delete(&server->rb_clients, &client->rbnode);
        free_bytebuf(client->buf);
        ez_free(client);

    } else if (r == ANET_ERR) {
        log_info("client [%d] > read error:[%d,%s]!", c, errno, strerror(errno));

    } else if (nbytes > 0) {
        r = ez_net_write_bf(client->fd, client->buf, &nbytes);
        log_debug("server write client [fd:%d] %d bytes, result: %d ", client->fd, nbytes, r);
    }
}

void accept_handler(ez_event_loop_t *eventLoop, int s, void *data, int mask) {
    EZ_NOTUSED(eventLoop);
    EZ_NOTUSED(s);
    server_t *server = (server_t*) data;
    EZ_NOTUSED(mask);

    int c = ez_net_unix_accept(server->fd);
    if (c < ANET_OK) {
        return;
    }
    log_info("server accept client [fd:%d] ... ", c);

    ez_net_set_non_block(c);
    ez_net_tcp_enable_nodelay(c);
    ez_net_tcp_keepalive(c, 300);

    client_t *client = ez_malloc(sizeof(client_t));
    client->fd = c;
    client->mask = AE_READABLE;
    client->create_time = mstime();
    client->lastrec_time = client->create_time;
    client->buf = new_bytebuf(256);
    rbtree_insert(&server->rb_clients, &client->rbnode);

    if (ez_create_file_event(server->ez_loop, client->fd, client->mask, &echo_client_handler, (void*)client) == AE_ERR) {
        log_error("server add client [fd:%d](AE_READABLE) failed!", c);
        ez_net_close_socket(client->fd);
        rbtree_delete(&server->rb_clients, &client->rbnode);
        free_bytebuf(client->buf);
        ez_free(client);
        return;
    }

    log_info("server add new client [fd:%d] in event_loop.", c);
}

void run_echo_server(server_t *svr) {
    log_info("server %d bind %s:%d wait client ...", svr->fd, svr->addr, svr->port);
    ez_create_file_event(svr->ez_loop, svr->fd, AE_READABLE, &accept_handler, svr);
    ez_run_event_loop(svr->ez_loop);
}

server_t *server = NULL;

int main(int argc, char **argv) {
    char *addr = NULL;
    int port = 9090;
    EZ_NOTUSED(argc);
    EZ_NOTUSED(argv);

    cust_signal_init();
    log_init(LOG_DEBUG, NULL);

    server = ez_malloc(sizeof(server_t));
    server->addr = addr;
    server->port = port;
    server->ez_loop = ez_create_event_loop(1024);
    rbtree_init(&server->rb_clients, &server->rb_sentinel, &client_compare_proc);

    server->fd = ez_net_tcp_server(server->port, server->addr, 1024);

    run_echo_server(server);

    ez_net_close_socket(server->fd);
    ez_delete_event_loop(server->ez_loop);

    ez_free(server);
    log_release();
    return 0;
}
