#include <ez_rwlock.h>

inline void rwlock_init(rwlock *lock) {
    lock->write = 0;
    lock->read = 0;
}

inline void rwlock_rlock(rwlock *lock) {
    for (; ;) {
        while (lock->write) {
            __sync_synchronize();
        }

        __sync_add_and_fetch(&lock->read, 1);     // read ++

        if (lock->write) {
            __sync_sub_and_fetch(&lock->read, 1); // read --;
        } else {
            break;
        }
    }
}

inline void rwlock_runlock(rwlock *lock) {
    __sync_sub_and_fetch(&lock->read, 1);        // read --;
}

inline void rwlock_wlock(rwlock *lock) {
    // 将*ptr设为value并返回*ptr操作之前的值,将[0->1]，其他的变化为1都是等待.
    while (__sync_lock_test_and_set(&lock->write, 1)) { }

    while (lock->read) {
        __sync_synchronize(); // 等待read == 0
    }
}

inline void rwlock_wunlock(rwlock *lock) {
    __sync_lock_release(&lock->write); // 将*ptr 置 0
}