#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdint.h>

#include <stdio.h>

/* i386 things */
#include "arch/i386/gdt.h"
#include "arch/i386/idt.h"
#include "arch/i386/irq.h"
#include "arch/i386/isr.h"

#include <kernel/tty/tm.h>

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

    puts("puts in kmain");

    /* uint8_t i, j, n; */
    /* char buf[4] = { '\0' }; */

    /* puts("SGR test:"); */
    /* for(i = 0; i < 11; ++i) { */
    /*     for(j = 0; j < 10; ++j) { */
    /*         n = 10 * i + j; */
    /*         if(n > 108) break; */
    /*         itos(buf, n, 3); */
    /*         printf("\033["); printf(buf); printf("m "); */
    /*         printf(buf); printf("\033[m"); */
    /*     } */
    /*     putchar('\n'); */
    /* } */

    printf("hex %#x\n", 0xdeadbeef);
    printf("test d:%4d s:%s x:%#-6x (%o)\n", -42, "hello", 0x42, 0755);

    for(;;) asm("hlt");
}
