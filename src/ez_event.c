#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ez_macro.h"
#include "ez_log.h"
#include "ez_malloc.h"
#include "ez_util.h"
#include "ez_rbtree.h"
#include "ez_list.h"

#include "ez_event.h"

/* File event structure */
typedef struct ez_file_event_s {
	int fd;
	int mask;				/* one of AE_(READABLE|WRITABLE) */
	ezFileProc rfileProc;
	ezFileProc wfileProc;
	void *clientData;

	ez_rbtree_node rb_node;	/* rbtree node */
} ez_file_event_t;

static inline ez_file_event_t *cast_to_file_event(ez_rbtree_node * node)
{
	return EZ_CONTAINER_OF(node, ez_file_event_t, rb_node);
}

/* Time event structure */
typedef struct ez_time_event_s {
	int64_t id;		/* time event identifier. */
	int64_t period;		/* milliseconds */
	int64_t when_ms;	/* Firing milliseconds */
	ezTimeProc timeProc;
	void *clientData;

	list_head_t listNode;	/* time events list node */
} ez_time_event_t;

static inline ez_time_event_t *cast_to_time_event(list_head_t * node)
{
	return EZ_CONTAINER_OF(node, ez_time_event_t, listNode);
}

/* A fired event */
typedef struct ez_fired_event_s {
	int fd;
	int mask;
} ez_fired_event;

/* State of an event based program */
struct ez_event_loop_s {
	int stop;
	void *apidata;	/* This is used for event polling API specific data */

	int setsize;	/* max number of file descriptors tracked */
	int count;		/* add file event count */

	ez_rbtree_t rbtree_events;			/* rbtree file events */
	ez_rbtree_node rbnode_sentinel;	/* rbtree sentinel node */
	ez_rbtree_node *free_events;		/* free rbtree node linked list (node->parent) */

	time_t lastTime;				/* Used to detect system clock skew */
	int64_t timeNextId;
	list_head_t time_events;			/* 按照时间长短和ID进行比较放入的队列 */

	ez_fired_event *fired;			/* Fired events */
};

#if defined(__linux__)
    #include "ez_epoll.c"
#else
    #error "not support os!"
#endif

static ez_file_event_t *ez_fund_file_event(ezEventLoop_t * eventLoop, int fd);

static inline void put_to_free_file_events(ezEventLoop_t * eventLoop, ez_file_event_t * e)
{
	if (e == NULL)
		return;
	e->rb_node.parent = eventLoop->free_events;
	eventLoop->free_events = &(e->rb_node);
}

static int ez_process_events(ezEventLoop_t * eventLoop, int flags);

static int file_event_compare_proc(ez_rbtree_node * newNode, ez_rbtree_node * existNode)
{
	ez_file_event_t *newEvent = cast_to_file_event(newNode);
	ez_file_event_t *existEvent = cast_to_file_event(existNode);
	return newEvent->fd > existEvent->fd ? 1 : (newEvent->fd < existEvent->fd ? -1 : 0);
}

static int file_event_find_compare_proc(ez_rbtree_node * node, void *find_args)
{
	int fd = *((int *)find_args);
	ez_file_event_t *fe = cast_to_file_event(node);
	return fe->fd == fd ? 0 : (fe->fd > fd ? 1 : -1);
}

static ez_file_event_t *ez_fund_file_event(ezEventLoop_t * eventLoop, int fd)
{
	ez_rbtree_node *n =
	    rbtree_find_node(&eventLoop->rbtree_events, file_event_find_compare_proc, (void *)&fd);
	return n == NULL ? NULL : cast_to_file_event(n);
}

ezEventLoop_t *ez_create_event_loop(int setsize)
{
	ezEventLoop_t *eventLoop = NULL;

	eventLoop = (ezEventLoop_t *) ez_malloc(sizeof(ezEventLoop_t));
	if (!eventLoop)
		goto err;

	rbtree_init(&eventLoop->rbtree_events, &eventLoop->rbnode_sentinel,
		    file_event_compare_proc);
	eventLoop->free_events = NULL;

	eventLoop->fired = (ez_fired_event *) ez_malloc(sizeof(ez_fired_event) * setsize);
	if (eventLoop->fired == NULL)
		goto err;

	init_list_head(&eventLoop->time_events);

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
		ez_free(eventLoop);
	}
	return NULL;
}

void ez_delete_event_loop(ezEventLoop_t * eventLoop)
{
	ez_rbtree_node *i;
	ez_file_event_t *e;
	ez_time_event_t *t;
	if (!eventLoop)
		return;
	ezApiDelete(eventLoop);

	// free ez_file_event_t.
	for (i = eventLoop->free_events; i != NULL;) {
		e = cast_to_file_event(i);
		i = i->parent;
		ez_free(e);
	}
	while ((i = rbtree_min_node(&eventLoop->rbtree_events)) != NULL) {
		rbtree_delete(&eventLoop->rbtree_events, i);
		e = cast_to_file_event(i);
		ez_free(e);
	}

	ez_free(eventLoop->fired);

	LIST_FOR(&(eventLoop->time_events), ti) {
		list_del(ti);

		t = cast_to_time_event(ti);
		log_debug("delete time event [id:%li].", t->id);
		ez_free(t);
	}

	ez_free(eventLoop);
}

void ez_stop_event_loop(ezEventLoop_t * eventLoop)
{
	if (!eventLoop)
		return;
	ezApiStop(eventLoop);
}

int ez_create_file_event(ezEventLoop_t * eventLoop, int fd, EVENT_MASK mask, ezFileProc proc, void *clientData)
{
	if (mask == AE_NONE) return AE_ERR;

	ez_file_event_t *fe = ez_fund_file_event(eventLoop, fd);
	int oldmask;
	int is_new = 0;

	if (fe == NULL) {
		if (eventLoop->count >= eventLoop->setsize) {
			log_error("event loop create file event count's over setsize:%d !", eventLoop->setsize);
			return AE_ERR;
		}
		// 从回收链接表中取
		if (eventLoop->free_events != NULL) {
			fe = cast_to_file_event(eventLoop->free_events);
			eventLoop->free_events = eventLoop->free_events->parent;
			memset(fe, 0, sizeof(ez_file_event_t));
		} else {
			fe = ez_malloc(sizeof(ez_file_event_t));
		}

		is_new = 1;
		oldmask = AE_NONE;

		fe->fd = fd;
		fe->mask = mask;
		fe->rfileProc = proc;
		fe->clientData = clientData;
	} else {
		is_new = 0;
		oldmask = fe->mask;
	}

	if (ezApiAddEvent(eventLoop, fd, (int)mask, oldmask) == -1) {
		if(is_new) ez_free(fe);
		return AE_ERR;
	}
    
    if (is_new) {
        ++(eventLoop->count);
        rbtree_insert(&eventLoop->rbtree_events, &(fe->rb_node));
    } else {
        // update file event's properties [mask|proc|clientData].
        fe->mask |= mask;

		if (mask == AE_READABLE)
			fe->rfileProc = proc;
		else if (mask == AE_WRITABLE)
			fe->wfileProc = proc;

		if (clientData != NULL && fe->clientData != clientData) {
            log_warn("file fd:%d add new mask  event's proc args not same!", fe->fd);
            fe->clientData = clientData;
        }
    }

    return AE_OK;
}

void ez_delete_file_event(ezEventLoop_t * eventLoop, int fd, EVENT_MASK mask)
{
	if (fd >= eventLoop->setsize)
		return;
	ez_file_event_t *fe = ez_fund_file_event(eventLoop, fd);
	if (fe == NULL || fe->mask == AE_NONE)
		return;

	ezApiDelEvent(eventLoop, fd, (int) mask, fe->mask);
	// 取反留下其他的mask
	fe->mask = fe->mask & (~(int)mask);
	if (fe->mask == AE_NONE) {
		rbtree_delete(&eventLoop->rbtree_events, &fe->rb_node);
		--(eventLoop->count);
		put_to_free_file_events(eventLoop, fe);
	}
}

static void insert_time_event_list(list_head_t *first, list_head_t *end, ez_time_event_t *te) {
	// 将 when_ms 大的放后, when_ms 小的放前
	list_head_t *ti ;
	for (ti = first; ti != end;) {
		ez_time_event_t *i = cast_to_time_event(ti);
		if (te->when_ms > i->when_ms) {
			ti = ti->next;
		} else {
			break;
		}
	}
	list_add(&te->listNode, ti->prev); // add newNode at ti->prev after.
}
/**
 * eventLoop    事件loop
 * milliseconds 启动时间
 */
int64_t ez_create_time_event(ezEventLoop_t * eventLoop, int64_t period, ezTimeProc proc, void *clientData)
{
	int64_t id = eventLoop->timeNextId++;
	ez_time_event_t *te = ez_malloc(sizeof(*te));
	if (te == NULL)
		return AE_ERR;

	te->id = id;
	te->timeProc = proc;
	te->clientData = clientData;
	te->period = period;
	te->when_ms = mstime() + te->period;
	log_debug("create time event [id:%li, when_ms:%li].", id, te->when_ms);

	insert_time_event_list(eventLoop->time_events.next, &eventLoop->time_events, te);
	return id;
}

void ez_delete_time_event(ezEventLoop_t * eventLoop, int64_t id)
{
    ez_time_event_t *te = NULL;
	LIST_FOR(&(eventLoop->time_events), pos) {
		te = cast_to_time_event(pos);
		if (te->id == id) {
			log_debug("delete time event [id:%li].", id);
			list_del(pos);
			ez_free(te);
			return;
		}
	}
}

#define AE_FILE_EVENTS  1
#define AE_TIME_EVENTS  2
#define AE_ALL_EVENTS   (AE_FILE_EVENTS | AE_TIME_EVENTS)

void ez_run_event_loop(ezEventLoop_t * eventLoop)
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
static int process_time_events(ezEventLoop_t * eventLoop)
{
	int processed = 0;
	int all_fired = 0;
	list_head_t *re_put = NULL;
	int64_t now_ms;
    ez_time_event_t *te = NULL;
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

	LIST_FOR(&(eventLoop->time_events), tmp){
		te = cast_to_time_event(tmp);
		now_ms = mstime();

		if (all_fired || now_ms >= te->when_ms) {
			list_del(tmp);

			log_debug("call time event [id:%li]", te->id);
			int ret_val = te->timeProc(eventLoop, te->id, te->clientData);
			processed++;

			if (ret_val <= AE_TIMER_END) {
				log_debug("delete one time event [id:%li]", te->id);
				ez_free(te);
			} else {
				if (ret_val == AE_TIMER_NEXT)
					te->when_ms = now_ms + te->period;
				else
    				te->when_ms = now_ms + ret_val;
                // 重新入队的加入reput链表
                te->listNode.next = re_put;
                re_put = &te->listNode;
			}
		} else {
			// 没得比这个时间小的了，停止处理timeEvent.
			break;
		}
	}

    // 重新入队
	while (re_put != NULL) {
        te = cast_to_time_event(re_put);
		// 插入前先转到下一个指针[因为insert_list_head会对next.prev进行操作]
		re_put = re_put->next;

		log_debug("reput time event [id:%li]", te->id);
		insert_time_event_list(eventLoop->time_events.next, &eventLoop->time_events, te);
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
static int ez_process_events(ezEventLoop_t * eventLoop, int flags)
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
		ez_time_event_t *shortest = NULL;
		int j, tvp;
		if (flags & AE_TIME_EVENTS) {
			shortest = NULL;
			// 第一个timeEvent就是最少的wait time.
			if (!list_is_empty(&eventLoop->time_events)) {
				shortest = cast_to_time_event(eventLoop->time_events.next);
				log_debug("find min wait time event [id:%li]", shortest->id);
			}
		}
		if (shortest != NULL) {
			tvp = (int) (shortest->when_ms - mstime());
			if (tvp < 0) tvp = 100;
		} else {
			tvp = -1;	// wait for block
		}

		numevents = ezApiPoll(eventLoop, tvp);
		for (j = 0; j < numevents; j++) {
			int fd = eventLoop->fired[j].fd;
			int fired_mask = eventLoop->fired[j].mask;
			ez_file_event_t *fe = ez_fund_file_event(eventLoop, fd);

            if ((fired_mask & AE_READABLE) == AE_READABLE && fe != NULL && fe->mask != AE_NONE) {
                fe->rfileProc(eventLoop, fd, fe->clientData, AE_READABLE);
            }
            if ((fired_mask & AE_WRITABLE) == AE_WRITABLE && fe != NULL && fe->mask != AE_NONE) {
                fe->wfileProc(eventLoop, fd, fe->clientData, AE_WRITABLE);
            }
			processed++;
		}
	}
	/* Check time events */
	if (flags & AE_TIME_EVENTS)
		processed += process_time_events(eventLoop);

	return processed;	/* return the number of processed file/time events */
}
