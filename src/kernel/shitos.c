#include <kernel/def.h>
#include <kernel/boot_opts.h>
#include <sys/stdint.h>
#include <sys/utils.h>

/* i386 things */
#include "i386/gdt.h"
#include "i386/idt.h"
#include "i386/irq.h"
#include "i386/isr.h"

#include "console/tm.h"

void puts(char *s) {
    char c;
    while((c = *s++)) tm_putc(c);
}

void itos(char *buf, uint8_t num, uint8_t len) {
    do
        buf[--len] = (num%10) + '0';
    while((num /= 10));
    while(len)
        buf[--len] = '0';
}

void kmain(struct kernel_args args) {
    /* TODO: remove this */
    tm_cur_x = args.tm_cursor % 80;
    tm_cur_y = args.tm_cursor / 80;

    /* i386 things */
    asm("cli");
    gdt_install();
    idt_install();
    irq_install();
    isrs_install();
    asm("sti");

    puts("puts in kmain\n");

    uint8_t i, j, n;
    char buf[4] = { '\0' };

    puts("SGR test:\n");
    for(i = 0; i < 11; ++i) {
        for(j = 0; j < 10; ++j) {
            n = 10 * i + j;
            if(n > 108) break;
            itos(buf, n, 3);
            puts("\033["); puts(buf); puts("m ");
            puts(buf); puts("\033[m");
        }
        tm_putc('\n');
    }

    for(;;) asm("hlt");
}
