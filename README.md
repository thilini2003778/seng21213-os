\# SENG21213-OS — Project Submission



\*\*Course\*\*: SENG 21213 – Computer Architecture \& Operating Systems  

\*\*Student Name\*\*: Thilini Kanchana  

\*\*Student Number\*\*: SE/2023/068  



\---



\## Project Overview



This is a custom 32-bit x86 Operating System developed for the SENG 21213 course. The project involves building an OS from scratch on bare metal, starting from a minimal 512-byte MBR bootloader, transitioning into a 32-bit protected-mode kernel, and implementing preemptive multitasking, kernel threads, synchronization primitives, physical memory management, and an in-memory file system.



\---



\## Implemented Features



The following features have been successfully implemented across all assignment stages:



\- \*\*Bootloader (Stage 0)\*\*: 16-bit real mode to 32-bit protected mode transition, 3-entry Global Descriptor Table (GDT) setup, and BIOS INT 0x13 disk read.

\- \*\*VGA Driver (Stage 0)\*\*: 80×25 text-mode display driver writing directly to physical address `0xB8000`.

\- \*\*Keyboard Driver (Stage 0)\*\*: PS/2 keyboard controller polling driver (ports `0x64` and `0x60`) translating Set-1 scancodes into ASCII.

\- \*\*Interactive Shell (Stage 0)\*\*: Built-in command-line interface (`ksh`) running inside the kernel.

\- \*\*Process Management \& Scheduler (Stage 1)\*\*: Process Control Block (`pcb\_t`), 4 KB stack per process, Intel 8253 PIT timer firing IRQ0 at 100 Hz, `switch.asm` (PUSHAD/POPAD) context switcher, and a preemptive Round-Robin scheduler. Includes `ps` and `kill` commands.

\- \*\*Kernel Threads \& Synchronization (Stage 2)\*\*: Lightweight kernel threads sharing address space (`thread\_create`), blocking Mutex locks (`mutex\_lock`/`mutex\_unlock`), and counting Semaphores (`sem\_wait`/`sem\_signal`). Demonstrates `myglobal` race condition and 3-semaphore Bounded-Buffer Producer-Consumer.

\- \*\*Physical Memory Manager (Stage 3)\*\*: Page frame allocator managing 32 MB of RAM in 4 KB frames using a 1 KB bitmap (8,192 frames). Includes `meminfo` and a 100-frame leak test (`memtest`) verifying zero leaks.

\- \*\*RAM Disk File System (Stage 4)\*\*: 1 MB block-based RAM disk residing at physical address `0x00200000` (2 MB extended memory). Superblock (magic `0x53454E47`), Inodes with 8 direct block pointers (32 KB max), flat directory table, and POSIX file operations (`ls`, `touch`, `write`, `cat`, `rm`, `fstest`).



\---



\## Project Structure



```text

seng21213-os/

├── boot/

│   └── boot.asm          ← MBR Bootloader (NASM, 16-bit → 32-bit transition)

├── kernel/

│   ├── kernel\_entry.asm  ← Protected-mode entry

│   ├── kernel.c          ← Main kernel, shell loop, and command dispatch

│   ├── switch.asm        ← Context switch stub and timer ISR (Stage 1)

│   ├── vga.c / vga.h     ← VGA text-mode driver

│   ├── keyboard.c / .h   ← PS/2 keyboard driver

│   ├── process.c / .h    ← Process table (PCB) \& management (Stage 1)

│   ├── scheduler.c / .h  ← Preemptive Round-Robin scheduler \& PIT (Stage 1)

│   ├── thread.c / .h     ← Kernel threads (Stage 2)

│   ├── mutex.c / .h      ← Mutex locks (Stage 2)

│   ├── semaphore.c / .h  ← Counting semaphores (Stage 2)

│   ├── pmm.c / .h        ← Physical Memory Manager \& frame bitmap (Stage 3)

│   ├── ramdisk.c / .h    ← 1 MB RAM disk driver (Stage 4)

│   └── fs.c / fs.h       ← Inode file system \& directory operations (Stage 4)

├── include/

│   ├── types.h           ← Standard integer and primitive types

│   └── io.h              ← Inline x86 I/O port helpers (inb, outb)

├── linker.ld             ← Linker script (places kernel at 0x10000)

├── Makefile              ← Build system

├── .gitignore            ← Excludes build/ and binary disk images (\*.img)

└── Dockerfile            ← Reproducible build environment

