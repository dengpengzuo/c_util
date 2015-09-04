#ifndef EZ_RWLOCK_H
#define EZ_RWLOCK_H

typedef struct rwlock_t {
    int write;
    int read;
} rwlock;

void rwlock_init(rwlock *lock);

void rwlock_rlock(rwlock *lock);

void rwlock_runlock(rwlock *lock);

void rwlock_wlock(rwlock *lock);

void rwlock_wunlock(rwlock *lock);

#endif
