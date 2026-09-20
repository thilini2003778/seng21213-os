# SENG21213-OS — Project Submission

**Course**: SENG 21213 – Computer Architecture & Operating Systems  
**Student Name**: Thilini Kanchana  
**Student Number**: SE/2023/068  

---

## Project Overview

This is a custom x86 Operating System developed for the SENG 21213 course. The project involves building an OS from scratch on bare metal, starting from a minimal bootloader and transitioning into a 32-bit protected-mode kernel, implementing multitasking, synchronization primitives, physical memory management, and an in-memory file system.

### Implemented Features

Currently, the following features have been successfully implemented:
- **Bootloader (Stage 0)**: 16-bit real mode to 32-bit protected mode transition, and Global Descriptor Table (GDT) setup.
- **VGA Driver (Stage 0)**: 80×25 text-mode display driver writing directly to `0xB8000`.
- **Keyboard Driver (Stage 0)**: PS/2 keyboard polling driver for user input.
- **Interactive Shell (Stage 0)**: A command-line interface running in the kernel (`ksh`).
- **Multitasking & Scheduler (Stage 1)**: Process Control Block (`pcb_t`), 100 Hz PIT timer interrupt, and Round-Robin scheduler with `ps` and `kill` commands.
- **Threading & Synchronization (Stage 2)**: Lightweight kernel threads, Mutex locks with sleep-wait queue, and Counting Semaphores, with `race` and `prodcon` demonstration tests.
- **Physical Memory Management (Stage 3)**: Page frame allocator managing 32 MB RAM in 4 KB frames using a 1 KB bitmap, with `meminfo` and `memtest` (verified zero memory leaks).
- **RAM Disk File System (Stage 4)**: Inode-based file system with superblock (`0x53454E47`), direct block pointers, directory table, and POSIX-like file commands (`ls`, `touch`, `write`, `cat`, `rm`, `fstest`).

---

## Project Structure

```text
seng21213-os/
├── boot/
│   └── boot.asm          ← MBR Bootloader (NASM, 16-bit → 32-bit transition)
├── kernel/
│   ├── kernel_entry.asm  ← Protected-mode entry
│   ├── kernel.c          ← Main kernel, shell loop, and command dispatch
│   ├── switch.asm        ← Context switch stub and timer ISR (Stage 1)
│   ├── vga.c / vga.h     ← VGA text-mode driver
│   ├── keyboard.c / .h   ← PS/2 keyboard driver
│   ├── process.c / .h    ← Process table (PCB) & management (Stage 1)
│   ├── scheduler.c / .h  ← Preemptive Round-Robin scheduler (Stage 1)
│   ├── thread.c / .h     ← Kernel threads (Stage 2)
│   ├── mutex.c / .h      ← Mutex locks (Stage 2)
│   ├── semaphore.c / .h  ← Counting semaphores (Stage 2)
│   ├── pmm.c / .h        ← Physical Memory Manager & bitmap (Stage 3)
│   ├── ramdisk.c / .h    ← 1 MB RAM disk driver (Stage 4)
│   └── fs.c / fs.h       ← Inode file system & directory operations (Stage 4)
├── include/
│   ├── types.h           ← Primitive types
│   └── io.h              ← Inline x86 I/O port helpers
├── linker.ld             ← Linker script
├── Makefile              ← Build system
├── .gitignore            ← Excludes build/ and binary disk images (*.img)
└── Dockerfile            ← Reproducible build environment
```

---

## Quick Start (How to Build and Run)

### Option A: Native Linux / WSL2 (Recommended)

Ensure you have the required dependencies installed:
```bash
# Ubuntu/Debian dependencies
sudo apt update
sudo apt install -y nasm gcc gcc-multilib binutils qemu-system-x86 make
```

Build and run using the Makefile:
```bash
make clean && make all
make run
```
*(Or run in curses terminal mode: `make run-curses`)*

### Option B: Docker

Using Docker ensures a reproducible build environment across all platforms.

```bash
# 1. Build the Docker image once:
docker build -t seng21213-os-builder .

# 2. Compile the OS using the container:
docker run --rm -v "$(pwd)":/os seng21213-os-builder

# 3. Run the compiled OS image in QEMU:
qemu-system-i386 -drive format=raw,file=seng21213-os.img -m 32M
```

### Option C: macOS (Homebrew)

```bash
brew install nasm x86_64-elf-binutils qemu

# Note: You also need an i686-elf-gcc cross-compiler.
make all
make run
```

---

## Available Shell Commands

| Command | Description |
| :--- | :--- |
| `help` | Show available commands |
| `clear` | Clear the screen |
| `version` | Display OS version |
| `echo <text>` | Print text to terminal |
| `ps` | List all running processes and states |
| `kill <pid>` | Terminate a process |
| `race` | Demonstrate race condition (without vs with mutex) |
| `prodcon` | Run Producer-Consumer bounded-buffer demo |
| `meminfo` | Show physical memory usage and bitmap stats |
| `memtest` | Run 100-frame memory allocation leak test |
| `ls` | List files on the RAM disk |
| `touch <name>` | Create a new file |
| `write <name> <text>` | Write text into a file |
| `cat <name>` | Display file contents |
| `rm <name>` | Delete a file from RAM disk |
| `fstest` | Run automated filesystem test suite |
| `reboot` | Soft reboot the system |

---

## Debugging Tips

To run the OS in debug mode with GDB:

```bash
# Terminal 1: Run QEMU in debug mode
make run-debug

# Terminal 2: Connect GDB
gdb
(gdb) target remote :1234
(gdb) set architecture i386
(gdb) symbol-file build/kernel.elf
(gdb) break kernel_main
(gdb) continue
```

To inspect the raw disk image:
```bash
xxd seng21213-os.img | head -32        # View MBR
xxd seng21213-os.img | grep -c aa55    # Verify boot signature
```

---

## Milestone Releases

All assignment milestones are tagged and published on GitHub Releases:

| Tag | Stage | Description |
| :--- | :--- | :--- |
| [`v0.1-stage0`](https://github.com/thilini2003778/seng21213-os/releases/tag/v0.1-stage0) | Stage 0 | Bootloader, Protected Mode, VGA Driver, Shell |
| [`v0.2-stage1`](https://github.com/thilini2003778/seng21213-os/releases/tag/v0.2-stage1) | Stage 1 | PCB, Round-Robin Scheduler, PIT Timer, `ps`, `kill` |
| [`v0.3-stage2`](https://github.com/thilini2003778/seng21213-os/releases/tag/v0.3-stage2) | Stage 2 | Kernel Threads, Mutex Locks, Counting Semaphores |
| [`v0.4-stage3`](https://github.com/thilini2003778/seng21213-os/releases/tag/v0.4-stage3) | Stage 3 | Bitmap Physical Memory Manager, `meminfo`, `memtest` |
| [`v0.5-stage4`](https://github.com/thilini2003778/seng21213-os/releases/tag/v0.5-stage4) | Stage 4 | RAM Disk Filesystem, Inodes, File API, `fstest` |
