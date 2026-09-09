#include "mutex.h"
#include "thread.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->owner_tid = 0;
}

/* Acquire the lock; blocks and yields CPU if already held */
void mutex_lock(mutex_t *m) {
    while (1) {
        __asm__ __volatile__("cli");
        if (!m->locked) {
            m->locked = 1;
            m->owner_tid = thread_current_id();
            __asm__ __volatile__("sti");
            return;
        }
        __asm__ __volatile__("sti");
        thread_yield(); /* Yield CPU to other threads while waiting */
    }
}

/* Release the lock */
void mutex_unlock(mutex_t *m) {
    __asm__ __volatile__("cli");
    m->locked = 0;
    m->owner_tid = 0;
    __asm__ __volatile__("sti");
}
