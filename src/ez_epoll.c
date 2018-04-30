#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

typedef struct ezApiState {
	int epfd;
	int evfd;
	struct epoll_event *events;
} ezApiState;

static eventfd_t _STOP_CMD = 0xFF;

static void ezApiDoEventfdCmd(ez_event_loop_t * eventLoop)
{
	eventfd_t cmd = 0;
	ezApiState *state = (ezApiState *) eventLoop->apidata;

	eventfd_read(state->evfd, &cmd);
	if (cmd == _STOP_CMD) {
		eventLoop->stop = 1;
	}
}

static int ezApiCreate(ez_event_loop_t * eventLoop)
{
	ezApiState *state = (ezApiState *) ez_malloc(sizeof(ezApiState));

	if (!state)
		return AE_ERR;
	state->events =
	    (struct epoll_event *)ez_malloc(sizeof(struct epoll_event) * eventLoop->setsize);
	if (!state->events) {
		ez_free(state);
		return AE_ERR;
	}
	state->epfd = epoll_create(1024);	/* 1024 is just a hint for the kernel */
	if (state->epfd == -1) {
		ez_free(state->events);
		ez_free(state);
		return AE_ERR;
	}

	state->evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (state->evfd == -1) {
		ez_free(state->events);
		ez_free(state);
		return AE_ERR;
	}

	eventLoop->apidata = state;
	return AE_OK;
}

static void ezApiDelete(ez_event_loop_t * eventLoop)
{
	ezApiState *state = eventLoop->apidata;

	close(state->epfd);
	close(state->evfd);

	ez_free(state->events);
	ez_free(state);
}

static int ezApiAddEvent(ez_event_loop_t * eventLoop, int fd, int mask, int old_mask)
{
	ezApiState *state = eventLoop->apidata;
	struct epoll_event ee;
	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	int op = (old_mask == AE_NONE) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= old_mask;	/* Merge old events */
	if (mask & AE_READABLE)
		ee.events |= EPOLLIN;
	if (mask & AE_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.u64 = 0;	/* avoid valgrind warning */
	ee.data.fd = fd;
	if (epoll_ctl(state->epfd, op, fd, &ee) == -1)
		return -1;
	return AE_OK;
}

static void ezApiDelEvent(ez_event_loop_t * eventLoop, int fd, int delmask, int oldmask)
{
	ezApiState *state = eventLoop->apidata;
	struct epoll_event ee;
	int mask = oldmask & (~delmask);

	ee.events = 0;
	if (mask & AE_READABLE)
		ee.events |= EPOLLIN;
	if (mask & AE_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.u64 = 0;	/* avoid valgrind warning */
	ee.data.fd = fd;
	if (mask != AE_NONE) {
		epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
	} else {
		/* Note, Kernel < 2.6.9 requires a non null event pointer even for
		 * EPOLL_CTL_DEL. */
		epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
	}
}

static void ezApiStop(ez_event_loop_t * eventLoop)
{
	ezApiState *state = eventLoop->apidata;
	eventfd_write(state->evfd, _STOP_CMD);
}

static void ezApiBeforePoll(ez_event_loop_t * eventLoop)
{
	ezApiState *state = eventLoop->apidata;
	ezApiAddEvent(eventLoop, state->evfd, AE_READABLE, AE_NONE);
}

static void ezApiAfterPoll(ez_event_loop_t * eventLoop)
{
	ezApiState *state = eventLoop->apidata;
	ezApiDelEvent(eventLoop, state->evfd, AE_READABLE, AE_READABLE);
}

static int ezApiPoll(ez_event_loop_t * eventLoop, int timeout)
{
	ezApiState *state = eventLoop->apidata;
	int retval, numevents = 0;
	int err;
	if (timeout < 0)
		timeout = -1;

	do {
        retval = epoll_wait(state->epfd, state->events, eventLoop->setsize, timeout);
        // was interrupted try again.
    } while (retval == -1 && ((err = errno) == EINTR));

	if (retval > 0) {
		int j, i;

		numevents = retval;
		i = 0;
		for (j = 0; j < retval; j++) {
			struct epoll_event *e = state->events + j;
			int what = e->events;

			/* nginx 处理方式 */
			if ((what & (EPOLLERR | EPOLLHUP)) && (what & (EPOLLIN | EPOLLOUT)) == 0) {
				/*
				 * if the error events were returned without EPOLLIN or EPOLLOUT,
				 * then add these flags to handle the events at least in one
				 * active handler
				 */
				what |= EPOLLIN | EPOLLOUT;
			}
			/* libevent 处理方式 */
			//if (what & (EPOLLERR|EPOLLHUP)) {
			//      mask = AE_READABLE | AE_WRITABLE;
			//}

			int mask = 0;
			if (what & EPOLLIN)
				mask |= AE_READABLE;
			if (what & EPOLLOUT)
				mask |= AE_WRITABLE;

			if (!mask)
				continue;

			// do inner event fd command.
			if (e->data.fd == state->evfd) {
				ezApiDoEventfdCmd(eventLoop);
				--numevents;	// 却掉eventfd数.
				continue;
			}

			eventLoop->fired[i].fd = e->data.fd;
			eventLoop->fired[i].mask = mask;
			++i;
		}
	}
	return numevents;
}
