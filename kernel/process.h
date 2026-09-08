#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES 16
#define STACK_SIZE     4096

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

/* Process Control Block (PCB) - Lecture L09 */
typedef struct pcb {
    uint32_t        pid;
    char            name[32];
    process_state_t state;
    uint32_t        esp;              /* Saved stack pointer */
    uint32_t        ticks;            /* CPU time consumed */
    uint8_t         stack[STACK_SIZE];/* 4 KB dedicated stack */
} pcb_t;

/* Public Process API */
void    process_init(void);
int     process_create(const char *name, void (*entry_point)(void));
void    process_exit(void);
void    process_kill(uint32_t pid);
void    process_print_table(void);
pcb_t  *process_get_current(void);
void    process_set_current(pcb_t *proc);
pcb_t  *process_get_table(void);

#endif
