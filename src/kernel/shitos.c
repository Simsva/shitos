#include <kernel/tty/tm.h>
#include <kernel/vmem.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/arch/i386/paging.h>

#include <kernel/ordered_array.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

extern uint32_t frame_count;
extern uint32_t *frames;

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("nframes: %1$x %1$d\nframes %2$p to %3$p\n",
           frame_count, frames, frames + ((frame_count + 31)>>5));

    printf("first frame: %#x\n", frame_find_first());

    ord_arr_type_t bruh[8] = { NULL };
    ord_arr_t test;
    ord_arr_place(&test, bruh, 8, ord_arr_stdcompar);
    ord_arr_insert(&test, (ord_arr_type_t)32);
    ord_arr_insert(&test, (ord_arr_type_t)8);
    ord_arr_insert(&test, (ord_arr_type_t)16);
    ord_arr_remove(&test, 1);
    printf("%#x\n", (uint32_t)ord_arr_get(&test, 1));

    for(;;) asm("hlt");
}
