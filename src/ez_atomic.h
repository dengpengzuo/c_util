#ifndef EZ_CUTIL_EZ_ATOMIC_H
#define EZ_CUTIL_EZ_ATOMIC_H

#if !defined(__ATOMIC_VAR_FORCE_SYNC_MACROS) && defined(__ATOMIC_RELAXED)

#define _ATOMIC_API "atomic-builtin"

/**
 * 如果*ptr == oldval,就将newval写入*ptr，并返回true
 */
#define ATOM_CAS(ptr, oval, nval) __atomic_compare_exchange(ptr, oval, nval, true, __ATOMIC_RELAXED, __ATOMIC_RELAXED)

#define ATOM_INC(ptr) __atomic_add_fetch(ptr, 1, __ATOMIC_RELAXED)
#define ATOM_INC_N(ptr, n) __atomic_add_fetch(ptr, n, __ATOMIC_RELAXED)
#define ATOM_FINC(ptr) __atomic_fetch_add(ptr, 1, __ATOMIC_RELAXED)
#define ATOM_FINC_N(ptr, n) __atomic_fetch_add(ptr, n, __ATOMIC_RELAXED)

#define ATOM_DEC(ptr) __atomic_sub_fetch(ptr, 1, __ATOMIC_RELAXED)
#define ATOM_DEC_N(ptr, n) __atomic_sub_fetch(ptr, n, __ATOMIC_RELAXED)
#define ATOM_FDEC(ptr) __atomic_fetch_sub(ptr, 1, __ATOMIC_RELAXED)
#define ATOM_FDEC_N(ptr, n) __atomic_fetch_sub(ptr, n, __ATOMIC_RELAXED)

/* Atomic BIT operator */
#define ATOM_BIT_AND(ptr, n) __atomic_and_fetch(ptr, n, __ATOMIC_RELAXED)
#define ATOM_BIT_FAND(ptr, n) __atomic_fetch_and(ptr, n, __ATOMIC_RELAXED)
#define ATOM_BIT_OR(ptr, n) __atomic_or_fetch(ptr, n, __ATOMIC_RELAXED)
#define ATOM_BIT_FOR(ptr, n) __atomic_fetch_or(ptr, n, __ATOMIC_RELAXED)

#else /* user gcc __sync */

#define _ATOMIC_API "sync-builtin"
/**
 * 如果*ptr == oldval,就将newval写入*ptr，并返回true
 */
#define ATOM_CAS(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)

#define ATOM_INC(ptr) __sync_add_and_fetch(ptr, 1)
#define ATOM_INC_N(ptr, n) __sync_add_and_fetch(ptr, n)
#define ATOM_FINC(ptr) __sync_fetch_and_add(ptr, 1)
#define ATOM_FINC_N(ptr, n) __sync_fetch_and_add(ptr, n)

#define ATOM_DEC(ptr) __sync_sub_and_fetch(ptr, 1)
#define ATOM_DEC_N(ptr, n) __sync_sub_and_fetch(ptr, n)
#define ATOM_FDEC(ptr) __sync_fetch_and_sub(ptr, 1)
#define ATOM_FDEC_N(ptr, n) __sync_fetch_and_sub(ptr, n)

/* Atomic BIT operator */
#define ATOM_BIT_AND(ptr, n) __sync_and_and_fetch(ptr, n)
#define ATOM_BIT_FAND(ptr, n) __sync_fetch_and_and(ptr, n)
#define ATOM_BIT_OR(ptr, n) __sync_or_and_fetch(ptr, n)
#define ATOM_BIT_FOR(ptr, n) __sync_fetch_and_or(ptr, n)

#endif

#endif // EZ_CUTIL_EZ_ATOMIC_H
