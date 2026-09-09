#include "semaphore.h"
#include "thread.h"

void sem_init(semaphore_t *s, int val) {
    s->count = val;
}

/* P operation (wait): Decrements counter; blocks if count <= 0 */
void sem_wait(semaphore_t *s) {
    while (1) {
        __asm__ __volatile__("cli");
        if (s->count > 0) {
            s->count--;
            __asm__ __volatile__("sti");
            return;
        }
        __asm__ __volatile__("sti");
        thread_yield(); /* Yield CPU while waiting */
    }
}

/* V operation (signal): Increments counter */
void sem_signal(semaphore_t *s) {
    __asm__ __volatile__("cli");
    s->count++;
    __asm__ __volatile__("sti");
}
