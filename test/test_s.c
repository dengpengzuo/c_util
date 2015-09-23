#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <ez_atomic.h>
#include <ez_macro.h>
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
	int  flags;
	char *signame;
    void (*handler)(int signo);
    void (*cust_handler)(struct ez_signal *sig);
}; // 字节对齐, sizeof(struct ez_signal)=32 bytes.

static ezWorker boss ;
#define WORKER_SIZE			4
static ezWorker workers[WORKER_SIZE];
static uint32_t worker_index = 0;

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
	ez_stop_event_loop(boss.w_event);
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

void *worker_thread_proc(void *t)
{
	ezWorker *worker = (ezWorker *) t;
	worker->thid = pthread_self();

	log_info("worker %d > run event loop .", worker->id);
	ez_run_event_loop(worker->w_event);
	log_info("worker %d > stop event loop .", worker->id);
	return NULL;
}

void read_client_handler(ezEventLoop * eventLoop, int c, void *clientData, int mask)
{
	EZ_NOTUSED(clientData);
#define BUF_SIZE 16
	char buf[BUF_SIZE + 1];
	int r = 0;
	ssize_t nread = 0;
	ssize_t nwrite = 0;

	if ((mask & AE_READABLE) == AE_READABLE) {
		r = ez_net_read(c, buf, BUF_SIZE, &nread);
		if (r == ANET_EAGAIN) {
			if (nread == 0) return ; // 读不出来或者数据没有来，下次继续读
		} else if (r == ANET_OK && nread == 0) {
			// ez_event 会在 client 端close也引发 AE_READABLE 事件，
			// 这时读取这个socket的结果就是读取0字节内容。
            log_info("发现客户端 %d 已经关闭，被动关闭socket!", c);
			ez_delete_file_event(eventLoop, c, AE_READABLE);
			ez_net_close_socket(c);
			return;
		} else if (r == ANET_ERR) {
			log_info("client %d > read error:[%d,%s]!", c, errno, strerror(errno));
		}
		if (nread > 0) {
			buf[nread] = '\0';
			log_hexdump(LOG_INFO, buf, nread, "client %d > 读取 %d bytes!", c, nread);

			if (strcmp(buf, "aclose") == 0) {
				// 主动关闭
				log_info("客户端 %d 发来命令，要求服务器主动关闭 connection !", c);
				ez_delete_file_event(eventLoop, c, AE_READABLE);
				ez_net_close_socket(c);
			} else {
				// 将内容回写到客户端.
				r = ez_net_write(c, buf, (size_t) nread, &nwrite);
				log_info("client %d > 回写 %d bytes, result %d!", c, nwrite, r);
			}
		}
	}
}

void worker_push_client(int c)
{
    uint32_t wid = ATOM_FINC(&worker_index);
    wid = wid % WORKER_SIZE;

	ez_net_set_non_block(c);
	ez_net_tcp_enable_nodelay(c);
	ez_net_tcp_keepalive(c, 30);

	log_info("worker %d > add new client %d .", workers[wid].id, c);
	if (ez_create_file_event(workers[wid].w_event, c, AE_READABLE, read_client_handler, NULL) == AE_ERR) {
		log_info("worker %d > add new client %d(AE_READABLE) failed!", workers[wid].id, c);
		ez_net_close_socket(c);
	}
}

void init_workers()
{
	pthread_t thid;
	for (int i = 0; i < WORKER_SIZE; ++i) {
		workers[i].id = i + 1;

		workers[i].w_event = ez_create_event_loop(1024);
		pthread_create(&thid, NULL, worker_thread_proc, &workers[i]);
	}
}

void stop_free_workers()
{
	for (int i = 0; i < WORKER_SIZE; ++i) {
		ez_stop_event_loop(workers[i].w_event);
		pthread_join(workers[i].thid, NULL);
		ez_delete_event_loop(workers[i].w_event);
	}
}

void accept_handler(ezEventLoop * eventLoop, int s, void *clientData, int mask)
{
	EZ_NOTUSED(eventLoop);
	EZ_NOTUSED(clientData);
	EZ_NOTUSED(mask);
	char client_addr[256];
	int  client_port;
	int c = ez_net_tcp_accept(s);

	if (c < ANET_OK) {
		return;
	}

	ez_net_peer_name(c, client_addr, 255, &client_port);
	client_addr[255] = '\0';
	log_info("server accept client [id:%d, %s:%d]", c, client_addr, client_port);

	worker_push_client(c);
}

int time_out_handler(ezEventLoop * eventLoop, int64_t timeId, void *clientData)
{
	EZ_NOTUSED(eventLoop);
	EZ_NOTUSED(clientData);
	log_info("timeId %li fired, next go.", timeId);

	return AE_TIMER_NEXT;
}

void init_boss()
{
	boss.id = 0;
	boss.thid = pthread_self();
	boss.w_event = ez_create_event_loop(128);
}

void run_boss()
{
	log_info("main > run event loop .");
	ez_run_event_loop(boss.w_event);
}

void stop_free_boss()
{
	log_info("main > stop event loop.");
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

	int s = ez_net_tcp_server(port, "0.0.0.0", 1024); 	// 监听的SRC := 0.0.0.0
	int s6 = ez_net_tcp6_server(port, "::", 1024);		// 监听的SRC := ::
	log_info("server %d at port %d wait client ...", s, port);
	log_info("server %d at port %d wait client ...", s6, port);

	init_boss();
	init_workers();

	// add s accept
	ez_create_file_event(boss.w_event, s, AE_READABLE, accept_handler, NULL);
	ez_create_file_event(boss.w_event, s6, AE_READABLE, accept_handler, NULL);
	// test time out.
	ez_create_time_event(boss.w_event, 5000, time_out_handler, NULL);

	run_boss();
	stop_free_boss();
	stop_free_workers();

	ez_net_close_socket(s);
	ez_net_close_socket(s6);
	log_release();
	return 0;
}
