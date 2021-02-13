#ifndef _EZ_EVENT_H
#define _EZ_EVENT_H

#include <stdint.h>

#define AE_OK 0
#define AE_ERR -1

#define AE_TIMER_END -1 /* timeProc 返回值，表示当前time事件执行完后，要求从eventLoop中直接移除 */
#define AE_TIMER_NEXT 0 /* timeProc 返回值，表示当前time事件执行完后，按照原来的超时继续执行 */

/* 结构定义 */
typedef struct ez_event_loop_s ez_event_loop_t;

typedef enum
{
    AE_NONE = 0x0,
    AE_READABLE = 0x1,
    AE_WRITABLE = 0x2
} EVENT_MASK;

typedef void (*ezFileProc)(ez_event_loop_t *eventLoop, int fd, void *clientData, int mask);
typedef int (*ezTimeProc)(ez_event_loop_t *eventLoop, int64_t timeId, void *clientData);

/* Prototypes */
ez_event_loop_t *ez_create_event_loop(int setsize);
void             ez_delete_event_loop(ez_event_loop_t *eventLoop);

/* socket event */
int  ez_create_file_event(ez_event_loop_t *eventLoop, int fd, EVENT_MASK mask, ezFileProc proc, void *clientData);
void ez_delete_file_event(ez_event_loop_t *eventLoop, int fd, EVENT_MASK mask);

/* time out event*/
int64_t ez_create_time_event(ez_event_loop_t *eventLoop, int64_t period, ezTimeProc proc, void *clientData);
void    ez_delete_time_event(ez_event_loop_t *eventLoop, int64_t time_id);

void ez_stop_event_loop(ez_event_loop_t *eventLoop);

void ez_run_event_loop(ez_event_loop_t *eventLoop);

#endif /* _EZ_EVENT_H */
