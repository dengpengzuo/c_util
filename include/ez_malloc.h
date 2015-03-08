
#ifndef __ZMALLOC_H
#define __ZMALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifndef EZMEM_ALIGNMENT
#define EZMEM_ALIGNMENT       sizeof(uintptr_t)
#endif

#define ezmem_align(d, a)     (((d) + (a - 1)) & ~(a - 1))

#define ezmem_align_ptr(p, a)  \
    (uint8_t *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

/* =============================================================================
 * Memory allocation and free
 */
void *zmalloc(size_t size);
void *zcalloc(size_t count, size_t size);
void *zrealloc(void *ptr, size_t size);
void zfree(void *ptr);

size_t zmalloc_used_memory(void);

void zmalloc_disable_thread_safeness(void);

typedef void (*zmalloc_oom_handler_t) (size_t);
void zmalloc_set_oom_handler(zmalloc_oom_handler_t oom_handler);

/* =============================================================================
 * Memory allocation and free wrappers log info.
 */
#define ez_malloc(_n)                    \
    _ez_malloc((size_t)(_n), __FILE__, __LINE__)

#define ez_calloc(_n, _s)               \
    _ez_calloc((size_t)(_n), (size_t)(_s), __FILE__, __LINE__)

#define ez_realloc(_p, _s)              \
    _ez_realloc(_p, (size_t)(_s), __FILE__, __LINE__)

#define ez_free(_p) do {                \
    _ez_free(_p, __FILE__, __LINE__);   \
    (_p) = NULL;                        \
} while (0)

void *_ez_malloc(size_t size, const char *name, int line);
void *_ez_calloc(size_t num, size_t size, const char *name, int line);
void *_ez_realloc(void *ptr, size_t size, const char *name, int line);
void _ez_free(void *ptr, const char *name, int line);

#endif				/* __ZMALLOC_H */
