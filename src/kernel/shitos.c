#include <kernel/tty/tm.h>
#include <kernel/vmem.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    /* NOTE: causes page fault */
    *(uint32_t *)0x400000 = 0xcafebabe;

    for(;;) asm("hlt");
}
