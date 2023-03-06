#include <stddef.h>
#include <stdint.h>

#include <kernel/vmem.h>

/* TODO: change to paddr */
void *kmem_head;

void *kmalloc_i(size_t sz, int align) {
    /* align to PAGE_SIZE, assumed to be a power of 2 */
    if(align)
        kmem_head = (void *)((uintptr_t)(kmem_head + PAGE_SIZE-1) & ~(PAGE_SIZE-1));

    void *tmp = kmem_head;
    kmem_head += sz;
    return tmp;
}

void *kmalloc(size_t sz) {
    return kmalloc_i(sz, 0);
}

void *kmalloc_a(size_t sz) {
    return kmalloc_i(sz, 1);
}
