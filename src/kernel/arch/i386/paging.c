#include "paging.h"

#include <sys/utils.h>
#include <stdio.h>

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
