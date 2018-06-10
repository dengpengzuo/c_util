#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <ez_macro.h>
#include <ez_malloc.h>
#include <ez_net.h>
#include <ez_log.h>
#include <ez_event.h>
#include <ez_list.h>

typedef struct worker_s {
    int32_t id;
    pthread_t thid;
    ez_event_loop_t *w_event;

    pthread_mutex_t lock;
    pthread_cond_t cond;
} worker_t;

typedef struct connect_list_s {
    list_head_t clients;           // list<connect>
    pthread_mutex_t client_lock; // client_lock
} connect_list_t;

typedef struct worker_connect_s {
    list_head_t list_node;
    worker_t *worker;  /* 所属的worker */
    int fd;            /* socket fd */

    #define CONNECT_READ_BUF_SIZE 8
    uint8_t data[CONNECT_READ_BUF_SIZE];      /* read buffer */
} worker_connect_t;

static inline worker_connect_t *cast_to_connect(list_head_t *entry) {
    return EZ_CONTAINER_OF(entry, worker_connect_t, list_node);
}

struct ez_signal {
    int signo;
    int flags;
    char *signame;

    void (*handler)(int signo);

    void (*cust_handler)(struct ez_signal *sig);
}; // 字节对齐, sizeof(struct ez_signal)=32 bytes.

#define WORKER_SIZE   4
static worker_t     _boss;
static worker_t     _workers[WORKER_SIZE];
static connect_list_t  _worker_clients[WORKER_SIZE];

static inline uint32_t _worker_index(int fd) {
    return (uint32_t) (fd & (EZ_MARK(EZ_NELEMS(_workers))));
}

static inline void _connect_list_lock(connect_list_t *wc) {
    pthread_mutex_lock(&(wc->client_lock));
}

static inline void _connect_list_unlock(connect_list_t *worker) {
    pthread_mutex_unlock(&(worker->client_lock));
}

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

static void cust_signal_handler(struct ez_signal *sig) {
    EZ_NOTUSED(sig);
    ez_stop_event_loop(_boss.w_event);
}

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
    struct sigaction sa;

    for (sig = cust_signals; sig->signo != 0; sig++) {
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

void *run_event_thread_proc(void *t) {
    worker_t *worker = (worker_t *) t;
    ez_run_event_loop(worker->w_event);

    pthread_mutex_lock(&(worker->lock));
    log_info("worker [%d] > exit ez_run_event_loop.", worker->id);
    pthread_cond_signal(&(worker->cond));   // 发出信号，表示退出了.
    pthread_mutex_unlock(&(worker->lock));

    return NULL;
}

void read_client_handler(ez_event_loop_t *eventLoop, int cfd, void *clientData, int mask) {
    worker_connect_t *c = (worker_connect_t *) clientData;
    uint32_t wid = _worker_index(c->fd);
    int r = 0;
    ssize_t nread = 0;

    if ((mask & AE_READABLE) == AE_READABLE) {
        // 没有空间[申请]
        r = ez_net_read(c->fd, (char *) &(c->data[0]), CONNECT_READ_BUF_SIZE, &nread);

        if (r == ANET_EAGAIN && nread == 0) {
            return; // 读不出来或者数据没有来，下次继续读
        } else if (r == ANET_OK && nread == 0) {
            // ez_event 会在 client 端close也引发 AE_READABLE 事件，
            // 这时读取这个socket的结果就是读取0字节内容。
            log_info("client [%d] > 已经关闭，reomve this from _workers[%d]", c->fd, c->worker->id);

            ez_delete_file_event(/*eventLoop*/c->worker->w_event, c->fd, AE_READABLE);
            ez_net_close_socket(c->fd);

            _connect_list_lock(&_worker_clients[wid]);
            list_del(&c->list_node);
            _connect_list_unlock(&_worker_clients[wid]);

            ez_free(c);
            return;
        } else if (r == ANET_ERR) {
            log_info("client [%d] > read error:[%d,%s]!", c->fd, errno, strerror(errno));
        }
        if (nread > 0) {
            // 读取了 nread.
            log_hexdump(LOG_INFO, c->data, CONNECT_READ_BUF_SIZE, "client [%d] > read data %d bytes!", c->fd, nread);
        }
    }
}

void accept_handler(ez_event_loop_t *eventLoop, int s, void *clientData, int mask) {
    EZ_NOTUSED(eventLoop);
    EZ_NOTUSED(clientData);
    EZ_NOTUSED(mask);

    int c = ez_net_unix_accept(s);
    if (c < ANET_OK) {
        return;
    }
    uint32_t wid = _worker_index(c);
    worker_t *worker = &_workers[wid];

    worker_connect_t *client = ez_malloc(sizeof(worker_connect_t));
    if (client == NULL) {
        log_error("server malloc connect faired, force close [%d].", c);
        ez_net_close_socket(c);
        return;
    }
    client->worker = worker;
    client->fd = c;
    log_info("server accept client [fd:%d]", client->fd);

    ez_net_set_non_block(client->fd);
    ez_net_tcp_enable_nodelay(client->fd);
    ez_net_tcp_keepalive(client->fd, 300);

    if (ez_create_file_event(worker->w_event, client->fd, AE_READABLE, read_client_handler, client) == AE_ERR) {
        log_error("server add new client [fd:%d](AE_READABLE) failed!", worker->id, client->fd);
        ez_net_close_socket(client->fd);
        ez_free(client);
        return;
    }

    log_info("server add new client [fd:%d] to worker [%d].", client->fd, worker->id);
    // _boss 线程锁定 worker
    _connect_list_lock(&_worker_clients[wid]);
    list_add(&client->list_node, &(_worker_clients[wid].clients));
    _connect_list_unlock(&_worker_clients[wid]);
}

void init_boss() {
    worker_t *boss = &_boss;
    boss->id = 0;
    boss->w_event = ez_create_event_loop(128);
    pthread_mutex_init(&(boss->lock), NULL);
    pthread_cond_init(&(boss->cond), NULL);
}

void init_workers() {
    for (int i = 0; i < EZ_NELEMS(_workers); ++i) {
        worker_t * w = &_workers[i];
        connect_list_t * cl = &_worker_clients[i];

        w->id = (i+1);
        w->w_event = ez_create_event_loop(1024);
        pthread_mutex_init(&(w->lock), NULL);
        pthread_cond_init(&(w->cond), NULL);

        init_list_head(&(cl->clients));
        pthread_mutex_init(&(cl->client_lock), NULL);
    }
}

void init_server(int s) {
    worker_t *boss = &_boss;
    // 将s加入boss接收clent连接.
    ez_create_file_event(boss->w_event, s, AE_READABLE, accept_handler, NULL);
    // 生成boss线程.
    pthread_create(&boss->thid, NULL, run_event_thread_proc, boss);

    // 生成worker线程
    for (int i = 0; i < EZ_NELEMS(_workers); ++i) {
        pthread_create(&_workers[i].thid, NULL, run_event_thread_proc, &_workers[i]);
    }
}

void wait_and_stop_boss() {
    worker_t *boss = &_boss;
    log_info("main > wait boss event loop ...");
    pthread_mutex_lock(&boss->lock);                     // 先锁定
    pthread_cond_wait(&boss->cond, &boss->lock);         // wait cust_signal_handler 引发 run_event_thread_proc 线程退出消息.
    pthread_mutex_unlock(&boss->lock);

    // _boss thread 是被其他操作系统用线号通知.
    log_info("main > wait boss thread stop event loop ...");
    pthread_join(boss->thid, NULL);

    log_info("main > delete boss event loop ...");
    ez_delete_event_loop(boss->w_event);
    pthread_mutex_destroy(&boss->lock);
    pthread_cond_destroy(&boss->cond);
}

void free_connect(list_head_t *entry) {
    worker_connect_t *con = cast_to_connect(entry);
    ez_free(con);
}

void wait_and_stop_workers() {
    for (int i = 0; i < EZ_NELEMS(_workers); ++i) {
        pthread_mutex_lock(&_workers[i].lock);                  // 先锁定

        log_info("main > stop worker[%d] event loop.", _workers[i].id);
        ez_stop_event_loop(_workers[i].w_event);                // 主线程通知退出.

        pthread_cond_wait(&_workers[i].cond, &_workers[i].lock); // 注意:pthread_con_wait 必须先有waiter，其他线程的cond_signal才会收得到.
        pthread_mutex_unlock(&_workers[i].lock);

        log_info("main > wait worker[%d] thread exit ...", _workers[i].id);
        pthread_join(_workers[i].thid, NULL);

        log_info("main > delete worker[%d] event loop .", _workers[i].id);
        ez_delete_event_loop(_workers[i].w_event);
        pthread_mutex_destroy(&_workers[i].lock);
        pthread_cond_destroy(&_workers[i].cond);

        list_foreach(&_worker_clients[i].clients, free_connect);
    }
}

int main(int argc, char **argv) {
    char *addr = NULL;
    int port = 9090;
    EZ_NOTUSED(argc);
    EZ_NOTUSED(argv);

    cust_signal_init();
    log_init(LOG_INFO, NULL);

    int s = ez_net_tcp_server(port, addr, 1024);    // 监听的SRC := 0.0.0.0
    if (s > 0) {
        log_info("server %d bind %s:%d wait client ...", s, addr, port);

        init_boss();
        init_workers();

        init_server(s);

        wait_and_stop_boss();
        wait_and_stop_workers();

        ez_net_close_socket(s);
    }

    log_release();
    return 0;
}
