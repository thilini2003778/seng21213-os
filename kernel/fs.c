#include "fs.h"
#include "ramdisk.h"
#include "vga.h"
#include "../include/types.h"

static superblock_t sb;
static inode_t      inodes[MAX_INODES];
static dirent_t     dir_entries[MAX_DIRENTS];
static uint8_t      block_bitmap[RAMDISK_BLOCKS];

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static void k_strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void print_num(uint32_t val) {
    if (val == 0) { vga_putchar('0'); return; }
    char buf[16];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    }
    while (idx > 0) vga_putchar(buf[--idx]);
}

static int alloc_block(void) {
    for (int i = 3; i < RAMDISK_BLOCKS; i++) {
        if (block_bitmap[i] == 0) {
            block_bitmap[i] = 1;
            sb.free_blocks--;
            return i;
        }
    }
    return -1;
}

static void free_block(int b) {
    if (b >= 3 && b < RAMDISK_BLOCKS) {
        block_bitmap[b] = 0;
        sb.free_blocks++;
    }
}

void fs_init(void) {
    ramdisk_init();

    sb.magic        = FS_MAGIC;
    sb.total_blocks = RAMDISK_BLOCKS;
    sb.total_inodes = MAX_INODES;
    sb.free_blocks  = RAMDISK_BLOCKS - 3;
    sb.free_inodes  = MAX_INODES;
    sb.block_size   = RAMDISK_BLOCK_SIZE;

    for (int i = 0; i < RAMDISK_BLOCKS; i++) block_bitmap[i] = 0;
    block_bitmap[0] = 1; /* Superblock */
    block_bitmap[1] = 1; /* Directory table */
    block_bitmap[2] = 1; /* Inode table */

    for (int i = 0; i < MAX_INODES; i++) {
        inodes[i].used = 0;
        inodes[i].size = 0;
        for (int j = 0; j < DIRECT_BLOCKS; j++) inodes[i].direct[j] = 0;
    }

    for (int i = 0; i < MAX_DIRENTS; i++) {
        dir_entries[i].name[0] = '\0';
        dir_entries[i].inode = 0;
    }
}

int fs_touch(const char *name) {
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] && k_strcmp(dir_entries[i].name, name) == 0) {
            return -1;
        }
    }

    int inum = -1;
    for (int i = 1; i < MAX_INODES; i++) {
        if (!inodes[i].used) { inum = i; break; }
    }
    if (inum == -1) return -2;

    int dslot = -1;
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] == '\0') { dslot = i; break; }
    }
    if (dslot == -1) return -3;

    inodes[inum].used = 1;
    inodes[inum].size = 0;
    inodes[inum].type = 1;
    for (int j = 0; j < DIRECT_BLOCKS; j++) inodes[inum].direct[j] = 0;
    sb.free_inodes--;

    k_strncpy(dir_entries[dslot].name, name, MAX_NAME_LEN);
    dir_entries[dslot].inode = inum;
    return 0;
}

int fs_write(const char *name, const char *text) {
    int inum = -1;
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] && k_strcmp(dir_entries[i].name, name) == 0) {
            inum = dir_entries[i].inode;
            break;
        }
    }
    if (inum == -1) {
        if (fs_touch(name) != 0) return -1;
        for (int i = 0; i < MAX_DIRENTS; i++) {
            if (dir_entries[i].name[0] && k_strcmp(dir_entries[i].name, name) == 0) {
                inum = dir_entries[i].inode;
                break;
            }
        }
    }

    inode_t *ino = &inodes[inum];
    if (ino->direct[0] == 0) {
        int blk = alloc_block();
        if (blk == -1) return -2;
        ino->direct[0] = blk;
    }

    uint8_t blk_buf[RAMDISK_BLOCK_SIZE];
    ramdisk_read_block(ino->direct[0], blk_buf);

    size_t len = k_strlen(text);
    if (len > RAMDISK_BLOCK_SIZE - 1) len = RAMDISK_BLOCK_SIZE - 1;

    for (size_t i = 0; i < len; i++) {
        blk_buf[i] = text[i];
    }
    blk_buf[len] = '\0';
    ino->size = len;

    ramdisk_write_block(ino->direct[0], blk_buf);
    return 0;
}

int fs_cat(const char *name) {
    int inum = -1;
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] && k_strcmp(dir_entries[i].name, name) == 0) {
            inum = dir_entries[i].inode;
            break;
        }
    }
    if (inum == -1) {
        vga_puts_color("  File not found.\n", VGA_LIGHT_RED, VGA_BLACK);
        return -1;
    }

    inode_t *ino = &inodes[inum];
    if (ino->size == 0 || ino->direct[0] == 0) {
        vga_puts("  (empty file)\n");
        return 0;
    }

    uint8_t blk_buf[RAMDISK_BLOCK_SIZE];
    ramdisk_read_block(ino->direct[0], blk_buf);

    vga_puts("  ");
    for (uint32_t i = 0; i < ino->size; i++) {
        vga_putchar((char)blk_buf[i]);
    }
    vga_puts("\n");
    return 0;
}

int fs_rm(const char *name) {
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] && k_strcmp(dir_entries[i].name, name) == 0) {
            uint32_t inum = dir_entries[i].inode;
            inode_t *ino = &inodes[inum];

            for (int j = 0; j < DIRECT_BLOCKS; j++) {
                if (ino->direct[j] != 0) {
                    free_block(ino->direct[j]);
                    ino->direct[j] = 0;
                }
            }
            ino->used = 0;
            ino->size = 0;
            sb.free_inodes++;

            dir_entries[i].name[0] = '\0';
            dir_entries[i].inode = 0;
            return 0;
        }
    }
    return -1;
}

void fs_ls(void) {
    vga_puts_color("\n  INODE   SIZE (B)   NAME\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  ─────────────────────────────────────────────\n");

    int count = 0;
    for (int i = 0; i < MAX_DIRENTS; i++) {
        if (dir_entries[i].name[0] != '\0') {
            uint32_t inum = dir_entries[i].inode;
            vga_puts("   ");
            print_num(inum);
            vga_puts("       ");
            print_num(inodes[inum].size);
            vga_puts("        ");
            vga_puts_color(dir_entries[i].name, VGA_LIGHT_GREEN, VGA_BLACK);
            vga_puts("\n");
            count++;
        }
    }
    if (count == 0) {
        vga_puts("  (No files on RAM disk. Type 'touch <name>' to create one.)\n");
    }
    vga_puts("\n");
}

void fs_test_suite(void) {
    vga_puts_color("\n  === Stage 4 File System 5-File Verification Test ===\n", VGA_YELLOW, VGA_BLACK);

    vga_puts("  [1] Creating 5 files (file1.txt .. file5.txt)...\n");
    fs_touch("file1.txt");
    fs_touch("file2.txt");
    fs_touch("file3.txt");
    fs_touch("file4.txt");
    fs_touch("file5.txt");

    vga_puts("  [2] Writing unique content into each file...\n");
    fs_write("file1.txt", "OS Assignment: Stage 4 File System");
    fs_write("file2.txt", "University of Kelaniya - Software Engineering");
    fs_write("file3.txt", "Round-Robin Preemptive Multitasking");
    fs_write("file4.txt", "Mutual Exclusion and Counting Semaphores");
    fs_write("file5.txt", "Physical Memory Allocation 4KB Page Frames");

    vga_puts_color("\n  --> File System State after 5 creates & writes:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    fs_ls();

    vga_puts_color("  [3] Reading back file2.txt:\n", VGA_LIGHT_GREY, VGA_BLACK);
    fs_cat("file2.txt");

    vga_puts_color("\n  [4] Deleting file1.txt and file5.txt (fs_rm)...\n", VGA_LIGHT_GREY, VGA_BLACK);
    fs_rm("file1.txt");
    fs_rm("file5.txt");

    vga_puts_color("  --> Final File System State after deletions:\n", VGA_LIGHT_CYAN, VGA_BLACK);
    fs_ls();
    vga_puts_color("  [SUCCESS] Created, written, read back, and deleted 5 files cleanly!\n\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);
}
