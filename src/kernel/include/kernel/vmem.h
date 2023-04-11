#ifndef KERNEL_VMEM_H_
#define KERNEL_VMEM_H_

#include <stdint.h>
#include <kernel/ordered_array.h>

#if _ARCH == i386
# include <kernel/arch/i386/paging.h>
# define PAGE_SIZE I386_PAGE_SIZE

# define PAGE_FLAG_PRESENT  0x001
# define PAGE_FLAG_RW       0x002
# define PAGE_FLAG_KERNEL   0x004
# define PAGE_FLAG_ACCESSED 0x008
# define PAGE_FLAG_DIRTY    0x010
#else
/* assumed default page size */
# define PAGE_SIZE 0x1000

# define PAGE_FLAG_PRESENT  0
# define PAGE_FLAG_RW       0
# define PAGE_FLAG_KERNEL   0
# define PAGE_FLAG_ACCESSED 0
# define PAGE_FLAG_DIRTY    0
#endif

#define PAGE_PTR_FLAG_NULL  0x01
#define PAGE_PTR_FLAG_WRITE 0x02

#define VMEM_HEAP_MAGIC      0x0dedbeef
#define VMEM_HEAP_INDEX_SZ   0x20000
#define VMEM_HEAP_INITIAL_SZ 0x100000

typedef struct {
    uint32_t magic;
    uint8_t hole;
    size_t size;
} vmem_header_t;

typedef struct {
    uint32_t magic;
    vmem_header_t *hdr;
} vmem_footer_t;

typedef struct {
    ord_arr_t index;
    void *start, *end, *max;
} vmem_heap_t;

extern vmem_heap_t kheap;

void *vmem_get_paddr(void *vaddr);
void vmem_map(void *paddr, void *vaddr, uint8_t flags);
void vmem_unmap(void *vaddr);
void vmem_init(void);
void vmem_alloc(void *vaddr, uint8_t flags);
int vmem_validate_ptr(void *vaddr, size_t sz, uint8_t flags);

void vmem_heap_create(vmem_heap_t *heap, void *start, void *end, void *max);
void *vmem_heap_alloc(vmem_heap_t *heap, size_t size, uint8_t align);
void vmem_heap_free(vmem_heap_t *heap, void *p);
void vmem_heap_dump(vmem_heap_t *heap);

#endif // KERNEL_VMEM_H_
