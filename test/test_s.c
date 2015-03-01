
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <ez_util.h>
#include <ez_net.h>
#include <ez_log.h>
#include <ez_event.h>

typedef struct ezWorker_t {
	int32_t id;
	pthread_t thid;
	ezEventLoop *w_event;
} ezWorker;

struct ez_signal {
    int  signo;
    char *signame;
    int  flags;
    void (*handler)(int signo);
};

static ezWorker boss ;
#define WORKER_SIZE			4
static ezWorker workers[WORKER_SIZE];
static int last_worker = 0;

static void cust_signal_handler(int signo);

static struct ez_signal cust_signals[] = {
    { SIGHUP,  "SIGHUP",  0, SIG_IGN },
    { SIGPIPE, "SIGPIPE", 0, SIG_IGN },
    { SIGINT,  "SIGINT",  0, cust_signal_handler },
    { 0,       "NULL",    0, NULL }
};

static void cust_signal_init(void)
{
    struct ez_signal *sig;
	int status;

    for (sig = cust_signals; sig->signo != 0; sig++) {

		struct sigaction sa;
        memset(&sa, 0, sizeof(sa));

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

static void cust_signal_handler(int signo)
{
    struct ez_signal *sig;
    for (sig = cust_signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

	log_info("signal %d (%s) received", signo, sig->signame);
	if (signo == 0) return;

    switch (signo) {
    case SIGINT:
		ez_stop_event_loop(boss.w_event);
        break;
    default:
        ;
    }
}

void *ez_worker_thread_run(void *t)
{
	ezWorker *worker = (ezWorker *) t;
	worker->thid = pthread_self();

	log_info("worker id: %d run event loop .", worker->id);
	ez_run_event_loop(worker->w_event);
	return NULL;
}

void read_client_handler(ezEventLoop * eventLoop, int c, void *clientData, int mask)
{
#define buf_size 32
	char buf[buf_size + 1];
	int r = 0;
	ssize_t nread = 0;

	EZ_NOTUSED(clientData);
	EZ_NOTUSED(mask);

	r = ez_net_read(c, buf, buf_size, &nread);
	if (r == ANET_ERR) {
		log_info("read client %d error:[%d,%s]!", c, errno, strerror(errno));
		ez_delete_file_event(eventLoop, c, AE_READABLE);
		ez_net_close_socket(c);
		return;

	} else if (r == ANET_EAGAIN) {
		if (nread == 0)
			return;
	} else if (r == ANET_OK && nread == 0) {
		// ez_event 会在 client 端close也引发 AE_READABLE 事件，
		// 这时读取这个socket的结果就是读取0字节内容。
		log_info("client %d closed connection !", c);
		ez_delete_file_event(eventLoop, c, AE_READABLE);
		ez_net_close_socket(c);
		return;
	}
	if (nread) {
		buf[nread] = '\0';
		log_hexdump(LOG_INFO, buf, nread, "read client id:%d read %d bytes!", c, nread);
	}
}

void ez_worker_push_client(int c)
{
	int wid = (last_worker + 1) % WORKER_SIZE;
	last_worker = wid;

	ez_net_set_non_block(c);
	ez_net_tcp_enable_nodelay(c);
	ez_net_tcp_keepalive(c, 30);

	log_info("worker id:%d add new client %d .", workers[wid].id, c);
	if (ez_create_file_event(workers[wid].w_event, c, AE_READABLE, read_client_handler, NULL) == AE_ERR) {
		log_info("worker id:%d add new client %d(AE_READABLE) failed!", workers[wid].id, c);
		ez_net_close_socket(c);
	}
}

void init_workers()
{
	pthread_t thid;
	for (int i = 0; i < WORKER_SIZE; ++i) {
		workers[i].id = i + 1;

		log_info("create worker id: %d event loop .", workers[i].id);
		workers[i].w_event = ez_create_event_loop(1024);
		pthread_create(&thid, NULL, ez_worker_thread_run, &workers[i]);
	}
}

void stop_free_workers()
{
	for (int i = 0; i < WORKER_SIZE; ++i) {
		log_info("send worker id: %d exit event loop .", workers[i].id);
		ez_stop_event_loop(workers[i].w_event);

		log_info("wait worker id: %d thread exit.", workers[i].id);
		pthread_join(workers[i].thid, NULL);

		log_info("delete worker id: %d event loop .", workers[i].id);
		ez_delete_event_loop(workers[i].w_event);
	}
}

void accept_handler(ezEventLoop * eventLoop, int s, void *clientData, int mask)
{
	EZ_NOTUSED(eventLoop);
	EZ_NOTUSED(clientData);
	EZ_NOTUSED(mask);

	int c = ez_net_tcp_accept(s);
	if (c == ANET_EAGAIN) {
		return;
	}
	if (c == ANET_ERR) {
		log_info("server accept error [%d,%s]", errno, strerror(errno));
		return;
	}
	ez_worker_push_client(c);
}

int time_out_handler(ezEventLoop * eventLoop, int64_t timeId, void *clientData)
{
	EZ_NOTUSED(eventLoop);
	EZ_NOTUSED(clientData);
	log_info("==> timeId %li !", timeId);
	return AE_TIMER_NEXT;
}

void init_boss()
{
	log_info("create main event loop...");
	boss.id = 0;
	boss.thid = pthread_self();
	boss.w_event = ez_create_event_loop(128);
}

void run_boss()
{
	log_info("run main event loop .");
	ez_run_event_loop(boss.w_event);
}

void stop_free_boss()
{
	log_info("stop main event loop exit, delete it!");
	ez_stop_event_loop(boss.w_event);
	ez_delete_event_loop(boss.w_event);
}

int main(int argc, char **argv)
{
	int port = 9090;
	EZ_NOTUSED(argc);
	EZ_NOTUSED(argv);

	cust_signal_init();
	log_init(LOG_INFO, NULL);

	int s = ez_net_tcp_server(port, NULL, 1024);
	log_info("server %d at port %d wait client ...", s, port);

	init_boss();
	init_workers();

	// add s accept
	ez_create_file_event(boss.w_event, s, AE_READABLE, accept_handler, NULL);
	// test time out.
	ez_create_time_event(boss.w_event, 5000, time_out_handler, NULL);

	run_boss();
	stop_free_boss();
	stop_free_workers();

	ez_net_close_socket(s);
	log_release();
	return 0;
}
