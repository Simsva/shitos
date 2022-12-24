#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "tm_io.h"

#ifndef asm
# define asm __asm__ volatile
#endif

const char *str;
void *global_esp;

void bmain(void *esp) {
    global_esp = esp;

    _gdt_install();
    _idt_install();
    _isrs_install();
    tm_init();

    asm("sti");

    /* NOTE: declaring with a value does not work for some reason */
    str = "hejhej";
    tm_color = 0x0f;
    tm_puts(str);

    /* FIXME: throws a double fault eventually */
    asm("int $0x80");
    for(;;) asm("hlt");
}
