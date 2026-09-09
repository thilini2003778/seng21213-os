#include "pmm.h"
#include "vga.h"
#include "../include/types.h"

/* Frame bitmap: 1 bit per 4 KB frame */
static uint8_t  pmm_bitmap[BITMAP_SIZE];
static uint32_t total_frames = TOTAL_FRAMES;
static uint32_t used_frames  = 0;

/* Bitmap bit manipulation helpers */
static inline void set_bit(uint32_t frame) {
    pmm_bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void clear_bit(uint32_t frame) {
    pmm_bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline int test_bit(uint32_t frame) {
    return (pmm_bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

static void print_uint(uint32_t val) {
    if (val == 0) { vga_putchar('0'); return; }
    char buf[16];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    }
    while (idx > 0) vga_putchar(buf[--idx]);
}

/* Initialize Physical Memory Manager */
void pmm_init(void) {
    /* Initially mark all frames as FREE (0) */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0;
    }
    used_frames = 0;

    /* Reserve first 1 MB (256 frames) for BIOS, VGA buffer, and hardware */
    for (uint32_t i = 0; i < 256; i++) {
        set_bit(i);
        used_frames++;
    }

    /* Reserve kernel area (frames 256 to 384 = up to ~1.5 MB) */
    for (uint32_t i = 256; i < 384; i++) {
        set_bit(i);
        used_frames++;
    }
}

/* Allocate a single 4 KB physical frame (First-Fit scan) */
uint32_t pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < total_frames; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            used_frames++;
            return i * PAGE_SIZE; /* Return physical byte address */
        }
    }
    return 0; /* Out of physical memory */
}

/* Free a physical frame */
void pmm_free_frame(uint32_t paddr) {
    uint32_t frame = paddr / PAGE_SIZE;
    if (frame < total_frames && test_bit(frame)) {
        clear_bit(frame);
        used_frames--;
    }
}

uint32_t pmm_get_total_frames(void) { return total_frames; }
uint32_t pmm_get_used_frames(void)  { return used_frames; }
uint32_t pmm_get_free_frames(void)  { return total_frames - used_frames; }

/* Display memory statistics (for 'meminfo' command) */
void pmm_print_info(void) {
    uint32_t free_frames = total_frames - used_frames;

    vga_puts_color("\n  === Physical Memory Manager (PMM) Info ===\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");
    vga_puts("  Page Frame Size : 4 KB (4096 bytes)\n");
    vga_puts("  Total RAM       : "); print_uint(TOTAL_MEMORY / (1024 * 1024)); vga_puts(" MB (");
    print_uint(total_frames); vga_puts(" frames)\n");

    vga_puts_color("  Used Memory     : ", VGA_LIGHT_CYAN, VGA_BLACK);
    print_uint((used_frames * 4) / 1024); vga_puts(" MB (");
    print_uint(used_frames); vga_puts(" frames)\n");

    vga_puts_color("  Free Memory     : ", VGA_LIGHT_GREEN, VGA_BLACK);
    print_uint((free_frames * 4) / 1024); vga_puts(" MB (");
    print_uint(free_frames); vga_puts(" frames)\n\n");
}

/* Allocate and free 100 frames in a loop and verify zero leaks */
void pmm_test_leak(void) {
    vga_puts_color("\n  [PMM Test] Allocating 100 physical frames...\n", VGA_LIGHT_CYAN, VGA_BLACK);
    uint32_t before_free = pmm_get_free_frames();

    uint32_t frames[100];
    for (int i = 0; i < 100; i++) {
        frames[i] = pmm_alloc_frame();
        if (frames[i] == 0) {
            vga_puts_color("  [FAIL] Out of memory during allocation!\n", VGA_LIGHT_RED, VGA_BLACK);
            return;
        }
    }

    vga_puts("  [PMM Test] Allocated 100 frames (400 KB). Freeing them back...\n");
    for (int i = 0; i < 100; i++) {
        pmm_free_frame(frames[i]);
    }

    uint32_t after_free = pmm_get_free_frames();
    if (before_free == after_free) {
        vga_puts_color("  [SUCCESS] 100 frames allocated and freed cleanly. Zero leaks detected!\n\n",
                       VGA_LIGHT_GREEN, VGA_BLACK);
    } else {
        vga_puts_color("  [FAIL] Memory leak detected!\n\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}
