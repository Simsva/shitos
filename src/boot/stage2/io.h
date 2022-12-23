#ifndef IO_H_
#define IO_H_

#include "stdint.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t out;
    __asm__ volatile("inb %1, %0" : "=a" (out) : "dN" (port));
    return out;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %1, %0" : : "dN" (port), "a" (data));
}

#endif // IO_H_
