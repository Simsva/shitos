#include <stdint.h>

/* Be careful when using higher half things */
#include <kernel/vmem.h>

/* this whole file is a big array bounds violation, so just ignore errors */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

#define KERNEL_MAP 0xc0000000
#define PADDR(a) ((void *)(a) - (KERNEL_MAP))
#define FCOUNT (*(uint32_t *)PADDR(&frame_count))
#define FRAMES (*(uint32_t **)PADDR(&frames))
#define MAP_PAGES_PT (*(page_t (*)[64][1024])PADDR(&map_pages_pt))
#define SET_FRAME(a) \
    FRAMES[(a) / 32] \
        |= 1<<((a) % 32);

extern void *kmem_head;
extern page_directory_t kernel_pd;
extern page_t kernel_pts[1024];
extern int *_kernel_lowtext_start;
extern uint32_t frame_count;
extern uint32_t *frames;
extern page_t map_pages_pt[0x400][64];

/* early pre-paging setup */
__attribute__((section(".low.text"))) void paging_init(void) {
    void **kmem_head_low, *ptbase, *srccur;
    uint32_t *pd, *pdcur, *ptcur;

    /* get physical address of kmem_head */
    kmem_head_low = (void **)PADDR(&kmem_head);
    /* get physical address of pd */
    pd = (uint32_t *)PADDR(&kernel_pts);
    /* inialize kernel pd */
    ((page_directory_t *)PADDR(&kernel_pd))->paddr = (uintptr_t)pd;
    pdcur = pd + (KERNEL_MAP>>22);

    ptbase = *kmem_head_low;
    srccur = (void *)0;
    ptcur = ptbase;

    /* identity map first page table */
    pd[0] = (uint32_t)ptbase | 0x1;

    /* zero out frame bitset, I do not dare use other higher-half functions in
     * this cursed C code, so no memset */
    for(uint32_t i = 0; i < ((FCOUNT+31) >> 5); i++)
        FRAMES[i] = 0;

    *pdcur++ = (uint32_t)ptbase | 0x1;
    for(;;) {
        if(srccur >= (void *)&_kernel_lowtext_start) {
            *ptcur = (uint32_t)srccur | 0x1;
            SET_FRAME((uint32_t)srccur >> 12);
        }
        /* map enough memory for the kernel heap too */
        if(srccur >= ptbase + VMEM_HEAP_INITIAL_SZ)
            break;

        srccur += 0x1000;
        ptcur++;

        if((void *)ptcur - ptbase >= 0x1000) {
            ptbase += 0x1000;
            *pdcur++ = (uint32_t)ptbase | 0x1;
        }
    }

    /* create page tables for the general mapping region */
    for(uint16_t i = 0; i < 64; i++)
        pd[((VMEM_MAP_PAGES_MEMORY >> 22) & 0x3ff) + i]
            = (uint32_t)(MAP_PAGES_PT[i]) | 0x1;

    /* set frames to vaddr */
    FRAMES = (void *)FRAMES + KERNEL_MAP;
    /* save vaddr of new kmem_head */
    *kmem_head_low = ptbase + 0x1000 + KERNEL_MAP;
}

#pragma GCC diagnostic pop
