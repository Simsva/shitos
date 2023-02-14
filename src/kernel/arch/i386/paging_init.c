#include <stdint.h>

#define KERNEL_MAP 0xc0000000

extern void *kmem_head;
extern void *kernel_pd;
extern int *_kernel_lowtext_start;

__attribute__((section(".low.text"))) void paging_init(void) {
    /* get physical address of  kmem_head */
    void *kmem_low = *(void **)((void *)&kmem_head - KERNEL_MAP);
    void *pt = kmem_low;
    void *srccur = (void *)0;
    uint32_t *pdcur = *(void **)((void *)&kernel_pd - KERNEL_MAP) + (KERNEL_MAP>>22);
    uint32_t *ptcur = pt;

    *pdcur++ = (uint32_t)pt;
    for(;;) {
        if(srccur >= (void *)&_kernel_lowtext_start)
            *ptcur = (unsigned)srccur | 0x1;
        if(srccur >= pt)
            break;

        srccur += 0x1000;
        ptcur++;

        if((void *)ptcur - pt >= 0x1000) {
            pt += 0x1000;
            *pdcur++ = (unsigned)pt;
        }
    }
    kmem_low = pt + 0x1000;
}
