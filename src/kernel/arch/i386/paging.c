#include "paging.h"

#include <kernel/kmem.h>
#include <sys/utils.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <ansi.h>

#include "idt.h"

/* globals */
extern void *kernel_pd;

/* frame bitset */
uint32_t *frames, nframes;

#define INDEX_FROM_BIT(a)  (a / sizeof(frames[0]))
#define OFFSET_FROM_BIT(a) (a % sizeof(frames[0]))

void frame_set(uint32_t frame_addr) {
    uint32_t frame = frame_addr >> 12;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] |= 1<<offset;
}

void frame_unset(uint32_t frame_addr) {
    uint32_t frame = frame_addr >> 12;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    frames[index] &= ~(1<<offset);
}

uint32_t frame_test(uint32_t frame_addr) {
    uint32_t frame = frame_addr >> 12;
    uint32_t index = INDEX_FROM_BIT(frame),
             offset = OFFSET_FROM_BIT(frame);
    return frames[index] & (1<<offset);
}

uint32_t frame_find_first() {
    uint32_t i, x;
    for(i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if((x = ffsl(frames[i])))
            return i*sizeof(frames[0]) + x-1;
    }
    return UINT32_MAX;
}

/* standard vmem functions */
void *i386_get_paddr(void *vaddr) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    return (void *)((pt[pti] & ~0xfff) + ((uint32_t)vaddr & 0xfff));
}

/* NOTE: not usable yet */
void i386_map_page(void *paddr, void *vaddr, uint8_t flags) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pd = (uint32_t *)0xfffff000;
    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    if(!(pd[pdi] & 0x1)) {
        /* TODO: allocate memory for a PT */
        puts("PD entry not present");
        return;
    }

    if(pt[pti] & 0x1) {
        /* TODO: kerror or something */
        puts("PT entry already mapped");
        return;
    }

    pt[pti] = (uint32_t)paddr | (flags & 0xfff) | 0x1;

    asm("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "%eax");
}

void i386_unmap_page(void *vaddr) {
    uint32_t pdi = (uint32_t)vaddr >> 22;
    uint32_t pti = (uint32_t)vaddr >> 12 & 0x3ff;

    uint32_t *pd = (uint32_t *)0xfffff000;
    uint32_t *pt = (uint32_t *)0xffc00000 + 0x400 * pdi;

    if(!(pd[pdi] & 0x1)) return;
    pt[pti] = 0;

    /* remove PD entry when empty */
    for(uint16_t i = 0; i < I386_PAGE_SIZE/sizeof(uint32_t); i++)
        if(pt[i] & 0x1) goto not_empty;
    /* TODO: free PT */
    pd[pdi] = 0;
not_empty:

    asm("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "%eax");
}

extern unsigned _kernel_end;
void i386_init_paging(void) {
    /* start memory allocation at first page after _kernel_end */
    kmem_head = (void *)(((uintptr_t)&_kernel_end + 0xfff) & 0xfffff000);
    /* kernel_pd = (void *)0xfffff000; */

    /* last available page */
    /* TODO: calculate this */
    uint32_t last_frame = 0x1000000;
    nframes = last_frame >> 12;

    /* "allocate" memory for the frame bitset */
    frames = kmem_head;
    kmem_head += INDEX_FROM_BIT(nframes);
    memset(frames, 0, INDEX_FROM_BIT(nframes));
}

/* fault */
void _page_fault(struct int_regs *r) {
    void *loc;
    asm("mov %%cr2, %0" : "=r"(loc));

    printf(ANSI_FG_BRIGHT_WHITE ANSI_BG_RED "Page fault (%s%s%s%s%s) at %p\n",
           r->err_code & 0x1  ? ""          : "not present,",
           r->err_code & 0x2  ? "ro,"       : "",
           r->err_code & 0x4  ? "user"      : "",
           r->err_code & 0x8  ? "reserved," : "",
           r->err_code & 0x10 ? "instr,"    : "",
           loc
    );
    for(;;);
}
