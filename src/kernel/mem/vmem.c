#include <kernel/vmem.h>

#if _ARCH == i386
# include "../arch/i386/paging.h"
#endif

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
/* NOTE: should be done in kernel entrypoint for every architecture */
/* void vmem_init(void) { */
/* #if _ARCH == i386 */
/*     i386_init_paging(); */
/* #else */
/*     puts("vmem_init not implemented for the current architecture"); */
/* #endif */
/* } */
