#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PAGE_SIZE       4096       /* 4 KB per physical frame */
#define TOTAL_MEMORY    (32 * 1024 * 1024) /* 32 MB QEMU RAM */
#define TOTAL_FRAMES    (TOTAL_MEMORY / PAGE_SIZE) /* 8,192 frames */
#define BITMAP_SIZE     (TOTAL_FRAMES / 8)         /* 1,024 bytes */

/* Public PMM API - Lecture L11 */
void     pmm_init(void);
uint32_t pmm_alloc_frame(void);
void     pmm_free_frame(uint32_t paddr);
void     pmm_print_info(void);
void     pmm_test_leak(void);

uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_used_frames(void);
uint32_t pmm_get_free_frames(void);

#endif
