#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "../include/types.h"

/* Counting Semaphore - Lecture L10 §5 */
typedef struct {
    volatile int count;
} semaphore_t;

void sem_init(semaphore_t *s, int val);
void sem_wait(semaphore_t *s);
void sem_signal(semaphore_t *s);

#endif
