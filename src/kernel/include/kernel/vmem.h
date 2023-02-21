#ifndef KERNEL_VMEM_H_
#define KERNEL_VMEM_H_

#include <stdint.h>

#if _ARCH == i386
# include <kernel/arch/i386/paging.h>
# define PAGE_SIZE I386_PAGE_SIZE
#else
/* assumed default page size */
# define PAGE_SIZE 0x1000
#endif

void *vmem_get_paddr(void *vaddr);
void vmem_map(void *paddr, void *vaddr, uint8_t flags);
void vmem_unmap(void *vaddr);
/* void vmem_init(void); */

#endif // KERNEL_VMEM_H_
