#include <kernel/tty/tm.h>
#include <kernel/vmem.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

void kmain(struct kernel_args *args) {
    /* TODO: remove this */
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    vmem_map((void *)0x200000, (void *)0x400000, 0x03);
    *(uint32_t *)0x400000 = 0xcafebabe;

    printf("%#x at paddr %p\n",
           *(uint32_t *)0x400000, vmem_get_paddr((void *)0x400000));

    vmem_unmap((void *)0x400000);
    /* NOTE: causes a page fault */
    printf("after unmap: %#x\n", *(uint32_t *)0x400000);

    for(;;) asm("hlt");
}
