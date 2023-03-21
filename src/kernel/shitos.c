#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/vmem.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    void *a, *b, *c;
    a = vmem_heap_alloc(&kheap, 4*sizeof(uint32_t), 1);
    b = vmem_heap_alloc(&kheap, 40, 0);
    c = vmem_heap_alloc(&kheap, 0x1000, 0);

    printf("a:%p\nb:%p\nc:%p\n", a, b, c);

    for(;;) asm("hlt");
}
