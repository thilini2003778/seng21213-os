#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"

#define MAX_THREADS 16
#define THREAD_STACK_SIZE 4096

typedef enum {
    THREAD_UNUSED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_TERMINATED
} thread_state_t;

/* Thread Control Block (TCB) - Lecture L10 */
typedef struct thread {
    uint32_t       tid;
    char           name[32];
    thread_state_t state;
    uint32_t       esp;
    void         (*entry_fn)(void *);
    void          *arg;
    uint8_t        stack[THREAD_STACK_SIZE];
} thread_t;

void     thread_init(void);
int      thread_create(const char *name, void (*fn)(void *), void *arg);
void     thread_exit(void);
void     thread_yield(void);
void     thread_print_table(void);
thread_t *thread_get_current(void);
uint32_t thread_current_id(void);

#endif
