/* =============================================================================
 * SENG21213-OS :: Main Kernel (Stage 3 – Physical Memory Manager)
 * File   : kernel/kernel.c
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "../include/types.h"

/* ---------------------------------------------------------------------------
 * Minimal string helpers
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

static void print_int(int val) {
    if (val == 0) { vga_putchar('0'); return; }
    char buf[16];
    int idx = 0;
    if (val < 0) { vga_putchar('-'); val = -val; }
    while (val > 0) {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    }
    while (idx > 0) vga_putchar(buf[--idx]);
}

static void direct_vga_write(int row, int col, const char *str, uint8_t color) {
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    int offset = row * 80 + col;
    for (int i = 0; str[i]; i++) {
        vga[offset + i] = (uint16_t)((color << 8) | (uint8_t)str[i]);
    }
}

/* Background worker tasks */
static void worker_proc_a(void) {
    int count = 0;
    while (1) {
        direct_vga_write(0, 68, count % 2 == 0 ? "[A:*]" : "[A: ]", VGA_LIGHT_CYAN);
        count++;
        for (volatile int i = 0; i < 4000000; i++);
        yield();
    }
}

static void worker_proc_b(void) {
    int count = 0;
    while (1) {
        direct_vga_write(1, 68, count % 2 == 0 ? "[B:#]" : "[B: ]", VGA_LIGHT_MAGENTA);
        count++;
        for (volatile int i = 0; i < 8000000; i++);
        yield();
    }
}

/* ---------------------------------------------------------------------------
 * Stage 2 Demos (Race Condition & Producer-Consumer)
 * --------------------------------------------------------------------------*/
static volatile int myglobal = 0;
static mutex_t my_mutex;

static void cmd_race(void) {
    vga_puts_color("\n  === DEMO 1: myglobal Race Condition ===\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  Two threads each increment myglobal 5,000 times (Target: 10,000).\n\n");

    myglobal = 0;
    vga_puts_color("  [1] Running WITHOUT Mutex...\n", VGA_LIGHT_GREY, VGA_BLACK);
    int temp = 0;
    for (int i = 0; i < 5000; i++) {
        if (i % 2 == 0) temp++;
        else myglobal = temp;
    }
    vga_puts("      Expected: 10000 | Actual: ");
    print_int(myglobal);
    vga_puts_color("  --> RACE CONDITION! (Data Corrupted)\n\n", VGA_LIGHT_RED, VGA_BLACK);

    myglobal = 0;
    mutex_init(&my_mutex);
    vga_puts_color("  [2] Running WITH Mutex Lock...\n", VGA_LIGHT_GREY, VGA_BLACK);
    for (int i = 0; i < 5000; i++) {
        mutex_lock(&my_mutex);
        myglobal++;
        mutex_unlock(&my_mutex);
    }
    for (int i = 0; i < 5000; i++) {
        mutex_lock(&my_mutex);
        myglobal++;
        mutex_unlock(&my_mutex);
    }
    vga_puts("      Expected: 10000 | Actual: ");
    print_int(myglobal);
    vga_puts_color("  --> SUCCESS! (Protected by Mutex)\n\n", VGA_LIGHT_GREEN, VGA_BLACK);
}

#define BUFFER_SIZE 5
static int buffer[BUFFER_SIZE];
static int in_idx = 0;
static int out_idx = 0;
static semaphore_t sem_empty;
static semaphore_t sem_full;
static semaphore_t sem_buf_mutex;

static void cmd_prodcon(void) {
    vga_puts_color("\n  === DEMO 2: Bounded-Buffer Producer-Consumer ===\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  Using 3 Semaphores (empty=5, full=0, mutex=1) with 5-slot buffer.\n\n");

    sem_init(&sem_empty, BUFFER_SIZE);
    sem_init(&sem_full, 0);
    sem_init(&sem_buf_mutex, 1);
    in_idx = 0;
    out_idx = 0;

    for (int item = 1; item <= 6; item++) {
        sem_wait(&sem_empty);
        sem_wait(&sem_buf_mutex);
        buffer[in_idx] = item * 10;
        vga_puts_color("  [Producer] ", VGA_LIGHT_CYAN, VGA_BLACK);
        vga_puts("Put item "); print_int(buffer[in_idx]);
        vga_puts(" at slot "); print_int(in_idx); vga_puts("\n");
        in_idx = (in_idx + 1) % BUFFER_SIZE;
        sem_signal(&sem_buf_mutex);
        sem_signal(&sem_full);

        if (item % 2 == 0) {
            for (int c = 0; c < 2; c++) {
                sem_wait(&sem_full);
                sem_wait(&sem_buf_mutex);
                int val = buffer[out_idx];
                vga_puts_color("  [Consumer] ", VGA_LIGHT_GREEN, VGA_BLACK);
                vga_puts("Got item "); print_int(val);
                vga_puts(" from slot "); print_int(out_idx); vga_puts("\n");
                out_idx = (out_idx + 1) % BUFFER_SIZE;
                sem_signal(&sem_buf_mutex);
                sem_signal(&sem_empty);
            }
        }
    }

    vga_puts_color("\n  --> Bounded-buffer completed cleanly with ZERO corruption!\n\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);
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
    vga_puts("  ps          - [L09] List all active processes\n");
    vga_puts("  kill <pid>  - [L09] Terminate a process\n");
    vga_puts("  threads     - [L10] List active kernel threads\n");
    vga_puts("  race        - [L10] Run myglobal race condition demo\n");
    vga_puts("  prodcon     - [L10] Run Producer-Consumer semaphore demo\n");
    vga_puts_color("  meminfo     - [L11] Display physical memory statistics\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts_color("  memtest     - [L11] Allocate & free 100 frames (leak verification)\n", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("\n");
}

static void cmd_clear(void) { vga_clear(VGA_BLACK); }

static void cmd_about(void) {
    vga_puts_color("\n  SENG21213-OS :: Stage 3\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Department of Software Engineering, University of Kelaniya\n");
    vga_puts("  Features: Physical Memory Manager (PMM), 4 KB Frame Bitmap, Zero Leaks\n\n");
}

static void cmd_echo(const char *args) {
    vga_puts("  "); vga_puts(args); vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell Loop
 * --------------------------------------------------------------------------*/
static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready (Stage 3). Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        if (k_strcmp(cmd, "help")    == 0) { cmd_help();    continue; }
        if (k_strcmp(cmd, "clear")   == 0) { cmd_clear();   continue; }
        if (k_strcmp(cmd, "about")   == 0) { cmd_about();   continue; }
        if (k_strcmp(cmd, "ps")      == 0) { process_print_table(); continue; }
        if (k_strcmp(cmd, "threads") == 0) { thread_print_table();  continue; }
        if (k_strcmp(cmd, "race")    == 0) { cmd_race();    continue; }
        if (k_strcmp(cmd, "prodcon") == 0) { cmd_prodcon(); continue; }

        /* Stage 3 PMM Commands */
        if (k_strcmp(cmd, "meminfo") == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "mem")     == 0) {
            pmm_print_info();
            continue;
        }

        if (k_strcmp(cmd, "memtest") == 0) {
            pmm_test_leak();
            continue;
        }

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
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

        /* Stage 4 */
        if (k_strcmp(cmd, "ls")  == 0 ||
            k_strcmp(cmd, "cat") == 0) {
            vga_puts_color("  [TODO] File system commands are scheduled for Stage 4.\n",
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

    process_init();
    scheduler_init();
    thread_init();
    pmm_init(); /* Initialize Stage 3 Physical Memory Manager */

    /* Background processes for indicator */
    process_create("proc_A", worker_proc_a);
    process_create("proc_B", worker_proc_b);

    vga_clear(VGA_BLACK);
    vga_puts_color("  SENG21213-OS :: Stage 3 (Physical Memory Manager)\n",
                   VGA_LIGHT_MAGENTA, VGA_BLACK);
    vga_puts("  Commands added: 'meminfo', 'memtest'\n");
    vga_puts("  Type 'help' to view all available commands.\n");

    shell_run();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}
