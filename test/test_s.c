
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

#define 	WORKER_SIZE			4
static ezWorker workers[WORKER_SIZE];
static int last_worker = 0;

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

void init_ez_workers()
{
	pthread_t thid;
	for (int i = 0; i < WORKER_SIZE; ++i) {
		workers[i].id = i;

		log_info("create worker id: %d event loop .", workers[i].id);
		workers[i].w_event = ez_create_event_loop(1024);

		pthread_create(&thid, NULL, ez_worker_thread_run, &workers[i]);
	}
}

void stop_ez_workers()
{
	for (int i = 0; i < WORKER_SIZE; ++i) {
		log_info("send worker id: %d exit event loop .", workers[i].id);
		ez_stop(workers[i].w_event);
	}
	for (int i = 0; i < WORKER_SIZE; ++i) {
		log_info("wait worker id: %d thread exit.", workers[i].id);
		pthread_join(workers[i].thid, NULL);

		log_info("delete worker id: %d event loop .", workers[i].id);
		ez_delete_event_loop(workers[i].w_event);
	}
}

void accept_handler(ezEventLoop * eventLoop, int s, void *clientData, int mask)
{
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

void read_char_from_stdin(ezEventLoop * eventLoop, int fd, void *clientData, int mask)
{
	EZ_NOTUSED(clientData);
	EZ_NOTUSED(mask);

	char buf[33];
	ssize_t nread = read(fd, buf, 32);
	if (nread > 0) {
		buf[nread] = '\0';
		if (buf[0] == 'q' || buf[0] == 'Q') {
			log_info("good bye!");
			ez_delete_file_event(eventLoop, fd, AE_READABLE);
			ez_stop(eventLoop);
		}
	}
}

int main(int argc, char **argv)
{
	int port = 9090;
	EZ_NOTUSED(argc);
	EZ_NOTUSED(argv);

	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	log_init(LOG_INFO, NULL);

	int s = ez_net_tcp_server(port, NULL, 1024);
	log_info("server %d at port %d wait client ...", s, port);

	int s6 = ez_net_tcp6_server(port, NULL, 1024);
	log_info("server %d at port %d wait client ...", s6, port);

	log_info("create main event loop...");
	ezEventLoop *main_event = ez_create_event_loop(128);
	ez_create_file_event(main_event, s, AE_READABLE, accept_handler, NULL);
	ez_create_file_event(main_event, s6, AE_READABLE, accept_handler, NULL);
	ez_create_time_event(main_event, 1000, time_out_handler, NULL);
	ez_create_file_event(main_event, STDIN_FILENO, AE_READABLE, read_char_from_stdin, NULL);
	// 初始化 worker thread.
	init_ez_workers();

	ez_run_event_loop(main_event);
	log_info("delete main event loop");
	ez_delete_event_loop(main_event);

	// 停止 worker thread.
	stop_ez_workers();

	ez_net_close_socket(s);
	ez_net_close_socket(s6);

	log_release();
	return 0;
}
