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
    a = vmem_heap_alloc(&kheap, 0x8000-sizeof(vmem_footer_t)-2*sizeof(vmem_header_t)+1, 0);
    c = vmem_heap_alloc(&kheap, 32, 0);
    b = vmem_heap_alloc(&kheap, 64, 0);
    vmem_heap_free(&kheap, c);
    vmem_heap_dump(&kheap);
    c = vmem_heap_alloc(&kheap, 32, 1);

    printf("a:%p\nb:%p\nc:%p\n", a, b, c);
    vmem_heap_dump(&kheap);

    for(;;) asm("hlt");
}
