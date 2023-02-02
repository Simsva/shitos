#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/tty/tm.h>

void kmain(struct kernel_args *args) {
    /* TODO: remove this */
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("puts in kmain");

    uint8_t i, j, n;
    puts("SGR test:");
    for(i = 0; i < 11; ++i) {
        for(j = 0; j < 10; ++j) {
            n = 10 * i + j;
            if(n > 108) break;
            printf("\033[%1$dm%1$4d\033[m", n);
        }
        putchar('\n');
    }

    printf("hex %#x\n", 0xdeadbeef);
    printf("test d:%1$4d s:%2$s x:%3$#-6x (%3$o)\n", -42, "hello", 0x42);

    for(;;) asm("hlt");
}
