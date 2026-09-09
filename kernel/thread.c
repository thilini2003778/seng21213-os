#include "thread.h"
#include "vga.h"
#include "scheduler.h"
#include "../include/types.h"

extern void switch_context(uint32_t *old_esp, uint32_t new_esp);

static thread_t thread_table[MAX_THREADS];
static thread_t *current_thread = 0;
static uint32_t next_tid = 1;

static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Wrapper that executes the thread's function then calls thread_exit */
static void thread_trampoline(void) {
    if (current_thread && current_thread->entry_fn) {
        current_thread->entry_fn(current_thread->arg);
    }
    thread_exit();
}

void thread_init(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_UNUSED;
    }

    /* Thread 0 represents the main kernel/shell thread */
    thread_table[0].tid = 0;
    k_strncpy(thread_table[0].name, "main", sizeof(thread_table[0].name));
    thread_table[0].state = THREAD_RUNNING;
    current_thread = &thread_table[0];
}

int thread_create(const char *name, void (*fn)(void *), void *arg) {
    int slot = -1;
    for (int i = 1; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED ||
            thread_table[i].state == THREAD_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return -1;

    thread_t *t = &thread_table[slot];
    t->tid = next_tid++;
    k_strncpy(t->name, name, sizeof(t->name));
    t->state = THREAD_READY;
    t->entry_fn = fn;
    t->arg = arg;

    uint32_t *st = (uint32_t *)(t->stack + THREAD_STACK_SIZE);

    *(--st) = (uint32_t)thread_exit;       /* Return fallback */
    *(--st) = (uint32_t)thread_trampoline; /* Entry point */

    /* Dummy registers for PUSHAD */
    *(--st) = 0; /* EDI */
    *(--st) = 0; /* ESI */
    *(--st) = 0; /* EBP */
    *(--st) = 0; /* ESP */
    *(--st) = 0; /* EBX */
    *(--st) = 0; /* EDX */
    *(--st) = 0; /* ECX */
    *(--st) = 0; /* EAX */

    /* EFLAGS (0x202 enables interrupts) */
    *(--st) = 0x0202;

    t->esp = (uint32_t)st;
    return t->tid;
}

void thread_exit(void) {
    if (current_thread && current_thread->tid != 0) {
        current_thread->state = THREAD_TERMINATED;
    }
    thread_yield();
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void thread_yield(void) {
    yield();
}

uint32_t thread_current_id(void) {
    return current_thread ? current_thread->tid : 0;
}

thread_t *thread_get_current(void) {
    return current_thread;
}

void thread_print_table(void) {
    vga_puts_color("\n  TID   NAME            STATE\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");

    const char *state_names[] = {
        "UNUSED", "READY", "RUNNING", "BLOCKED", "TERMINATED"
    };

    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED) continue;
        thread_t *t = &thread_table[i];

        vga_puts("   ");
        vga_putchar('0' + (t->tid % 10));
        vga_puts("    ");

        vga_puts(t->name);
        int pad = 16;
        for (int j = 0; t->name[j]; j++) pad--;
        while (pad-- > 0) vga_putchar(' ');

        vga_color_t color = (t->state == THREAD_RUNNING) ? VGA_LIGHT_GREEN :
                            (t->state == THREAD_READY)   ? VGA_YELLOW : VGA_LIGHT_GREY;
        vga_puts_color(state_names[t->state], color, VGA_BLACK);
        vga_puts("\n");
    }
    vga_puts("\n");
}
