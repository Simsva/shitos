#ifndef KERNEL_VMEM_H_
#define KERNEL_VMEM_H_

#include <stdint.h>
#include <kernel/ordered_array.h>

#if _ARCH == i386
# include <kernel/arch/i386/vmem.h>
#else
# error "Unsupported architecture"
#endif

/* vmem_frame_alloc and vmem_frame_map_addr flags */
#define VMEM_FLAG_WRITE        0x0001
#define VMEM_FLAG_KERNEL       0x0002
#define VMEM_FLAG_WRITETHROUGH 0x0004
#define VMEM_FLAG_NOCACHE      0x0008
#define VMEM_FLAG_SIZE         0x0010
#define VMEM_FLAG_PAT          0x0020
#define VMEM_FLAG_GLOBAL       0x0040

/* vmem_validate_user_ptr flags */
#define VMEM_PTR_FLAG_NULL  0x0001
#define VMEM_PTR_FLAG_WRITE 0x0002

/* vmem_get_page flags */
#define VMEM_GET_CREATE     0x0001
#define VMEM_GET_KERNEL     0x0002

/* heap constants */
#define VMEM_HEAP_MAGIC      0x0dedbeef
#define VMEM_HEAP_INDEX_SZ   0x20000
#define VMEM_HEAP_INITIAL_SZ 0x100000

/* addresses */
#define VMEM_MAP_PAGES_MEMORY 0xf0000000

/* heap types */
typedef struct {
    uint32_t magic;
    uint8_t align; /* aligned on 1<<align boundaries */
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
    uint8_t kernel;
} vmem_heap_t;

/* kernel heap */
extern vmem_heap_t kheap;

/* pages/frames */
void      vmem_init(void);
void      vmem_frame_set(uintptr_t frame_addr);
void      vmem_frame_unset(uintptr_t frame_addr);
int       vmem_frame_test(uintptr_t frame_addr);
uintptr_t vmem_frame_find_first(void);
uintptr_t vmem_frame_find_first_n(unsigned n);
void      vmem_frame_alloc(page_t *page, unsigned flags);
void      vmem_frame_map_addr(page_t *page, unsigned flags, uintptr_t paddr);
void     *vmem_map_vaddr(uintptr_t paddr);
void     *vmem_map_vaddr_n(uintptr_t paddr, size_t sz);
void      vmem_frame_free(page_t *page);
uintptr_t vmem_get_paddr(page_t *dir, uintptr_t vaddr);
page_t   *vmem_get_page_other(page_directory_t *dir, uintptr_t vaddr, unsigned flags);
page_t   *vmem_get_page(uintptr_t vaddr, unsigned flags);

page_directory_t *vmem_clone_dir(page_directory_t *src);
void              vmem_set_dir(page_directory_t *dir);
void              vmem_free_dir(page_directory_t *dir);

int vmem_validate_user_ptr(const void *vaddr, size_t sz, unsigned flags);

/* heap */
void  vmem_heap_create(vmem_heap_t *heap, void *start, void *end, void *max, size_t index_sz, uint8_t kernel);
void *vmem_heap_alloc(vmem_heap_t *heap, size_t size, uint8_t align);
void *vmem_heap_realloc(vmem_heap_t *heap, void *old, size_t size);
void  vmem_heap_free(vmem_heap_t *heap, void *p);
void  vmem_heap_dump(vmem_heap_t *heap);

#endif // KERNEL_VMEM_H_
