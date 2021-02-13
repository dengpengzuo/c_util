#include "ez_rwlock.h"

#include "ez_atomic.h"

inline void rwlock_init(rwlock_t *lock)
{
    lock->write = 0;
    lock->read = 0;
}

inline void rwlock_rlock(rwlock_t *lock)
{
    for (;;)
    {
        while (lock->write)
        {
            __sync_synchronize();
        }

        ATOM_INC(&lock->read); // read ++

        if (lock->write)
        {
            ATOM_DEC(&lock->read); // read --
        }
        else
        {
            break;
        }
    }
}

inline void rwlock_runlock(rwlock_t *lock)
{
    ATOM_DEC(&lock->read); // read --
}

inline void rwlock_wlock(rwlock_t *lock)
{
    // 将*ptr设为value并返回*ptr操作之前的值,将[0->1,return:0]，其他的变化为[1->1, return:1]都是等待.
    while (__sync_lock_test_and_set(&lock->write, 1))
    {
    }

    while (lock->read)
    {
        __sync_synchronize(); // 等待 read == 0
    }
}

inline void rwlock_wunlock(rwlock_t *lock)
{
    __sync_lock_release(&lock->write); // 将*ptr 置 0
}