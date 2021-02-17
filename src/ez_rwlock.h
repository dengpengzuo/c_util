#ifndef EZ_RWLOCK_H
#define EZ_RWLOCK_H

typedef struct rwlock_s {
    int write;
    int read;
} rwlock_t;

void rwlock_init(rwlock_t* lock);

void rwlock_rlock(rwlock_t* lock);

void rwlock_runlock(rwlock_t* lock);

void rwlock_wlock(rwlock_t* lock);

void rwlock_wunlock(rwlock_t* lock);

#endif
