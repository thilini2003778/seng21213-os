#include "process.h"
#include "vga.h"
#include "../include/types.h"

static pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = 0;
static uint32_t next_pid = 1;

/* String copy helper */
static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Initialize Process Management (PID 0 is the Kernel/Shell) */
void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_UNUSED;
        process_table[i].ticks = 0;
    }

    /* Process 0 is the active shell */
    process_table[0].pid = 0;
    k_strncpy(process_table[0].name, "shell", sizeof(process_table[0].name));
    process_table[0].state = PROCESS_RUNNING;
    current_process = &process_table[0];
}

/* Create a new process with its own 4 KB stack */
int process_create(const char *name, void (*entry_point)(void)) {
    int slot = -1;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED ||
            process_table[i].state == PROCESS_TERMINATED) {
            slot = i;
            break;
        }
    }

    if (slot == -1) return -1; /* No free slot */

    pcb_t *proc = &process_table[slot];
    proc->pid = next_pid++;
    k_strncpy(proc->name, name, sizeof(proc->name));
    proc->state = PROCESS_READY;
    proc->ticks = 0;

    /* Prepare stack frame so switch_context pops it smoothly */
    uint32_t *st = (uint32_t *)(proc->stack + STACK_SIZE);

    *(--st) = (uint32_t)process_exit;  /* Return address if entry_point finishes */
    *(--st) = (uint32_t)entry_point;   /* Return address for switch_context (EIP) */

    /* PUSHAD registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI) */
    *(--st) = 0; /* EDI */
    *(--st) = 0; /* ESI */
    *(--st) = 0; /* EBP */
    *(--st) = 0; /* ESP dummy */
    *(--st) = 0; /* EBX */
    *(--st) = 0; /* EDX */
    *(--st) = 0; /* ECX */
    *(--st) = 0; /* EAX */

    /* PUSHFD (EFLAGS: 0x202 enables interrupts) */
    *(--st) = 0x0202;

    proc->esp = (uint32_t)st;
    return proc->pid;
}

/* Terminate the currently running process */
void process_exit(void) {
    if (current_process && current_process->pid != 0) {
        current_process->state = PROCESS_TERMINATED;
    }
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/* Kill a process by PID */
void process_kill(uint32_t pid) {
    if (pid == 0) {
        vga_puts_color("  Cannot kill kernel shell (PID 0)!\n", VGA_LIGHT_RED, VGA_BLACK);
        return;
    }
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_UNUSED) {
            process_table[i].state = PROCESS_TERMINATED;
            vga_puts_color("  Process terminated.\n", VGA_LIGHT_GREEN, VGA_BLACK);
            return;
        }
    }
    vga_puts_color("  Process PID not found.\n", VGA_LIGHT_RED, VGA_BLACK);
}

/* Print Process Table (for 'ps' command) */
void process_print_table(void) {
    vga_puts_color("\n  PID   NAME            STATE         TICKS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");

    const char *state_names[] = {
        "UNUSED", "READY", "RUNNING", "BLOCKED", "TERMINATED"
    };

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_UNUSED) continue;

        pcb_t *p = &process_table[i];

        /* Print PID */
        char buf[16];
        buf[0] = ' '; buf[1] = ' ';
        buf[2] = (p->pid >= 10) ? ('0' + (p->pid / 10)) : ' ';
        buf[3] = '0' + (p->pid % 10);
        buf[4] = ' '; buf[5] = ' '; buf[6] = '\0';
        vga_puts(buf);

        /* Print Name */
        vga_puts(p->name);
        int pad = 16;
        for (int j = 0; p->name[j]; j++) pad--;
        while (pad-- > 0) vga_putchar(' ');

        /* Print State */
        vga_color_t color = (p->state == PROCESS_RUNNING) ? VGA_LIGHT_GREEN :
                            (p->state == PROCESS_READY)   ? VGA_YELLOW : VGA_LIGHT_GREY;
        vga_puts_color(state_names[p->state], color, VGA_BLACK);

        pad = 14;
        const char *sname = state_names[p->state];
        for (int j = 0; sname[j]; j++) pad--;
        while (pad-- > 0) vga_putchar(' ');

        /* Print Ticks */
        char tick_buf[16];
        uint32_t t = p->ticks;
        int idx = 0;
        if (t == 0) tick_buf[idx++] = '0';
        else {
            char temp[16];
            int tidx = 0;
            while (t > 0) {
                temp[tidx++] = '0' + (t % 10);
                t /= 10;
            }
            while (tidx > 0) tick_buf[idx++] = temp[--tidx];
        }
        tick_buf[idx] = '\0';
        vga_puts(tick_buf);
        vga_puts("\n");
    }
    vga_puts("\n");
}

pcb_t *process_get_current(void) { return current_process; }
void process_set_current(pcb_t *proc) { current_process = proc; }
pcb_t *process_get_table(void) { return process_table; }
