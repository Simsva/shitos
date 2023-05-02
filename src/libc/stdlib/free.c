#include <stdlib.h>

#ifdef __is_libk
# include <kernel/kmem.h>
#else
# include <syscall.h>
#endif

void free(void *p) {
#ifdef __is_libk
    kfree(p);
#else
    syscall_sysfunc(2, &p);
#endif
}
