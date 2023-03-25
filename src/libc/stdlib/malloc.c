#include <stdlib.h>

#ifdef __is_libk
# include <kernel/kmem.h>
#endif

void *malloc(size_t sz) {
#ifdef __is_libk
    return kmalloc(sz);
#endif

    return NULL;
}
