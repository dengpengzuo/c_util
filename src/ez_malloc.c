#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_JEMALLOC
// apt install libjemalloc-dev
#include <jemalloc/jemalloc.h>
#else
#include <malloc.h>
#endif

#define __malloc_size(p) malloc_usable_size(p)
#define __malloc(size) malloc(size)
#define __calloc(count, size) calloc(count, size)
#define __realloc(ptr, size) realloc(ptr, size)
#define __free(ptr) free(ptr)
#define __memalign(pptr, align, size) posix_memalign(pptr, align, size)

#include "ez_atomic.h"
#include "ez_log.h"
#include "ez_macro.h"
#include "ez_malloc.h"

static volatile uint64_t used_memory = 0;

#define update_zmalloc_stat_alloc(__n)           \
    do                                           \
    {                                            \
        ATOM_INC_N(&used_memory, EZ_ALIGN(__n)); \
    } while (0)

#define update_zmalloc_stat_free(__n)            \
    do                                           \
    {                                            \
        ATOM_DEC_N(&used_memory, EZ_ALIGN(__n)); \
    } while (0)

static void zmalloc_default_oom(size_t size)
{
    fprintf(stderr, "zmalloc: Out of memory trying to allocate (%zu) bytes.", size);
    fflush(stderr);
    abort();
}

static zmalloc_oom_handler_t zmalloc_oom_handler = zmalloc_default_oom;

static void *zmalloc(size_t size)
{
    void *ptr = __malloc(size);

    if (!ptr)
        zmalloc_oom_handler(size);

    update_zmalloc_stat_alloc(__malloc_size(ptr));
    // 分配后将其清零.
    memset(ptr, 0, size);
    return ptr;
}

static void *zcalloc(size_t count, size_t size)
{
    size_t nSize = count * size;
    void * ptr = __calloc(count, size);

    if (!ptr)
        zmalloc_oom_handler(nSize);

    update_zmalloc_stat_alloc(__malloc_size(ptr));

    return ptr;
}

static void *zrealloc(void *ptr, size_t size)
{
    size_t oldsize;
    void * newptr;

    if (ptr == NULL)
        return zmalloc(size);
    oldsize = __malloc_size(ptr);
    newptr = __realloc(ptr, size);
    if (!newptr)
        zmalloc_oom_handler(size);

    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(__malloc_size(newptr));
    return newptr;
}

static void zfree(void *ptr)
{
    if (ptr == NULL)
        return;
    update_zmalloc_stat_free(__malloc_size(ptr));
    __free(ptr);
}

size_t ez_malloc_used_memory(void)
{
    uint64_t um = used_memory;
    return (size_t)um;
}

void ezmalloc_set_oom_handler(zmalloc_oom_handler_t oom_handler)
{
    zmalloc_oom_handler = oom_handler;
}

void *ez_malloc_lg(size_t size, const char *name, int line)
{
    void *p;

    p = zmalloc(size);
    if (p == NULL)
    {
        log_error_x(name, line, "malloc(%zu bytes) failed ", size);
    }
    else
    {
        log_debug_x(name, line, "malloc(addr: %p, size: %zu bytes)", p, size);
    }

    return p;
}

void *ez_calloc_lg(size_t num, size_t size, const char *name, int line)
{
    void *p;

    p = zcalloc(num, size);
    if (p == NULL)
    {
        log_error_x(name, line, "calloc(%zu,%zu) failed", num, size);
    }
    else
    {
        log_debug_x(name, line, "calloc(addr: %p, count: %zu, per size: %zu bytes)", p, num, size);
    }

    return p;
}

void *ez_realloc_lg(void *ptr, size_t size, const char *name, int line)
{
    void *p;
    // 要注意：如果size=0时，CLIB会执行free(ptr)，这样外部再free(ptr)时会报错
    p = zrealloc(ptr, size);
    if (p == NULL)
    {
        log_error_x(name, line, "realloc(addr: %p, size:%zu) failed ", ptr, size);
    }
    else
    {
        log_debug_x(name, line, "realloc(addr: %p, size: %zu bytes)", p, size);
    }

    return p;
}

void ez_free_lg(void *ptr, const char *name, int line)
{
    log_debug_x(name, line, "free(addr: %p)", ptr);
    zfree(ptr);
}

void *ez_memalign_lg(size_t alignment, size_t size, const char *name, int line)
{
    void *p = NULL;
    int   err;

    err = __memalign(&p, alignment, size);

    if (p == NULL)
    {
        log_error_x(name, line, "memalign(%zu, %zu) failed [err_Code:%d].", alignment, size, err);
    }
    else
    {
        log_debug_x(name, line, "memalign: %p %zu @%zu", p, size, alignment);
    }
    return p;
}
