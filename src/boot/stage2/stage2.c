#include "gdt.h"
#include "idt.h"
#include "irq.h"
#include "isr.h"
#include "tm_io.h"

#ifndef asm
# define asm __asm__ volatile
#endif

const char *str = "hejhej";
void *global_esp;

uint32_t ticks = 0;
void timer_test(struct int_regs *r) {
    /* will print approximately once every second */
    /* (runs at 18.222 Hz) */
    if(ticks++ % 18 == 0)
        tm_puts("tick");
}

/* bootloader main */
void bmain(void *esp) {
    global_esp = esp;

    tm_init();
    gdt_install();
    idt_install();
    irq_install();
    isrs_install();

    irq_handler_install(0, timer_test);

    asm("sti");

    tm_color = 0x0f;
    tm_puts(str);

    asm("int $0x80");
    for(;;) asm("hlt");
}
