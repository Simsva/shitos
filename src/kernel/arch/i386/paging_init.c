#include <stdint.h>

#define KERNEL_MAP 0xc0000000
#define PADDR(a) ((void *)(a) - (KERNEL_MAP))
#define FCOUNT (*(uint32_t *)PADDR(&frame_count))
#define FRAMES (*(uint32_t **)PADDR(&frames))
#define SET_FRAME(a) \
    FRAMES[(a) / UINT32_WIDTH] \
        |= 1<<((a) % UINT32_WIDTH);

extern void *kmem_head;
extern void *kernel_pd;
extern int *_kernel_lowtext_start;
extern uint32_t frame_count;
extern uint32_t *frames;

/* early pre-paging setup */
__attribute__((section(".low.text"))) void paging_init(void) {
    void *kmem_head_low, *ptbase, *srccur;
    uint32_t *pd, *pdcur, *ptcur, *vga_pt;

    /* get physical address of kmem_head */
    kmem_head_low = *(void **)PADDR(&kmem_head);
    /* get physical address of pd */
    pd = (uint32_t *)PADDR(&kernel_pd);
    pdcur = pd + (KERNEL_MAP>>22);

    vga_pt = kmem_head_low;
    ptbase = kmem_head_low + 0x1000;
    srccur = (void *)0;
    ptcur = ptbase;

    /* identity map first page table */
    pd[0] = (uint32_t)ptbase | 0x1;

    *(uint32_t *)(0x100) = (uint32_t)&FCOUNT;

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
        if(srccur >= ptbase)
            break;

        srccur += 0x1000;
        ptcur++;

        if((void *)ptcur - ptbase >= 0x1000) {
            ptbase += 0x1000;
            *pdcur++ = (uint32_t)ptbase | 0x1;
        }
    }

    /* map VGA text mode memory to 0xffbff000 */
    vga_pt[0x3ff] = 0xb8000 | 0x1;
    pd[0x3fe] = (uint32_t)(vga_pt + 0x3ff) | 0x1;
    SET_FRAME(0xb80000 >> 12);

    /* map last PD entry to itself */
    pd[0x3ff] = (uint32_t)pd | 0x1;

    /* set frames to vaddr */
    FRAMES = (void *)FRAMES + KERNEL_MAP;
    /* save vaddr of new kmem_head */
    kmem_head_low = ptbase + 0x1000 + KERNEL_MAP;
}
