#include "stdio_impl.h"
#include <syscall.h>

size_t __stdio_read(FILE *f, unsigned char *buf, size_t sz) {
    return syscall_read(f->fd, (char *)buf, sz);
}
