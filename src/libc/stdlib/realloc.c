#include <stdlib.h>
#include <stdint.h>
#include <syscall.h>

#ifndef __is_libk
void *realloc(void *p, size_t sz) {
    uintptr_t args[] = {(uintptr_t)p, sz};
    return (void *)syscall_sysfunc(1, (void *)args);
}
#endif
