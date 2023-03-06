#ifndef KERNEL_KMEM_H_
#define KERNEL_KMEM_H_

#include <stddef.h>

extern void *kmem_head;

void *kmalloc(size_t sz);
void *kmalloc_a(size_t sz);

#endif // KERNEL_KMEM_H_
