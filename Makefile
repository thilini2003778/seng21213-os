# =============================================================================
# SENG21213-OS :: Makefile (Stage 2 - Threads, Mutex, Semaphores)
# =============================================================================

AS       := nasm
ASFLAGS  := -f elf32
CC       := gcc
LD       := ld
CFLAGS   := -m32 -ffreestanding -fno-stack-protector -fno-pie -nostdlib \
            -std=gnu99 -Wall -Wextra -O2 -I./include
LDFLAGS  := -m elf_i386 -nostdlib

BOOT_SRC := boot/boot.asm
BOOT_BIN := boot/boot.bin

ASM_OBJS := build/kernel_entry.o build/switch.o
C_SRCS   := kernel/kernel.c    \
            kernel/vga.c       \
            kernel/keyboard.c  \
            kernel/process.c   \
            kernel/scheduler.c \
            kernel/thread.c    \
            kernel/mutex.c     \
            kernel/semaphore.c
C_OBJS   := $(patsubst kernel/%.c, build/%.o, $(C_SRCS))

KERNEL_ELF := build/kernel.elf
KERNEL_BIN := build/kernel.bin
OS_IMAGE   := seng21213-os.img

.PHONY: all clean run

all: $(OS_IMAGE)
	@echo ""
	@echo "  ✓  Build successful → $(OS_IMAGE)"
	@echo "  →  Run with:  make run"
	@echo ""

$(BOOT_BIN): $(BOOT_SRC)
	@mkdir -p build
	@echo "  [AS]  $<"
	$(AS) -f bin $< -o $@

build/kernel_entry.o: kernel/kernel_entry.asm
	@mkdir -p build
	@echo "  [AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

build/switch.o: kernel/switch.asm
	@mkdir -p build
	@echo "  [AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

build/%.o: kernel/%.c
	@mkdir -p build
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(ASM_OBJS) $(C_OBJS)
	@echo "  [LD]  $@"
	$(LD) $(LDFLAGS) -T linker.ld $^ -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  [OBJCOPY] $@"
	objcopy -O binary $< $@

$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "  [IMG]  Creating $(OS_IMAGE)..."
	dd if=/dev/zero bs=512 count=2880 of=$(OS_IMAGE) 2>/dev/null
	dd if=$(BOOT_BIN) conv=notrunc bs=512 count=1 of=$(OS_IMAGE) 2>/dev/null
	dd if=$(KERNEL_BIN) conv=notrunc bs=512 seek=1 of=$(OS_IMAGE) 2>/dev/null
	@echo "  [IMG]  $(OS_IMAGE) ready ($(shell wc -c < $(KERNEL_BIN)) kernel bytes)"

run: $(OS_IMAGE)
	@echo "  Starting QEMU..."
	qemu-system-i386 -display curses -drive format=raw,file=$(OS_IMAGE) -m 32M

clean:
	rm -rf build $(OS_IMAGE) $(BOOT_BIN)
	@echo "  Cleaned."
