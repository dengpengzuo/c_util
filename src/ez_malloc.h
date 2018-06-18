
#ifndef __ZMALLOC_H
#define __ZMALLOC_H

#include <stddef.h>
#include <stdint.h>

size_t ez_malloc_used_memory(void);

typedef void (*zmalloc_oom_handler_t) (size_t);

void ezmalloc_set_oom_handler(zmalloc_oom_handler_t oom_handler);

#define ez_malloc(_n)                    \
    ez_malloc_lg((size_t)(_n), __FILE__, __LINE__)

#define ez_calloc(_n, _s)               \
    ez_calloc_lg((size_t)(_n), (size_t)(_s), __FILE__, __LINE__)

#define ez_realloc(_p, _s)              \
    ez_realloc_lg(_p, (size_t)(_s), __FILE__, __LINE__)

#define ez_free(_p) do {                \
    ez_free_lg(_p, __FILE__, __LINE__);   \
    (_p) = NULL;                        \
} while (0)

#define NGX_POOL_ALIGNMENT       16

#define ez_memalign(_n)     \
    ez_memalign_lg(NGX_POOL_ALIGNMENT, (size_t)(_n), __FILE__, __LINE__)

void *ez_malloc_lg(size_t size, const char *name, int line);

void *ez_calloc_lg(size_t num, size_t size, const char *name, int line);

void *ez_realloc_lg(void *ptr, size_t size, const char *name, int line);

void ez_free_lg(void *ptr, const char *name, int line);

void *ez_memalign_lg(size_t alignment, size_t size, const char *name, int line);

#endif				/* __ZMALLOC_H */
