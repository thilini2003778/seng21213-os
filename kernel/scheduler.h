#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../include/types.h"

/* Public Scheduler API - Lecture L09 */
void     scheduler_init(void);
void     schedule(void);
void     yield(void);
void     timer_handler(void);
uint32_t scheduler_get_ticks(void);

#endif
