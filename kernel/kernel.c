/* =============================================================================
 * SENG21213-OS :: Main Kernel (Stage 1 – Process Management & Scheduler)
 * File   : kernel/kernel.c
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "../include/types.h"

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* Helper to write directly to VGA memory without moving the shell cursor */
static void direct_vga_write(int row, int col, const char *str, uint8_t color) {
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    int offset = row * 80 + col;
    for (int i = 0; str[i]; i++) {
        vga[offset + i] = (uint16_t)((color << 8) | (uint8_t)str[i]);
    }
}

/* ---------------------------------------------------------------------------
 * Concurrent Demo Tasks (Assignment Requirement for Stage 1)
 * --------------------------------------------------------------------------*/
static void worker_proc_a(void) {
    int count = 0;
    while (1) {
        /* Blink A in top right directly in memory (never touches shell cursor) */
        direct_vga_write(0, 68, count % 2 == 0 ? "[A:*]" : "[A: ]", VGA_LIGHT_CYAN);
        count++;
        for (volatile int i = 0; i < 4000000; i++);
        yield();
    }
}

static void worker_proc_b(void) {
    int count = 0;
    while (1) {
        /* Blink B in top right directly in memory at different speed */
        direct_vga_write(1, 68, count % 2 == 0 ? "[B:#]" : "[B: ]", VGA_LIGHT_MAGENTA);
        count++;
        for (volatile int i = 0; i < 8000000; i++);
        yield();
    }
}

/* ---------------------------------------------------------------------------
 * Shell Commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  help        - Show this help message\n");
    vga_puts("  clear       - Clear the screen\n");
    vga_puts("  about       - About this OS and course\n");
    vga_puts("  echo <text> - Echo text to screen\n");
    vga_puts("  mem         - Memory map (stub)\n");
    vga_puts_color("  ps          - [L09] List all active processes\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts_color("  kill <pid>  - [L09] Terminate a process\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_about(void) {
    vga_puts_color("\n  SENG21213-OS :: Stage 1\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Department of Software Engineering, University of Kelaniya\n");
    vga_puts("  Features: Round-Robin Preemptive Scheduler, Process Table, Context Switch\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_mem(void) {
    vga_puts_color("\n  Memory Map (stub – implement PMM in Lecture 11)\n",
                   VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  0x00000000 – 0x000FFFFF : First 1 MB (reserved/BIOS)\n");
    vga_puts("  0x00100000 – 0x00EFFFFF : Extended memory (usable ~14 MB)\n");
    vga_puts("  0x00F00000 – 0x00FFFFFF : BIOS / ROM area\n");
    vga_puts("  0xB8000    – 0xBFFFF    : VGA frame buffer\n\n");
}

/* ---------------------------------------------------------------------------
 * Shell Loop
 * --------------------------------------------------------------------------*/
static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready (Stage 1). Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }

        /* Stage 1 Commands */
        if (k_strcmp(cmd, "ps") == 0) {
            process_print_table();
            continue;
        }

        if (k_strncmp(cmd, "kill ", 5) == 0) {
            const char *arg = k_ltrim(cmd + 5);
            uint32_t pid = 0;
            while (*arg >= '0' && *arg <= '9') {
                pid = pid * 10 + (*arg - '0');
                arg++;
            }
            process_kill(pid);
            continue;
        }

        /* Later milestones */
        if (k_strcmp(cmd, "threads") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is scheduled for Stages 2-4.\n",
                           VGA_YELLOW, VGA_BLACK);
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel Entry Point
 * --------------------------------------------------------------------------*/
void kernel_main(void) {
    vga_init();
    kb_init();

    /* Initialize Stage 1 Process Table & Round-Robin Scheduler */
    process_init();
    scheduler_init();

    /* Launch two background tasks running concurrently */
    process_create("worker_A", worker_proc_a);
    process_create("worker_B", worker_proc_b);

    vga_clear(VGA_BLACK);
    vga_puts_color("  SENG21213-OS :: Stage 1 (Process Scheduler)\n", VGA_LIGHT_MAGENTA, VGA_BLACK);
    vga_puts("  Two background tasks [A] and [B] are running concurrently.\n");
    vga_puts("  Type 'ps' to view active processes or 'help' for commands.\n");

    shell_run();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
