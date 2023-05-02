#include <stdlib.h>

#ifdef __is_libk
# include <kernel/kmem.h>
#else
# include <syscall.h>
#endif

void *malloc(size_t sz) {
#ifdef __is_libk
    return kmalloc(sz);
#else
    /* TODO: an actual heap in libc? */
    return (void *)syscall_sysfunc(0, &sz);
#endif
}
