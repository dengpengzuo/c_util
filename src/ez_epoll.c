#include <sys/epoll.h>

typedef struct ezApiState {
	int epfd;
	struct epoll_event *events;
} ezApiState;

static int ezApiCreate(ezEventLoop * eventLoop)
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
	eventLoop->apidata = state;
	return AE_OK;
}

static void ezApiDelete(ezEventLoop * eventLoop)
{
	ezApiState *state = eventLoop->apidata;

	close(state->epfd);

	ez_free(state->events);
	ez_free(state);
}

static int ezApiAddEvent(ezEventLoop * eventLoop, int fd, int mask)
{
	ezApiState *state = eventLoop->apidata;
	struct epoll_event ee;
	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	int op = eventLoop->events[fd].mask == AE_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= eventLoop->events[fd].mask;	/* Merge old events */
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

static void ezApiDelEvent(ezEventLoop * eventLoop, int fd, int delmask)
{
	ezApiState *state = eventLoop->apidata;
	struct epoll_event ee;
	int mask = eventLoop->events[fd].mask & (~delmask);

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

static int ezApiPoll(ezEventLoop * eventLoop, int timeout)
{
	ezApiState *state = eventLoop->apidata;
	int retval, numevents = 0;
	if (timeout < 0)
		timeout = -1;
	retval = epoll_wait(state->epfd, state->events, eventLoop->setsize, timeout);
	if (retval > 0) {
		int j;

		numevents = retval;
		for (j = 0; j < numevents; j++) {
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

			eventLoop->fired[j].fd = e->data.fd;
			eventLoop->fired[j].mask = mask;
		}
	}
	return numevents;
}
