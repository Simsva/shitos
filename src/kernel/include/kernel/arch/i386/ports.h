#ifndef I386_PORTS_H_
#define I386_PORTS_H_

#include <features.h>
#include <kernel/arch/i386/ports.h>
#include <kernel/utils.h>

inline unsigned char inportb(unsigned short port) {
    unsigned char o;
    asm volatile("inb %1, %0" : "=a"(o) : "dN"(port));
    return o;
}

inline void outportb(unsigned short port, unsigned char data) {
    asm volatile("outb %1, %0" :: "dN"(port), "a"(data));
}

inline unsigned short inportw(unsigned short port) {
    unsigned short o;
    asm volatile("inw %1, %0" : "=a"(o) : "dN"(port));
    return o;
}

inline void outportw(unsigned short port, unsigned short data) {
    asm volatile("outw %1, %0" :: "dN"(port), "a"(data));
}

inline unsigned int inportl(unsigned short port) {
    unsigned int o;
    asm volatile("inl %1, %0" : "=a"(o) : "dN"(port));
    return o;
}

inline void outportl(unsigned short port, unsigned int data) {
    asm volatile("outl %1, %0" :: "dN"(port), "a"(data));
}

inline void inportsm(unsigned short port, unsigned char *data, unsigned long sz) {
    asm volatile("rep insw" : "+D"(data), "+c"(sz) : "d"(port) : "memory");
}

inline void outportsm(unsigned short port, unsigned char *data, unsigned long sz) {
    asm volatile("rep outsw" : "+S"(data), "+c"(sz) : "d"(port));
}

inline void io_wait(void) {
    outportb(0x80, 0);
}

#endif // I386_PORTS_H_
