#ifndef I386_VMEM_H_
#define I386_VMEM_H_

#include <stdint.h>

#define PAGE_SIZE 0x1000
#define PAGE_BITS 12

/* generic type for page directories, tables, and pages themselves */
typedef union page {
    struct {
        uint32_t present      : 1;
        uint32_t write        : 1;
        uint32_t user         : 1;
        uint32_t writethrough : 1;
        uint32_t nocache      : 1;
        uint32_t accessed     : 1;
        uint32_t dirty        : 1;
        uint32_t pat          : 1;
        uint32_t global       : 1;
        uint32_t reserved     : 3;
        uint32_t frame        : 20;
    };
    uint32_t raw;
} page_t;

typedef struct page_directory {
    uintptr_t paddr;
    page_t *pts;
} page_directory_t;

#define vmem_page_user_readable(p) (p->user)
#define vmem_page_user_writable(p) (p->rw)

#endif // I386_VMEM_H_
