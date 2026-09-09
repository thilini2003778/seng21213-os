#include "ramdisk.h"

/* Place the 1 MB RAM disk at 2 MB physical RAM (Safe Extended Memory) */
#define RAMDISK_ADDR  0x00200000
static uint8_t * const disk = (uint8_t *)RAMDISK_ADDR;

void ramdisk_init(void) {
    for (uint32_t i = 0; i < RAMDISK_SIZE; i++) {
        disk[i] = 0;
    }
}

int ramdisk_read_block(uint32_t block_no, void *buf) {
    if (block_no >= RAMDISK_BLOCKS) return -1;
    uint8_t *src = &disk[block_no * RAMDISK_BLOCK_SIZE];
    uint8_t *dst = (uint8_t *)buf;
    for (int i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
    return 0;
}

int ramdisk_write_block(uint32_t block_no, const void *buf) {
    if (block_no >= RAMDISK_BLOCKS) return -1;
    const uint8_t *src = (const uint8_t *)buf;
    uint8_t *dst = &disk[block_no * RAMDISK_BLOCK_SIZE];
    for (int i = 0; i < RAMDISK_BLOCK_SIZE; i++) {
        dst[i] = src[i];
    }
    return 0;
}
