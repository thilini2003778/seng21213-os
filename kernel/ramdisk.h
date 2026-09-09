#ifndef RAMDISK_H
#define RAMDISK_H

#include "../include/types.h"

#define RAMDISK_BLOCK_SIZE 4096 /* 4 KB blocks */
#define RAMDISK_BLOCKS     256  /* 256 blocks = 1 MB RAM disk */
#define RAMDISK_SIZE       (RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE)

void ramdisk_init(void);
int  ramdisk_read_block(uint32_t block_no, void *buf);
int  ramdisk_write_block(uint32_t block_no, const void *buf);

#endif
