#ifndef I386_PAGING_H_
#define I386_PAGING_H_

#include <stdint.h>

#define I386_PAGE_SIZE 0x1000

void *i386_get_paddr(void *vaddr);
void i386_map_page(void *paddr, void *vaddr, uint8_t flags);
void i386_unmap_page(void *vaddr);
void i386_init_paging(void);

#endif // I386_PAGING_H_
