#include <kernel/tty/tm.h>
#include <kernel/vmem.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include "arch/i386/paging.h"
#include "arch/i386/keyboard.h"

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

extern uint32_t frame_count;
extern uint32_t *frames;

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;
    
    printf("\033[2J\033[H");
    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("nframes: %1$x %1$d\nframes %2$p to %3$p\n",
           frame_count, frames, frames + ((frame_count + 31)>>5));

    printf("first frame: %#x\n", frame_find_first());
    
        /* NOTE: causes page fault */
    //*(uint32_t *)0x400000 = 0xcafebabe;
    
    keyboard_init(); 
    
    for(;;) asm("hlt");
}
