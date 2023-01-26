#include <kernel/def.h>
#include <kernel/boot_opts.h>
#include <sys/stdint.h>
#include <sys/utils.h>

volatile uint16_t *tm_memory = (uint16_t *)0xb8000;

const unsigned char alphanum[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

static uint16_t tm_cursor = 0;
void putc(unsigned char c) {
    tm_memory[tm_cursor++] = 0x0700|c;
}

void puts(char *s) {
    char c;
    while((c = *s++)) putc(c);
}

void kmain(struct kernel_args args) {
    tm_cursor = args.tm_cursor;
    puts("puts in kmain");

    if(args.boot_options & BOOT_OPT_VERBOSE)
        puts(" verbose!");

    for(;;) asm("hlt");
}
