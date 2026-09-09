#ifndef FS_H
#define FS_H

#include "../include/types.h"

#define FS_MAGIC          0x53454E47 /* 'SENG' */
#define MAX_INODES        64
#define MAX_NAME_LEN      28
#define DIRECT_BLOCKS     8
#define MAX_FILE_SIZE     (DIRECT_BLOCKS * 4096) /* 32 KB max file size */
#define MAX_DIRENTS       64

/* Superblock (Block 0) - Lecture L12 §2 */
typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_size;
} superblock_t;

/* Inode Structure (8 direct block pointers) - Lecture L12 §2 */
typedef struct {
    uint32_t size;
    uint32_t direct[DIRECT_BLOCKS];
    uint8_t  used;
    uint8_t  type;
    uint16_t reserved;
} inode_t;

/* Flat Directory Entry (32 bytes) - Lecture L12 §3 */
typedef struct {
    char     name[MAX_NAME_LEN];
    uint32_t inode;
} dirent_t;

void fs_init(void);
int  fs_touch(const char *name);
int  fs_write(const char *name, const char *text);
int  fs_cat(const char *name);
int  fs_rm(const char *name);
void fs_ls(void);
void fs_test_suite(void);

#endif
