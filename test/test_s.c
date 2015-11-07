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

typedef struct ezWorker_t {
    int32_t id;
    pthread_t thid;
    ezEventLoop *w_event;
    list_head clients;

    pthread_mutex_t lock;
    pthread_cond_t cond;
} ezWorker;

typedef struct connect_t {
    list_head list_node;

    int fd;
    char addr[256];
    int port;

    uint64_t last_read;
    /* 上一次读取消息时间 */
    uint64_t last_write; /* 上一次写入消息时间 */
} connect;

static inline connect *cast_to_connect(list_head *entry) {
    return EZ_CONTAINER_OF(entry, struct connect_t, list_node);
}

struct ez_signal {
    int signo;
    int flags;
    char *signame;

    void (*handler)(int signo);

    void (*cust_handler)(struct ez_signal *sig);
}; // 字节对齐, sizeof(struct ez_signal)=32 bytes.

static ezWorker boss;
static ezWorker workers[4];

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
    ez_stop_event_loop(boss.w_event);
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
    ezWorker *worker = (ezWorker *) t;
    worker->thid = pthread_self();

    ez_run_event_loop(worker->w_event);

    pthread_mutex_lock(&(worker->lock));
    log_info("worker [%d] > event_thread_proc exit.", worker->id);
    pthread_cond_signal(&(worker->cond));   // 发出信号，表示退出了.
    pthread_mutex_unlock(&(worker->lock));

    return NULL;
}

void read_client_handler(ezEventLoop *eventLoop, int c, void *clientData, int mask) {
    connect *client = (connect *) clientData;
}

void accept_handler(ezEventLoop *eventLoop, int s, void *clientData, int mask) {
    EZ_NOTUSED(eventLoop);
    EZ_NOTUSED(clientData);
    EZ_NOTUSED(mask);

    int c = ez_net_tcp_accept(s);
    if (c < ANET_OK) {
        return;
    }
    uint32_t wid = (uint32_t) (c & (EZ_NELEMS(workers) - 1));

    connect *client = ez_malloc(sizeof(connect));
    if (client == NULL) {
        log_error("server malloc connect faired, force close [%d].", c);
        ez_net_close_socket(c);
        return;
    }
    client->fd = c;
    ez_net_peer_name(c, client->addr, 255, &client->port);
    log_info("server accept client [id:%d, %s:%d]", client->fd, client->addr, client->port);

    ez_net_set_non_block(client->fd);
    ez_net_tcp_enable_nodelay(client->fd);
    ez_net_tcp_keepalive(client->fd, 120);

    if (ez_create_file_event(workers[wid].w_event, client->fd, AE_READABLE, read_client_handler, client) == AE_ERR) {
        log_error("server add new client %d(AE_READABLE) failed!", workers[wid].id, client->fd);
        ez_net_close_socket(client->fd);
        ez_free(client);
        return;
    }

    log_info("server add new client %d to worker [%d].", client->fd, workers[wid].id);
    list_add(&client->list_node, &workers[wid].clients);
}

void init_boss() {
    pthread_t thid;

    boss.id = 0;
    boss.w_event = ez_create_event_loop(128);
    pthread_mutex_init(&(boss.lock), NULL);
    pthread_cond_init(&(boss.cond), NULL);

    pthread_create(&thid, NULL, run_event_thread_proc, &boss);
}

void init_server(int s) {
    // 将s加入boss接收clent连接.
    ez_create_file_event(boss.w_event, s, AE_READABLE, accept_handler, NULL);
}

int time_out_handler(ezEventLoop *eventLoop, int64_t timeId, void *clientData) {
    ezWorker *worker = (ezWorker *) clientData;
    log_info("worker [%d]> timeId:%li clean die client.", worker->id, timeId);

    connect *con = NULL;
    LIST_FOR(&worker->clients) {
        con = cast_to_connect(pos);
        log_info("worker [%d]> timeId:%li 处理 client [%d] 超时 ... ", worker->id, timeId, con->fd);
    }
    return AE_TIMER_NEXT;
}

void init_workers() {
    pthread_t thid;

    for (int i = 0; i < EZ_NELEMS(workers); ++i) {
        workers[i].id = i + 1;
        workers[i].w_event = ez_create_event_loop(1024);
        init_list_head(&workers[i].clients);

        pthread_mutex_init(&(workers[i].lock), NULL);
        pthread_cond_init(&(workers[i].cond), NULL);

        // add one time out event.
        ez_create_time_event(workers[i].w_event, 1000 * 30, time_out_handler, &workers[i]);

        pthread_create(&thid, NULL, run_event_thread_proc, &workers[i]);
    }
}

void wait_and_stop_boss() {
    log_info("main > wait boss event loop ...");
    pthread_mutex_lock(&boss.lock);
    pthread_cond_wait(&boss.cond, &boss.lock);          // wait cust_signal_handler 引发 run_event_thread_proc 线程退出消息.
    pthread_mutex_unlock(&boss.lock);

    log_info("main > begin stop boss event loop ...");
    pthread_join(boss.thid, NULL);
    log_info("main > delete boss event loop ...");
    ez_delete_event_loop(boss.w_event);

    pthread_mutex_destroy(&boss.lock);
    pthread_cond_destroy(&boss.cond);
}

void free_connect(list_head *entry) {
    connect *con = cast_to_connect(entry);
    ez_free(con);
}

void wait_and_stop_workers() {
    for (int i = 0; i < EZ_NELEMS(workers); ++i) {
        log_info("main > wait worker[%d] event loop break ...", workers[i].id);

        pthread_mutex_lock(&workers[i].lock);
        ez_stop_event_loop(workers[i].w_event);                // 发送退出消息.
        pthread_cond_wait(&workers[i].cond, &workers[i].lock); // 注意:pthread_con_wait必须先有wait，其他线程的cond_signal才会收得到.
        pthread_mutex_unlock(&workers[i].lock);

        pthread_join(workers[i].thid, NULL);

        log_info("main > delete worker[%d] event loop ...", workers[i].id);
        ez_delete_event_loop(workers[i].w_event);
        pthread_mutex_destroy(&workers[i].lock);
        pthread_cond_destroy(&workers[i].cond);

        list_foreach(&workers[i].clients, free_connect);
    }
}

int main(int argc, char **argv) {
    int port = 9090;
    EZ_NOTUSED(argc);
    EZ_NOTUSED(argv);

    cust_signal_init();
    log_init(LOG_INFO, NULL);

    int s = ez_net_tcp_server(port, "0.0.0.0", 1024);    // 监听的SRC := 0.0.0.0
    log_info("server %d bind %s:%d wait client ...", s, "0.0.0.0", port);

    init_boss();
    init_workers();

    init_server(s);

    wait_and_stop_boss();
    wait_and_stop_workers();

    ez_net_close_socket(s);

    log_release();
    return 0;
}
