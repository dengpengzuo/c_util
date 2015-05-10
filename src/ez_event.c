#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ez_log.h"
#include "ez_malloc.h"
#include "ez_util.h"
#include "ez_rbtree.h"
#include "ez_list.h"

#include "ez_event.h"

/* File event structure */
typedef struct ezFileEvent_t {
	int fd;
	int mask;
	/* one of AE_(READABLE|WRITABLE) */
	ezFileProc rfileProc;
	void *clientData;

	ezRBTreeNode rb_node;	/* rbtree node */
} ezFileEvent;

static inline ezFileEvent *cast_to_file_event(ezRBTreeNode * node)
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

	list_head listNode;	/* time events list node */
} ezTimeEvent;

static inline ezTimeEvent *cast_to_time_event(list_head * node)
{
	return ez_container_of(node, ezTimeEvent, listNode);
}

/* A fired event */
typedef struct ezFiredEvent_t {
	int fd;
	int mask;
} ezFiredEvent;

/* State of an event based program */
struct ezEventLoop_t {
	int stop;
	void *apidata;	/* This is used for event polling API specific data */

	int setsize;	/* max number of file descriptors tracked */
	int count;		/* add file event count */

	ezRBTree rbtree_events;			/* rbtree file events */
	ezRBTreeNode rbnode_sentinel;	/* rbtree sentinel node */
	ezRBTreeNode *free_events;		/* free rbtree node linked list (node->parent) */

	time_t lastTime;				/* Used to detect system clock skew */
	int64_t timeNextId;
	list_head time_events;			/* 按照时间长短和ID进行比较放入的队列 */

	ezFiredEvent *fired;			/* Fired events */
};

// linux epoll code
#include "ez_epoll.c"

static ezFileEvent *ez_fund_file_event(ezEventLoop * eventLoop, int fd);

static inline void put_to_free_file_events(ezEventLoop * eventLoop, ezFileEvent * e)
{
	if (e == NULL)
		return;
	e->rb_node.parent = eventLoop->free_events;
	eventLoop->free_events = &(e->rb_node);
}

static int ez_process_events(ezEventLoop * eventLoop, int flags);

static int file_event_compare_proc(ezRBTreeNode * newNode, ezRBTreeNode * existNode)
{
	ezFileEvent *newEvent = cast_to_file_event(newNode);
	ezFileEvent *existEvent = cast_to_file_event(existNode);
	return newEvent->fd > existEvent->fd ? 1 : (newEvent->fd < existEvent->fd ? -1 : 0);
}

static int file_event_find_compare_proc(ezRBTreeNode * node, void *find_args)
{
	int fd = *((int *)find_args);
	ezFileEvent *fe = cast_to_file_event(node);
	return fe->fd == fd ? 0 : (fe->fd > fd ? 1 : -1);
}

static ezFileEvent *ez_fund_file_event(ezEventLoop * eventLoop, int fd)
{
	ezRBTreeNode *n =
	    rbtree_find_node(&eventLoop->rbtree_events, file_event_find_compare_proc, (void *)&fd);
	return n == NULL ? NULL : cast_to_file_event(n);
}

ezEventLoop *ez_create_event_loop(int setsize)
{
	ezEventLoop *eventLoop = NULL;

	eventLoop = (ezEventLoop *) ez_malloc(sizeof(ezEventLoop));
	if (!eventLoop)
		goto err;

	rbtree_init(&eventLoop->rbtree_events, &eventLoop->rbnode_sentinel,
		    file_event_compare_proc);
	eventLoop->free_events = NULL;

	eventLoop->fired = (ezFiredEvent *) ez_malloc(sizeof(ezFiredEvent) * setsize);
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

void ez_delete_event_loop(ezEventLoop * eventLoop)
{
	ezRBTreeNode *i;
	ezFileEvent *e;
	ezTimeEvent *t;
	list_head *ti, *tlist;
	if (!eventLoop)
		return;
	ezApiDelete(eventLoop);

	// free ezFileEvent.
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

    for (ti = eventLoop->time_events.next; ti != &eventLoop->time_events;) {
		tlist = ti;
        ti = ti->next;
		list_del(tlist);

		t = cast_to_time_event(tlist);
		log_debug("delete time event [id:%li].", t->id);
		ez_free(t);
	}

	ez_free(eventLoop);
}

void ez_stop_event_loop(ezEventLoop * eventLoop)
{
	if (!eventLoop)
		return;
	ezApiStop(eventLoop);
}

int ez_create_file_event(ezEventLoop * eventLoop, int fd, int mask, ezFileProc proc,
			 void *clientData)
{
	ezFileEvent *fe = ez_fund_file_event(eventLoop, fd);
	int oldmask;
	int add;

	if (fe == NULL) {
		if (eventLoop->count >= eventLoop->setsize) {
			log_error("event loop create file event count's over setsize:%d !",
				  eventLoop->setsize);
			return AE_ERR;
		}
		// 从回收链接表中取
		if (eventLoop->free_events != NULL) {
			fe = cast_to_file_event(eventLoop->free_events);
			eventLoop->free_events = eventLoop->free_events->parent;
			memset(fe, 0, sizeof(ezFileEvent));
		} else {
			fe = ez_malloc(sizeof(ezFileEvent));
		}

		add = 1;
		oldmask = AE_NONE;

		fe->fd = fd;
		fe->mask = mask;
		fe->rfileProc = proc;
		fe->clientData = clientData;
	} else {
		add = 0;
		oldmask = fe->mask;
		fe->mask |= mask;

		// update proc clientData
		if (proc != NULL && fe->rfileProc != proc) {
			log_warn("file fd:%d add new mask event's proc not same!", fe->fd);
			fe->rfileProc = proc;
		}
		if (clientData != NULL && fe->clientData != clientData) {
			log_warn("file fd:%d add new mask  event's proc args not same!", fe->fd);
			fe->clientData = clientData;
		}
	}

	if (ezApiAddEvent(eventLoop, fd, mask, oldmask) == -1) {
		ez_free(fe);
		return AE_ERR;
	}

	if (add) {
		++(eventLoop->count);
		rbtree_insert(&eventLoop->rbtree_events, &(fe->rb_node));
	}

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
		--(eventLoop->count);
		put_to_free_file_events(eventLoop, fe);
	}
}

static void insert_time_event_list(list_head *first, list_head *end, ezTimeEvent *te) {
	list_head *ti ;
	for (ti = first; ti != end;) {
		ezTimeEvent *i = cast_to_time_event(ti);
		if (te->when_ms > i->when_ms) {
			ti = ti->next;
		} else {
			break;
		}
	}
	list_add(&te->listNode, ti); // add ti after.
}
/**
 * eventLoop    事件loop
 * milliseconds 启动时间
 */
int64_t ez_create_time_event(ezEventLoop * eventLoop, int64_t period, ezTimeProc proc,
			     void *clientData)
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
	log_debug("create time event [id:%li].", id);

	insert_time_event_list(eventLoop->time_events.next, &eventLoop->time_events, te);
	return id;
}

void ez_delete_time_event(ezEventLoop * eventLoop, int64_t id)
{
    ezTimeEvent *te = NULL;
	log_debug("delete time event [id:%li].", id);

    for (list_head * tlist = eventLoop->time_events.next; tlist != &eventLoop->time_events;) {
        te = cast_to_time_event(tlist);
        if (te->id == id) {
            list_del(tlist);
            ez_free(te);
            break;
        } else {
            tlist = tlist->next;
        }
   	}
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
	int processed = 0;
	int all_fired = 0;
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

	for (list_head *ti = eventLoop->time_events.next; ti != &eventLoop->time_events;) {
		list_head *tmp = ti;
		ezTimeEvent *te = cast_to_time_event(tmp);
		now_ms = ez_get_cur_milliseconds();

		if (all_fired || now_ms >= te->when_ms) {
			ti = ti->next;
			list_del(tmp);

			log_debug("pop time event [id:%li], call it!", te->id);
			int retval = te->timeProc(eventLoop, te->id, te->clientData);
			processed++;

			if (retval <= AE_TIMER_END) {
				log_debug("delete once time event [id:%li]", te->id);
				ez_free(te);
			} else {
				if (retval == AE_TIMER_NEXT)
					te->when_ms = now_ms + te->period;
				else
					te->when_ms = now_ms + retval;

				log_debug("time event [id:%li] next .", te->id);
				insert_time_event_list(ti, &eventLoop->time_events, te);
			}
		} else {
			// 没得比这个时间小的了，停止处理timeEvent.
			break;
		}
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
			shortest = NULL;
			// 第一个timeEvent就是最少的wait time.
			if (!list_empty(&eventLoop->time_events)) {
				shortest = cast_to_time_event(eventLoop->time_events.next);
			}
		}
		if (shortest != NULL) {
			tvp = (int)(shortest->when_ms - ez_get_cur_milliseconds());
		} else {
			tvp = -1;	// wait for block
		}

		numevents = ezApiPoll(eventLoop, tvp);
		for (j = 0; j < numevents; j++) {
			int fd = eventLoop->fired[j].fd;
			int fired_mask = eventLoop->fired[j].mask;
			ezFileEvent *fe = ez_fund_file_event(eventLoop, fd);

			if (((fired_mask & AE_READABLE) || (fired_mask & AE_WRITABLE))
			    && fe != NULL && fe->mask != AE_NONE) {
				// 只击发一次，由函数中实行中区分出是<AE_READABLE|AE_WRITABLE>操作.
				fe->rfileProc(eventLoop, fd, fe->clientData, fired_mask);
			}
			processed++;
		}
	}
	/* Check time events */
	if (flags & AE_TIME_EVENTS)
		processed += process_time_events(eventLoop);

	return processed;	/* return the number of processed file/time events */
}
