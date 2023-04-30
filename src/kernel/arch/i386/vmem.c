#include <kernel/vmem.h>
#include <kernel/kmem.h>
#include <kernel/process.h>
#include <kernel/hashmap.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <features.h>
#include <ansi.h>

#include <kernel/arch/i386/idt.h>
#include <kernel/arch/i386/arch.h>

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

/* frame bitset */
uint32_t *frames;
size_t frame_count;

/* reserved pages for mapping */
uint32_t map_pages[2048] = { 0 };
#define map_pages_count (sizeof map_pages * 8*sizeof *map_pages)
static hashmap_t *map_pages_index = NULL;

/* TODO: move all pre-allocated pages here */
#define __pagemap __attribute__((aligned(PAGE_SIZE))) = {0}
page_t kernel_pts[1024] __pagemap;
/* page tables that cover the entire mapping region */
page_t map_pages_pt[64][1024] __pagemap;

page_directory_t kernel_pd = {.pts = kernel_pts};

#define INDEX_FROM_BIT(a)  (a / (8*sizeof(frames[0])))
#define OFFSET_FROM_BIT(a) (a % (8*sizeof(frames[0])))

static void vmem_frame_set_internal(uint32_t *frames, uintptr_t frame);
static void vmem_frame_unset_internal(uint32_t *frames, uintptr_t frame);
static int vmem_frame_test_internal(uint32_t *frames, uintptr_t frame);
static uintptr_t vmem_frame_find_first_internal(uint32_t *frames, uintptr_t min, uintptr_t max);

static void vmem_copy_page(page_t *dst_pt, page_t *src_pt, size_t i);

static void vmem_map_page_set(uintptr_t addr);
static void vmem_map_page_unset(uintptr_t addr);
static int vmem_map_page_test(uintptr_t addr);
static uintptr_t vmem_map_page_find_first(void);
static uintptr_t vmem_map_page_find_first_n(unsigned n);

static void reload_pd(void);

/**
 * Initialize virtual memory (and the kernel heap).
 */
void vmem_init(void) {
    vmem_heap_create(&kheap, kmem_head, kmem_head+VMEM_HEAP_INITIAL_SZ,
                     (void *)0xcffff000, VMEM_HEAP_INDEX_SZ, 1);

    map_pages_index = hashmap_create_int(128);
    map_pages_index->value_free = NULL;

    this_core->current_pd = &kernel_pd;
}

/* internal versions of bitset logic common between frames and map_pages
 * XXX: all functions use the argument name "frames" for the macros to work */
static inline void vmem_frame_set_internal(uint32_t *frames, uintptr_t frame) {
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] |= 1<<offset;
}

static inline void vmem_frame_unset_internal(uint32_t *frames, uintptr_t frame) {
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] &= ~(1<<offset);
}

static inline int vmem_frame_test_internal(uint32_t *frames, uintptr_t frame) {
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    return (frames[index] & (1<<offset)) ? 1 : 0;
}

static uintptr_t vmem_frame_find_first_internal(uint32_t *frames, uintptr_t min, uintptr_t max) {
    size_t i;
    uint32_t x;

    /* NOTE: skips the first MiB as it contains a lot of reserved memory */
    for(i = INDEX_FROM_BIT(min); i < INDEX_FROM_BIT(max); i++)
        if((x = ffsl(~frames[i]))) return i*8*sizeof(frames[0]) + x-1;

    return UINTPTR_MAX;
}

/**
 * Mark the frame at address as used.
 */
void vmem_frame_set(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    vmem_frame_set_internal(frames, frame);
}

/**
 * Mark the frame at arress as free.
 */
void vmem_frame_unset(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    vmem_frame_unset_internal(frames, frame);
}

/**
 * Test if frame at address is used.
 */
int vmem_frame_test(uintptr_t frame_addr) {
    uint32_t frame = frame_addr >> PAGE_BITS;
    return vmem_frame_test_internal(frames, frame);
}

/**
 * Find the first free frame.
 */
uintptr_t vmem_frame_find_first(void) {
    /* NOTE: skips the first MiB as it contains a lot of reserved memory */
    uintptr_t frame = vmem_frame_find_first_internal(frames, 0x100, frame_count);
    if(frame != UINTPTR_MAX) return frame;

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
 * Same as vmem_frame_set, but for mapping pages.
 */
static void vmem_map_page_set(uintptr_t addr) {
    uintptr_t idx = (addr - VMEM_MAP_PAGES_MEMORY) >> PAGE_BITS;
    vmem_frame_set_internal(map_pages, idx);
}

/**
 * Same as vmem_frame_unset, but for mapping pages.
 */
static __unused void vmem_map_page_unset(uintptr_t addr) {
    uintptr_t idx = (addr - VMEM_MAP_PAGES_MEMORY) >> PAGE_BITS;
    vmem_frame_unset_internal(map_pages, idx);
}

/**
 * Same as vmem_frame_test, but for mapping pages.
 */
static int vmem_map_page_test(uintptr_t addr) {
    uintptr_t idx = (addr - VMEM_MAP_PAGES_MEMORY) >> PAGE_BITS;
    return vmem_frame_test_internal(map_pages, idx);
}

/**
 * Same as vmem_frame_find_first, but for mapping pages.
 */
static uintptr_t vmem_map_page_find_first(void) {
    uintptr_t idx = vmem_frame_find_first_internal(map_pages, 0, map_pages_count);
    if(idx != UINTPTR_MAX) return VMEM_MAP_PAGES_MEMORY + (idx << PAGE_BITS);

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED "Out of reserved mapping pages.\n");
    arch_fatal();
    return UINTPTR_MAX;
}

/**
 * Same as vmem_frame_find_first_n, but for mapping pages.
 */
static uintptr_t vmem_map_page_find_first_n(unsigned n) {
    size_t i;
    unsigned j;

    for(i = 0; i < map_pages_count; i++) {
        int bad = 0;
        for(j = 0; j < n; j++) {
            if(vmem_map_page_test(VMEM_MAP_PAGES_MEMORY + PAGE_SIZE * (i+j))) {
                bad = 1;
                break;
            }
        }
        if(!bad) return VMEM_MAP_PAGES_MEMORY + (i << PAGE_BITS);
        else i += j;
    }

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED
           "Failed to map %d contiguous pages.\n", n);
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

    reload_pd();
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
 * Get a virtual address which maps to the given physical address in the
 * reserved mapping region.
 */
void *vmem_map_vaddr(uintptr_t paddr) {
    uintptr_t page_off = paddr & (PAGE_SIZE-1);
    paddr &= ~(PAGE_SIZE-1);

    uintptr_t vaddr;
    if((vaddr = (uintptr_t)hashmap_get(map_pages_index, (void *)paddr)))
        return (void *)vaddr + page_off;

    vaddr = vmem_map_page_find_first();
    vmem_map_page_set(vaddr);
    vmem_frame_map_addr(vmem_get_page(vaddr, VMEM_GET_CREATE|VMEM_GET_KERNEL),
                        VMEM_FLAG_KERNEL|VMEM_FLAG_WRITE, paddr);

    hashmap_set(map_pages_index, (void *)paddr, (void *)vaddr);
    return (void *)vaddr + page_off;
}

/**
 * Get a virtual address which maps the the given physical address for sz bytes.
 * NOTE: will remap new addresses after each call, so never use this twice on
 * the same physical memory.
 */
void *vmem_map_vaddr_n(uintptr_t paddr, size_t sz) {
    uintptr_t page_off = paddr & (PAGE_SIZE-1);
    paddr &= ~(PAGE_SIZE-1);

    unsigned n_pages = (sz + page_off + PAGE_SIZE-1) / PAGE_SIZE;
    uintptr_t vaddr = vmem_map_page_find_first_n(n_pages);

    /* NOTE: use two loops so vmem_get_page does not use up one of our pages */
    for(unsigned i = 0; i < n_pages; i++)
        vmem_map_page_set(vaddr + i*PAGE_SIZE);
    for(unsigned i = 0; i < n_pages; i++) {
        vmem_frame_map_addr(vmem_get_page(vaddr + i*PAGE_SIZE,
                                          VMEM_GET_CREATE|VMEM_GET_KERNEL),
                            VMEM_FLAG_KERNEL|VMEM_FLAG_WRITE, paddr + i*PAGE_SIZE);

        hashmap_set(map_pages_index, (void *)paddr + i*PAGE_SIZE,
                                     (void *)vaddr + i*PAGE_SIZE);
    }

    return (void *)vaddr + page_off;
}

/**
 * Free the frame pointed at by a page.
 * TODO: refcounts
 */
void vmem_frame_free(page_t *page) {
    vmem_frame_unset(page->frame << PAGE_BITS);
    page->raw = 0;
    reload_pd();
}

/**
 * Get the physical address of a given virtual address in a directory.
 */
uintptr_t vmem_get_paddr(page_t *pd, uintptr_t vaddr) {
    uintptr_t page_addr = vaddr >> PAGE_BITS;
    uintptr_t pdi = (page_addr >> 10) & 0x3ff;
    uintptr_t pti = (page_addr)       & 0x3ff;

    if(!pd[pdi].present) return UINTPTR_MAX;
    page_t *pt = vmem_map_vaddr((uintptr_t)pd[pdi].frame << PAGE_BITS);

    if(!pt[pti].present) return UINTPTR_MAX;
    return ((uintptr_t)pt[pti].frame << PAGE_BITS) | (vaddr & (PAGE_SIZE-1));
}

/**
 * Get the page for a given virtual address from another page directory.
 *
 * If an intermediate table is not found and the VMEM_GET_CREATE flag is set, it
 * will be allocated (with user mode access if VMEM_GET_KERNEL is not set).
 * Otherwise NULL is returned.
 */
page_t *vmem_get_page_other(page_directory_t *dir, uintptr_t vaddr, unsigned flags) {
    /* if in the general mapping region,
     * assumed to be mapped in every page directory */
    if(vaddr >= 0xf0000000) {
        uintptr_t page_addr = (vaddr - 0xf0000000) >> PAGE_BITS;
        uintptr_t pdi = (page_addr >> 10) & 0x3ff;
        uintptr_t pti = (page_addr)       & 0x3ff;
        return &map_pages_pt[pdi][pti];
    }

    uintptr_t page_addr = vaddr >> PAGE_BITS;
    uintptr_t pdi = (page_addr >> 10) & 0x3ff;
    uintptr_t pti = (page_addr)       & 0x3ff;

    page_t *pd = dir->pts;

    if(!pd[pdi].present) {
        if(!(flags & VMEM_GET_CREATE)) goto not_found;

        uintptr_t frame = vmem_frame_find_first() << PAGE_BITS;
        vmem_frame_set(frame);
        memset(vmem_map_vaddr(frame), 0, PAGE_SIZE);
        pd[pdi].raw = frame | (flags & VMEM_GET_KERNEL
                               ? PD_FLAGS_KERNELMODE
                               : PD_FLAGS_USERMODE);
    }
    page_t *pt = vmem_map_vaddr((uintptr_t)pd[pdi].frame << PAGE_BITS);

    return pt + pti;
not_found:
    return NULL;
}

/**
 * Get the page for a given virtual address from the current page directory.
 *
 * If an intermediate table is not found and the VMEM_GET_CREATE flag is set, it
 * will be allocated (with user mode access if VMEM_GET_KERNEL is not set).
 * Otherwise NULL is returned.
 */
page_t *vmem_get_page(uintptr_t vaddr, unsigned flags) {
    return vmem_get_page_other(this_core->current_pd, vaddr, flags);
}

/**
 * Copy a single page.
 */
static void vmem_copy_page(page_t *dst_pt, page_t *src_pt, size_t i) {
    /* TODO: refcounts */
    dst_pt[i].raw = src_pt[i].raw;
}

/**
 * Clone a page directory. If src is NULL, make an empty page directory.
 */
page_directory_t *vmem_clone_dir(page_directory_t *src) {
    if(!src) src = &kernel_pd;

    page_directory_t *dst = kmalloc(sizeof(page_directory_t));
    dst->paddr = vmem_frame_find_first() << PAGE_BITS;
    dst->pts = vmem_map_vaddr(dst->paddr);

    /* zero bottom 3 GiB */
    memset(dst->pts, 0, 0x300 * sizeof(page_t));
    /* link top 1 GiB (kernel memory) */
    memcpy(&dst->pts[0x300], &src->pts[0x300], 0x100 * sizeof(page_t));

    /* copy page tables */
    for(uint16_t i = 0; i < 0x300; i++) {
        if(!src->pts[i].present) continue;
        page_t *src_pt = vmem_map_vaddr(src->pts[i].frame << PAGE_BITS);

        uintptr_t dst_pt_paddr = vmem_frame_find_first() << PAGE_BITS;
        vmem_frame_set(dst_pt_paddr);
        page_t *dst_pt = vmem_map_vaddr(dst_pt_paddr);
        memset(dst_pt, 0, PAGE_SIZE);
        dst->pts[i].raw = dst_pt_paddr | PT_FLAGS_USERMODE;

        for(uint16_t j = 0; j < 0x400; j++) {
            if(!src_pt[j].present) continue;
            if(src_pt->user)
                vmem_copy_page(dst_pt, src_pt, j);
            else
                dst_pt[j].raw = src_pt[j].raw;
        }
    }

    return dst;
}

/**
 * Set the current page directory.
 */
void vmem_set_dir(page_directory_t *dir) {
    if(!dir) return;
    this_core->current_pd = dir;
    asm volatile("mov %0, %%cr3" :: "r"(dir->paddr));
}

/**
 * TODO: Free a page directory.
 */
void vmem_free_dir(page_directory_t *dir);

/**
 * Check if a pointer is accessable in user mode in the current page directory.
 */
int vmem_validate_user_ptr(const void *vaddr, size_t sz, unsigned flags) {
    if(vaddr == NULL && !(flags & VMEM_PTR_FLAG_NULL)) return 0;

    uintptr_t base = (uintptr_t)vaddr;
    uintptr_t end = sz ? (base + sz-1) : base;

    uintptr_t page_base = base >> PAGE_BITS;
    uintptr_t page_end = end >> PAGE_BITS;

    for(uintptr_t page = page_base; page <= page_end; page++) {
        page_t *page_entry = vmem_get_page_other(this_core->current_proc->pd, page << PAGE_BITS, 0);

        if(!page_entry) return 0;
        if(!page_entry->present) return 0;
        if(!page_entry->user) return 0;
        if(!page_entry->write && flags & VMEM_PTR_FLAG_WRITE)
            return 0;
    }

    return 1;
}

static void reload_pd(void) {
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
