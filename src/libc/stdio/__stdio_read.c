#include "stdio_impl.h"
#include <syscall.h>

/* TODO: buffered read */
size_t __stdio_read(FILE *f, unsigned char *buf, size_t sz) {
    ssize_t r = syscall_read(f->fd, (char *)buf, sz);
    if(r == 0) f->eof = 1;
    return r;
}
