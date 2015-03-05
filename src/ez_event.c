#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ez_rbtree.h>

#include "ez_malloc.h"
#include "ez_util.h"
#include "ez_min_heap.h"
#include "ez_log.h"

#include "ez_event.h"

/* File event structure */
typedef struct ezFileEvent_t {
	int fd ;
	int mask;		/* one of AE_(READABLE|WRITABLE) */
	ezFileProc rfileProc;
	ezFileProc wfileProc;
	void *clientData;
	ezRBTreeNode rb_node; /* rbtree node */
} ezFileEvent;

static inline ezFileEvent * cast_to(ezRBTreeNode *node)
{
	return ez_container_of(node, ezFileEvent, rb_node);
}

/* Time event structure */
typedef struct ezTimeEvent_t {
	int64_t id;		/* time event identifier. */
	int64_t period;		/* milliseconds */
	int64_t when_ms;	/* Firing milliseconds */
	ezTimeProc timeProc;
	void *clientData;
} ezTimeEvent;

/* A fired event */
typedef struct ezFiredEvent_t {
	int fd;
	int mask;
} ezFiredEvent;

/* State of an event based program */
struct ezEventLoop_t {
	int stop;
	void *apidata;					/* This is used for event polling API specific data */

	int setsize;					/* max number of file descriptors tracked */
	int count;
	ezRBTree rbtree_events;
	ezRBTreeNode rbnode_sentinel;
	ezRBTreeNode *free_events;		/* free linked list */

	// time out event fields.
	time_t lastTime;				/* Used to detect system clock skew */
	int64_t timeNextId;
	ezMinHeap timeEventMinHeap;		/* Registered time events */

	ezFiredEvent *fired;			/* Fired events */
};

// linux epoll code
#include "ez_epoll.c"

static ezFileEvent *ez_fund_file_event(ezEventLoop * eventLoop, int fd);

static inline void put_to_free_file_events(ezEventLoop * eventLoop, ezFileEvent *e)
{
	if(e == NULL) return;
	e->rb_node.parent = eventLoop->free_events;
	eventLoop->free_events = &(e->rb_node);
}

static int ez_process_events(ezEventLoop * eventLoop, int flags);

static int ez_time_event_compare_proc(void *p, void *n)
{
	ezTimeEvent *pte = (ezTimeEvent *) p;
	ezTimeEvent *nte = (ezTimeEvent *) n;
	return (pte->when_ms > nte->when_ms) ? 1 : 0;
}

static void ez_time_event_free_proc(void *p)
{
	ezTimeEvent *pte = (ezTimeEvent *) p;
	log_debug("delete time event [id:%li].", pte->id);
	ez_free(pte);
}

static int file_event_compare_proc(ezRBTreeNode *newNode, ezRBTreeNode *existNode)
{
	ezFileEvent * newEvent = cast_to(newNode);
	ezFileEvent * existEvent = cast_to(existNode);
	return newEvent->fd > existEvent->fd ? 1 : (newEvent->fd < existEvent->fd ? -1 : 0);
}

static int file_event_find_compare_proc(ezRBTreeNode *node, void *find_args)
{
	int fd = *((int *) find_args);
	ezFileEvent *fe = cast_to(node);
	return fe->fd == fd ? 0 : (fe->fd > fd ? 1 : -1);
}

static ezFileEvent *ez_fund_file_event(ezEventLoop *eventLoop, int fd)
{
	ezRBTreeNode *n = rbtree_find_node(&eventLoop->rbtree_events, file_event_find_compare_proc, (void *) &fd);
	return n == NULL ? NULL : cast_to(n);
}

ezEventLoop *ez_create_event_loop(int setsize)
{
	ezEventLoop *eventLoop = NULL;

	eventLoop = (ezEventLoop *) ez_malloc(sizeof(ezEventLoop));
	if (!eventLoop)
		goto err;

	rbtree_init(&eventLoop->rbtree_events, &eventLoop->rbnode_sentinel, file_event_compare_proc);
	eventLoop->free_events = NULL;

	eventLoop->fired = (ezFiredEvent *) ez_malloc(sizeof(ezFiredEvent) * setsize);
	if (eventLoop->fired == NULL)
		goto err;

	ez_min_heap_init(&eventLoop->timeEventMinHeap, ez_time_event_compare_proc, ez_time_event_free_proc);

	eventLoop->setsize = setsize;
	eventLoop->lastTime = time(NULL);
	eventLoop->timeNextId = 0;
	eventLoop->stop = 0;

	if (ezApiCreate(eventLoop) != AE_OK)
		goto err;

	return eventLoop;
 err:
	if (eventLoop) {
		ez_free(eventLoop->fired);
		ez_min_heap_free(&eventLoop->timeEventMinHeap);
		ez_free(eventLoop);
	}
	return NULL;
}

void ez_delete_event_loop(ezEventLoop * eventLoop)
{
	ezRBTreeNode * i;
	ezFileEvent  * e;
	if (!eventLoop)
		return;
	ezApiDelete(eventLoop);

	// free ezFileEvent.
	for (i = eventLoop->free_events; i != NULL;) {
		e = cast_to(i);
		i = i->parent;
		ez_free(e);
	}
	while ((i = rbtree_min_node(&eventLoop->rbtree_events)) != NULL) {
		rbtree_delete(&eventLoop->rbtree_events, i);
		e = cast_to(i);
		ez_free(e);
	}

	ez_free(eventLoop->fired);
	ez_min_heap_free(&eventLoop->timeEventMinHeap);
	ez_free(eventLoop);
}

void ez_stop_event_loop(ezEventLoop *eventLoop)
{
	if (!eventLoop)
		return;
	ezApiStop(eventLoop);
}

int ez_create_file_event(ezEventLoop * eventLoop, int fd, int mask, ezFileProc proc, void *clientData)
{
	ezFileEvent *fe = ez_fund_file_event(eventLoop, fd);
	if (fe == NULL) {
		if (eventLoop->count >= eventLoop->setsize) {
			log_error("event loop create file event count's over setsize:%d !", eventLoop->setsize);
			return AE_ERR;
		}
		// 从回收链接表中取
		if (eventLoop->free_events != NULL) {
			fe = cast_to(eventLoop->free_events);
			eventLoop->free_events = eventLoop->free_events->parent;
			memset(fe, 0, sizeof(ezFileEvent));
		} else {
			fe = ez_malloc(sizeof(ezFileEvent));
		}
		fe->fd = fd;
		fe->mask = AE_NONE;
		fe->clientData = clientData;
	}

	if (ezApiAddEvent(eventLoop, fd, mask, fe->mask) == -1) {
        // 加入到回收链表头
		ez_free(fe);
		return AE_ERR;
	}

	fe->mask |= mask;
	if (mask & AE_READABLE)
		fe->rfileProc = proc;
	if (mask & AE_WRITABLE)
		fe->wfileProc = proc;

	rbtree_insert(&eventLoop->rbtree_events, &(fe->rb_node));
	return AE_OK;
}

void ez_delete_file_event(ezEventLoop * eventLoop, int fd, int mask)
{
	if (fd >= eventLoop->setsize)
		return;
	ezFileEvent *fe = ez_fund_file_event(eventLoop, fd);
	if (fe == NULL || fe->mask == AE_NONE)
		return;

	ezApiDelEvent(eventLoop, fd, mask, fe->mask);
	// 取反留下其他的mask
	fe->mask = fe->mask & (~mask);
	if (fe->mask == AE_NONE) {
		rbtree_delete(&eventLoop->rbtree_events, &fe->rb_node);
		put_to_free_file_events(eventLoop, fe);
	}
}

/**
 * eventLoop    事件loop
 * milliseconds 启动时间
 */
int64_t ez_create_time_event(ezEventLoop * eventLoop, int64_t period, ezTimeProc proc, void *clientData)
{
	int64_t id = eventLoop->timeNextId++;
	ezTimeEvent *te = ez_malloc(sizeof(*te));
	if (te == NULL)
		return AE_ERR;

	te->id = id;
	te->timeProc = proc;
	te->clientData = clientData;
	te->period = period;
	te->when_ms = ez_get_cur_milliseconds() + te->period;

	int r = ez_min_heap_push(&eventLoop->timeEventMinHeap, (void *)te);
	if (r != 0) {
		ez_free(te);
		log_error("push time event [id:%li] to min_heap failed!", id);
		return AE_ERR;
	}
	return id;
}

static int ez_time_event_find_cmp(void *data, void *args)
{
	ezTimeEvent *te = (ezTimeEvent *) data;
	int64_t *find_time_id = (int64_t *) args;

	return (*find_time_id == te->id) ? 1 : 0;
}

void ez_delete_time_event(ezEventLoop * eventLoop, int64_t id)
{
	int index = ez_min_heap_find(&eventLoop->timeEventMinHeap, ez_time_event_find_cmp, &id);
	if (index == MIN_HEAP_NOT_FUND) {
		log_error("delete time event [id:%li] not find!", id);
		return;
	}
	log_debug("delete time event [id:%li].", id);
	ezTimeEvent *te = (ezTimeEvent *) ez_min_heap_delete(&eventLoop->timeEventMinHeap, index);
	ez_free(te);
}

#define AE_FILE_EVENTS  1
#define AE_TIME_EVENTS  2
#define AE_ALL_EVENTS   (AE_FILE_EVENTS | AE_TIME_EVENTS)

void ez_run_event_loop(ezEventLoop * eventLoop)
{
	if (!eventLoop)
		return;

	ezApiBeforePoll(eventLoop);
	while (!eventLoop->stop) {
		ez_process_events(eventLoop, AE_ALL_EVENTS);
	}
	ezApiAfterPoll(eventLoop);
}

/* Process time events */
static int process_time_events(ezEventLoop * eventLoop)
{
	#define REPUT_ARRAY_SIZE 64

	int processed = 0;
	int re_put_index = 0, over_size = 0;
	int put_min_result = 0, all_fired = 0;
	ezTimeEvent *te = NULL;
	ezTimeEvent *rePutTimeEvents[REPUT_ARRAY_SIZE];
	int64_t now_ms;
	/* If the system clock is moved to the future, and then set back to the
	 * right value, time events may be delayed in a random way. Often this
	 * means that scheduled operations will not be performed soon enough.
	 *
	 * Here we try to detect system clock skews, and force all the time
	 * events to be processed ASAP when this happens: the idea is that
	 * processing events earlier is less dangerous than delaying them
	 * indefinitely, and practice suggests it is. */
	time_t now = time(NULL);
	if (now < eventLoop->lastTime) {
		// 时间被调整得小于了上次启发时间点，直接要求全部都启发一次.
		all_fired = 1;
	}
	eventLoop->lastTime = now;
	memset(rePutTimeEvents, 0, REPUT_ARRAY_SIZE * sizeof(ezTimeEvent *));
	do {
		now_ms = ez_get_cur_milliseconds();
		if (!all_fired)
			te = (ezTimeEvent *) ez_min_heap_min(&eventLoop->timeEventMinHeap);

		if (te == NULL)
			break;
		if (all_fired || now_ms > te->when_ms) {
			// 弹出最小值.
			te = (ezTimeEvent *) ez_min_heap_pop(&eventLoop->timeEventMinHeap);
			log_debug("pop time event [id:%li].", te->id);
			int retval = te->timeProc(eventLoop, te->id, te->clientData);
			processed++;

			if (retval > AE_TIMER_END) {
				// 固定step period.
				if (retval == AE_TIMER_NEXT)
					te->when_ms = now_ms + te->period;
				else
					te->when_ms = now_ms + retval;

				if (re_put_index >= REPUT_ARRAY_SIZE) {
					over_size = 1;
					re_put_index = 0;
				}
				// 将原来位置的入min_heap中.
				if (rePutTimeEvents[re_put_index] != NULL) {
					put_min_result = ez_min_heap_push(&eventLoop->timeEventMinHeap, rePutTimeEvents[re_put_index]);
					if (put_min_result != 0) {
						log_error("push time event [id:%li] to min_heap failed!", rePutTimeEvents[re_put_index]->id);
					}
				}
				// 将新的加入到这个数组中.
				rePutTimeEvents[re_put_index++] = te;
			} else {
				log_debug("time event [id:%li] return AE_TIMER_END, delete it.", te->id);
				ez_free(te);
			}
		} else {
			break;	// 没有小于当前时间的time_event.
		}
	} while (true);

	// re put to min_heap.
	for (int i = 0; i < (over_size == 1 ? REPUT_ARRAY_SIZE : re_put_index); ++i) {
		put_min_result = ez_min_heap_push(&eventLoop->timeEventMinHeap, rePutTimeEvents[i]);

		if (put_min_result != 0) {
			log_error("push time event [id:%li] to min_heap failed!", rePutTimeEvents[i]->id);
		}
		rePutTimeEvents[i] = NULL;
	}

	return processed;
}

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
static int ez_process_events(ezEventLoop * eventLoop, int flags)
{
	int processed = 0, numevents;

	/* Nothing to do? return ASAP */
	if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS))
		return 0;

    /**
     * Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire.
     */
	if (flags & AE_FILE_EVENTS) {
		ezTimeEvent *shortest = NULL;
		int j, tvp;
		if (flags & AE_TIME_EVENTS) {
			shortest = (ezTimeEvent *) ez_min_heap_min(&eventLoop->timeEventMinHeap);
		}
		if (shortest != NULL) {
			tvp = (int)(shortest->when_ms - ez_get_cur_milliseconds());
		} else {
			tvp = -1; // wait for block
		}

		numevents = ezApiPoll(eventLoop, tvp);
		for (j = 0; j < numevents; j++) {
			int fired_mask = eventLoop->fired[j].mask;
			int fd = eventLoop->fired[j].fd;
			ezFileEvent *fe = ez_fund_file_event(eventLoop, fd);

			if (fe->mask & fired_mask & AE_READABLE) {
				fe->rfileProc(eventLoop, fd, fe->clientData, fired_mask);
			}
			if (fe->mask & fired_mask & AE_WRITABLE) {
				fe->wfileProc(eventLoop, fd, fe->clientData, fired_mask);
			}
			processed++;
		}
	}
	/* Check time events */
	if (flags & AE_TIME_EVENTS)
		processed += process_time_events(eventLoop);

	return processed;	/* return the number of processed file/time events */
}
