#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdint.h>
#include <stdio.h>

#include <kernel/tty/tm.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

void kmain(struct kernel_args *args) {
    /* TODO: remove this */
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    for(;;) asm("hlt");
}
