#include "scheduler.h"
#include "process.h"
#include "vga.h"
#include "../include/io.h"
#include "../include/types.h"

/* External assembly functions */
extern void switch_context(uint32_t *old_esp, uint32_t new_esp);
extern void timer_isr_stub(void);

/* IDT Entry structure */
struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;
static uint32_t         system_ticks = 0;

/* Set an entry in the IDT */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = sel;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}

/* Remap PIC and set PIT to 100 Hz */
static void init_timer(void) {
    /* Remap PIC: Master to 0x20..0x27, Slave to 0x28..0x2F */
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    /* Unmask ONLY IRQ0 (timer) on Master PIC; mask all other IRQs */
    outb(0x21, 0xFE); io_wait();
    outb(0xA1, 0xFF); io_wait();

    /* Set i8253 PIT to 100 Hz (Divisor = 1193182 / 100 = 11932 = 0x2E9C) */
    uint16_t divisor = 11932;
    outb(0x43, 0x36);              /* Command: Channel 0, Lobyle/Hibyte, Mode 3 */
    outb(0x40, divisor & 0xFF);    /* Low byte */
    outb(0x40, (divisor >> 8) & 0xFF); /* High byte */
}

/* Initialize IDT and Scheduler */
void scheduler_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    /* Vector 0x20 = IRQ0 (Hardware Timer) */
    idt_set_gate(0x20, (uint32_t)timer_isr_stub, 0x08, 0x8E);

    /* Load IDT register */
    __asm__ __volatile__("lidt %0" : : "m"(idtp));

    init_timer();

    /* Enable interrupts */
    __asm__ __volatile__("sti");
}

/* Round-Robin Scheduler (Lecture L09 §4) */
void schedule(void) {
    pcb_t *table = process_get_table();
    pcb_t *current = process_get_current();

    int start_index = 0;
    if (current) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (&table[i] == current) {
                start_index = i;
                break;
            }
        }
    }

    /* Look for the next READY process in circular order */
    int next_index = -1;
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (start_index + i) % MAX_PROCESSES;
        if (table[idx].state == PROCESS_READY) {
            next_index = idx;
            break;
        }
    }

    /* If no other process is ready, continue with current if it can run */
    if (next_index == -1) {
        if (current && (current->state == PROCESS_RUNNING || current->state == PROCESS_READY)) {
            current->state = PROCESS_RUNNING;
        }
        return;
    }

    pcb_t *next_proc = &table[next_index];
    if (next_proc == current) return;

    if (current && current->state == PROCESS_RUNNING) {
        current->state = PROCESS_READY;
    }

    next_proc->state = PROCESS_RUNNING;
    process_set_current(next_proc);

    /* Perform context switch */
    switch_context(current ? &current->esp : 0, next_proc->esp);
}

/* Voluntary CPU yield */
void yield(void) {
    __asm__ __volatile__("cli");
    schedule();
    __asm__ __volatile__("sti");
}

/* Timer Interrupt Handler (fires 100 times per second) */
void timer_handler(void) {
    system_ticks++;

    pcb_t *curr = process_get_current();
    if (curr) {
        curr->ticks++;
    }

    /* Send EOI (End of Interrupt) to PIC */
    outb(0x20, 0x20);

    /* Preempt and switch tasks every 10 ticks (100 ms time slice) */
    if (system_ticks % 10 == 0) {
        schedule();
    }
}

uint32_t scheduler_get_ticks(void) {
    return system_ticks;
}
