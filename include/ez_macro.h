/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#ifndef EZ_MACRO_H
#define EZ_MACRO_H

#include <stdint.h>

#define XSTRINGIFY(x)       #x
#define STRINGIFY(x)        XSTRINGIFY(x)

#define EZ_NOTUSED(V)	    ((void) V)
#define EZ_NELEMS(ARRAY)    ((sizeof(ARRAY)) / sizeof((ARRAY)[0]))

#define EZ_OFFSETOF(TYPE, MEMBER)             ((uintptr_t)&(((TYPE *)0)->MEMBER))
#define EZ_CONTAINER_OF(ptr, type, member)    ((type *)(((uintptr_t)ptr) - EZ_OFFSETOF(type, member)))

/*
 * Make data 'd' or pointer 'p', n-byte aligned, where n is a power of 2
 */
#define EZ_ALIGNMENT           sizeof(uintptr_t)    /* platform word */
#define EZ_ALIGN(d)            (ez_align_to(d, EZ_ALIGNMENT-1))
static inline size_t ez_align_to(size_t d, size_t a) {
    return (size_t)(d + a) & ~a;
}

#define EZ_ALIGN_PTR(p)        (ez_align_ptr_to(p,EZ_ALIGNMENT-1))
static inline void *ez_align_ptr(void *ptr, size_t a) {
    return (void *) (uintptr_t) ez_align_to((size_t)((uintptr_t) ptr), a);
}

/* pointer cast int macro */
#define PTR_TO_UINT32(p) ((uint32_t) ((uintptr_t) (p)))
#define UINT32_TO_PTR(u) ((void*) ((uintptr_t) (u)))

#define PTR_TO_UINT64(p) ((uint64_t) ((uintptr_t) (p)))
#define UINT64_TO_PTR(u) ((void*) ((uintptr_t) (u)))

#define PTR_TO_INT32(p)  ((int32_t) ((intptr_t) (p)))
#define INT32_TO_PTR(u)  ((void*) ((intptr_t) (u)))

#define PTR_TO_INT64(p)  ((int64_t)((intptr_t) (p)))
#define INT64_TO_PTR(u)  ((void*) ((intptr_t) (u)))

#endif /* EZ_MACRO_H */
