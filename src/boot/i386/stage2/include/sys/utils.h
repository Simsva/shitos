#ifndef SYS_UTILS_H_
#define SYS_UTILS_H_

#include <stdint.h>

#ifndef asm
# define asm __asm__ volatile
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LEN(a) (sizeof(a)/sizeof(a[0]))

static inline uint8_t inb(uint16_t port) {
    uint8_t out;
    asm("inb %1, %0" : "=a" (out) : "dN" (port));
    return out;
}

static inline void outb(uint16_t port, uint8_t data) {
    asm("outb %1, %0" : : "dN" (port), "a" (data));
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // SYS_UTILS_H_
