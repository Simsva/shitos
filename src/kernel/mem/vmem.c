#include <kernel/vmem.h>

#include <kernel/kmem.h>
#include <assert.h>

#if _ARCH == i386
# include <kernel/arch/i386/paging.h>
#endif

vmem_heap_t *kheap;

void *vmem_get_paddr(void *vaddr) {
#if _ARCH == i386
    return i386_get_paddr(vaddr);
#else
    puts("vmem_get_paddr not implemented for the current architecture");
    return NULL;
#endif
}

void vmem_map(void *paddr, void *vaddr, uint8_t flags) {
#if _ARCH == i386
    i386_map_page(paddr, vaddr, flags);
#else
    puts("vmem_map not implemented for the current architecture");
#endif
}

void vmem_unmap(void *vaddr) {
#if _ARCH == i386
    i386_unmap_page(vaddr);
#else
    puts("vmem_unmap not implemented for the current architecture");
#endif
}

/* initialize everything necessary for virtual memory and allocation */
void vmem_init(void) {
#if _ARCH == i386
    /* i386_init_paging(); */
#else
    puts("vmem_init not implemented for the current architecture");
#endif

    /* create heap */
    kheap = (vmem_heap_t *)kmalloc(sizeof(vmem_heap_t));
    kmem_head = (void *)((uintptr_t)(kmem_head + PAGE_SIZE-1) & ~(PAGE_SIZE-1));
    vmem_heap_create(kheap, kmem_head, kmem_head+VMEM_HEAP_INITIAL_SZ,
                     (void *)0xcffff000);
}

/* heap */
static int vmem_header_compar(ord_arr_type_t a, ord_arr_type_t b) {
    return ((vmem_header_t *)a)->size < ((vmem_header_t *)b)->size;
}

void vmem_heap_create(vmem_heap_t *heap, void *start, void *end, void *max) {
    /* vmem_heap_t *heap = (vmem_heap_t *)kmalloc(sizeof(vmem_heap_t)); */

    assert((uint32_t)start % PAGE_SIZE == 0);
    assert((uint32_t)end % PAGE_SIZE == 0);

    ord_arr_place(&heap->index, start, VMEM_HEAP_INDEX_SZ, &vmem_header_compar);
    start += sizeof(ord_arr_type_t) * VMEM_HEAP_INDEX_SZ;

    /* align start */
    start = (void *)((uintptr_t)(start + PAGE_SIZE-1) & ~(PAGE_SIZE-1));

    heap->start = start;
    heap->end = end;
    heap->max = max;

    /* create one large hole */
    vmem_header_t *hole = (vmem_header_t *)start;
    hole->size = end - start;
    hole->magic = VMEM_HEAP_MAGIC;
    hole->hole = 1;
    ord_arr_insert(&heap->index, hole);
}
