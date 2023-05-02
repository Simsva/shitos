#ifndef KERNEL_KMEM_H_
#define KERNEL_KMEM_H_

#include <stddef.h>

#define KERNEL_MAP 0xc0000000

extern void *kmem_head;

void *kmalloc(size_t sz);
void *kmalloc_a(size_t sz);
void *krealloc(void *ptr, size_t sz);
void kfree(void *ptr);

#endif // KERNEL_KMEM_H_
