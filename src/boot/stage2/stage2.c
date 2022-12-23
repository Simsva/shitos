#include "gdt.h"

#ifndef asm
# define asm __asm__ volatile
#endif

void puts_pe(const char *str);

const char *str;

void bmain() {
    _gdt_install();
    /* TODO: IDT + IRQ */

    asm("sti");

    str = "hejhej";
    for(;;) {
        puts_pe(str);
        /* asm("hlt"); */
    }
}
