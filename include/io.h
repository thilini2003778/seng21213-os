#ifndef IO_H
#define IO_H

#include "types.h"

/* Output a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Wait a very short CPU cycle for slow I/O ports */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif
