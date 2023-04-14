#include <kernel/vmem.h>
#include <kernel/kmem.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <features.h>
#include <ansi.h>

#include <kernel/arch/i386/idt.h>
#include <kernel/arch/i386/error.h>

/* page directory flags */
#define PD_FLAG_PRESENT      0x001
#define PD_FLAG_WRITE        0x002
#define PD_FLAG_USER         0x004
#define PD_FLAG_WRITETHROUGH 0x008
#define PD_FLAG_NOCACHE      0x010
#define PD_FLAG_ACCESSED     0x020
#define PD_FLAG_SIZE         0x080

#define PD_FLAGS_KERNELMODE  (PD_FLAG_PRESENT|PD_FLAG_WRITE)
#define PD_FLAGS_USERMODE    (PD_FLAG_PRESENT|PD_FLAG_WRITE|PD_FLAG_USER)

/* page table flags */
#define PT_FLAG_PRESENT      0x001
#define PT_FLAG_WRITE        0x002
#define PT_FLAG_USER         0x004
#define PT_FLAG_WRITETHROUGH 0x008
#define PT_FLAG_NOCACHE      0x010
#define PT_FLAG_ACCESSED     0x020
#define PT_FLAG_DIRTY        0x040
#define PT_FLAG_PAT          0x080
#define PT_FLAG_GLOBAL       0x100

#define PT_FLAGS_KERNELMODE  (PT_FLAG_PRESENT|PT_FLAG_WRITE)
#define PT_FLAGS_USERMODE    (PT_FLAG_PRESENT|PT_FLAG_WRITE|PT_FLAG_USER)

/* globals */
/* TODO: initial memory maps to load in paging_init */
extern page_t *kernel_pd;

/* frame bitset */
uint32_t *frames;
size_t frame_count;

#define INDEX_FROM_BIT(a)  (a / (8*sizeof(frames[0])))
#define OFFSET_FROM_BIT(a) (a % (8*sizeof(frames[0])))

/**
 * Mark the frame at address as used.
 */
void vmem_frame_set(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] |= 1<<offset;
}

/**
 * Mark the frame at arress as free.
 */
void vmem_frame_unset(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] &= ~(1<<offset);
}

/**
 * Test if frame at address is used.
 */
int vmem_frame_test(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    return (frames[index] & (1<<offset)) ? 1 : 0;
}

/**
 * Find the first free frame.
 */
uintptr_t vmem_frame_find_first(void) {
    size_t i;
    uint32_t x;

    /* NOTE: skips the first MiB as it contains a lot of reserved memory */
    for(i = INDEX_FROM_BIT(0x100); i < INDEX_FROM_BIT(frame_count); i++)
        if((x = ffsl(~frames[i]))) return i*8*sizeof(frames[0]) + x-1;

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED "Out of memory.\n");
    arch_fatal();
    return UINTPTR_MAX;
}

/**
 * Find the first n contiguous free frames.
 */
uintptr_t vmem_frame_find_first_n(unsigned n) {
    size_t i;
    unsigned j;

    for(i = 0x100; i < frame_count; i++) {
        int bad = 0;
        for(j = 0; j < n; j++) {
            if(vmem_frame_test(PAGE_SIZE * (i+j))) {
                bad = 1;
                break;
            }
        }
        if(!bad) return i;
        else i += j;
    }

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED
           "Failed to allocate %d contiguous frames.\n", n);
    arch_fatal();
    return UINTPTR_MAX;
}

/**
 * Set the flags for a page and allocate a frame for it if needed.
 */
void vmem_frame_alloc(page_t *page, unsigned flags) {
    if(!page->frame) {
        uintptr_t frame = vmem_frame_find_first();
        vmem_frame_set(frame << PAGE_BITS);
        page->frame = frame;
    }

    page->present = 1;
    page->write   = (flags & VMEM_FLAG_WRITE)  ? 1 : 0;
    page->user    = (flags & VMEM_FLAG_KERNEL) ? 0 : 1;
    page->writethrough = (flags & VMEM_FLAG_WRITETHROUGH) ? 1 : 0;
    page->nocache = (flags & VMEM_FLAG_NOCACHE) ? 1 : 0;
    page->pat     = (flags & VMEM_FLAG_PAT) ? 1 : 0;
    page->global  = (flags & VMEM_FLAG_GLOBAL) ? 1 : 0;
}

/**
 * Map a page to the given physical address.
 */
void vmem_frame_map_addr(page_t *page, unsigned flags, uintptr_t paddr) {
    vmem_frame_set(paddr);
    page->frame = paddr >> PAGE_BITS;
    vmem_frame_alloc(page, flags);
}

/**
 * Get a virtual address which maps to the given physical address.
 */
void *vmem_get_vaddr(uintptr_t paddr) {
#warning "FIX THIS IMMEDIATELY! vmem_get_vaddr"
    return (void *)paddr;
}

/**
 * Free the frame pointed at by a page.
 * TODO: refcounts
 */
void vmem_page_free(page_t *page) {
    vmem_frame_unset(page->frame << PAGE_BITS);
}

/**
 * Get the physical address of a given virtual address in a directory.
 */
uintptr_t vmem_get_paddr(page_t *pd, uintptr_t vaddr) {
    uintptr_t page_addr = vaddr >> PAGE_BITS;
    uintptr_t pdi = (page_addr >> 10) & 0x3ff;
    uintptr_t pti = (page_addr)       & 0x3ff;

    if(!pd[pdi].present) return UINTPTR_MAX;
    page_t *pt = vmem_get_vaddr((uintptr_t)pd[pdi].frame << PAGE_BITS);

    if(!pt[pti].present) return UINTPTR_MAX;
    return ((uintptr_t)pt[pti].frame << PAGE_BITS) | (vaddr & (PAGE_SIZE-1));
}

/**
 * Get a the page for a given virtual address from the current page directory.
 *
 * If an intermediate table is not found and the VMEM_GET_CREATE flag is set, it
 * will be allocated (with user mode access if VMEM_GET_KERNEL is not set).
 * Otherwise NULL is returned.
 */
page_t *vmem_get_page(uintptr_t vaddr, unsigned flags) {
    uintptr_t page_addr = vaddr >> PAGE_BITS;
    uintptr_t pdi = (page_addr >> 10) & 0x3ff;
    uintptr_t pti = (page_addr)       & 0x3ff;

    /* TODO: core */
    page_t *pd = kernel_pd;

    if(!pd[pdi].present) {
        if(!(flags & VMEM_GET_CREATE)) goto not_found;

        uintptr_t frame = vmem_frame_find_first() << PAGE_BITS;
        vmem_frame_set(frame);
        memset(vmem_get_vaddr(frame), 0, PAGE_SIZE);
        pd[pdi].raw = frame | (flags & VMEM_GET_KERNEL
                               ? PD_FLAGS_KERNELMODE
                               : PD_FLAGS_USERMODE);
    }
    page_t *pt = vmem_get_vaddr((uintptr_t)pd[pdi].frame << PAGE_BITS);

    return pt + pti;
not_found:
    return NULL;
}

void vmem_free_dir(page_t *dir);

int vmem_validate_user_ptr(void *vaddr, size_t sz, unsigned flags);

/* standard vmem functions */
void *i386_get_paddr(void *vaddr) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    return (void *)((pt[pti] & ~0xfff) + ((uint32_t)vaddr & 0xfff));
}

void i386_map_page(void *paddr, void *vaddr, uint8_t flags) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pd = (uint32_t *)0xfffff000;
    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    if(!(pd[pdi] & 0x1)) {
        if(!buffer_pt) {
            puts("\033[41mOut of memory!");
            for(;;) asm volatile("hlt");
        }
        pd[pdi] = (uint32_t)i386_get_paddr(buffer_pt) | 0x1;
        buffer_pt = kmalloc_a(I386_PAGE_SIZE);
        memset(buffer_pt, 0, I386_PAGE_SIZE);
    }

    if(pt[pti] & 0x1) {
        /* TODO: kerror or something */
        printf("PT entry already mapped (%p)\n", vaddr);
        return;
    }

    frame_set((uint32_t)paddr);
    pt[pti] = (uint32_t)paddr | (flags & 0xfff) | 0x1;

    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "%eax");
}

void i386_unmap_page(void *vaddr) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pd = (uint32_t *)0xfffff000;
    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    if(!(pd[pdi] & 0x1)) return;
    frame_unset((uint32_t)i386_get_paddr(vaddr));
    pt[pti] = 0;

    /* remove PD entry when empty */
    for(uint16_t i = 0; i < I386_PAGE_SIZE/sizeof(uint32_t); i++)
        if(pt[i] & 0x1) goto not_empty;
    /* TODO: free PT */
    pd[pdi] = 0;
not_empty:

    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "%eax");
}

/* fault */
void _page_fault(struct int_regs *r) {
    void *loc;
    asm volatile("mov %%cr2, %0" : "=r"(loc));

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED "Page fault (%s%s%s%s%s) at %p\n",
           r->err_code & 0x1  ? ""          : "not present,",
           r->err_code & 0x2  ? "write,"    : "",
           r->err_code & 0x4  ? "user"      : "",
           r->err_code & 0x8  ? "reserved," : "",
           r->err_code & 0x10 ? "instr,"    : "",
           loc
    );
    for(;;);
}
