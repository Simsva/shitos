#include <stddef.h>
#include <stdint.h>

#include <kernel/vmem.h>

void *kmem_head;

void *kmalloc_i(size_t sz, int align) {
    /* if the kernel heap exists, allocate in that */
    if(kheap.start)
        return vmem_heap_alloc(&kheap, sz, align);

    /* align to PAGE_SIZE, assumed to be a power of 2 */
    if(align)
        kmem_head = (void *)((uintptr_t)(kmem_head + PAGE_SIZE-1) & ~(PAGE_SIZE-1));

    void *tmp = kmem_head;
    kmem_head += sz;
    return tmp;
}

/**
 * Allocate memory on the kernel heap.
 */
void *kmalloc(size_t sz) {
    return kmalloc_i(sz, 0);
}

/**
 * Allocate page aligned memory on the kernel heap.
 */
void *kmalloc_a(size_t sz) {
    return kmalloc_i(sz, PAGE_BITS);
}

/**
 * Resize an allocation, respects alignment.
 */
void *krealloc(void *ptr, size_t sz) {
    return vmem_heap_realloc(&kheap, ptr, sz);
}

/* should never be called on memory allocated pre-heap */
void kfree(void *p) {
    vmem_heap_free(&kheap, p);
}
