#ifndef MUTEX_H
#define MUTEX_H

#include "../include/types.h"

/* Mutex Structure - Lecture L10 §4 */
typedef struct {
    volatile int locked;
    uint32_t     owner_tid;
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
