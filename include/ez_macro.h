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
#define EZ_ALIGNMENT           sizeof(uintptr_t)	/* platform word */

#define EZ_ALIGN(d)            EZ_ALIGN_TO(d, EZ_ALIGNMENT)
#define EZ_ALIGN_TO(d, n)      ((size_t)(((d) + (n - 1)) & ~(n - 1)))

#define EZ_ALIGN_PTR(p)        EZ_ALIGN_PTR_TO(p, EZ_ALIGNMENT)
#define EZ_ALIGN_PTR_TO(p,n)   ((void *) (((uintptr_t) (p) + ((uintptr_t) n - 1)) & ~((uintptr_t) n - 1)))

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
