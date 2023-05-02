#include "stdio_impl.h"
#include <syscall.h>

/* TODO: buffered read */
size_t __stdio_read(FILE *f, unsigned char *buf, size_t sz) {
    ssize_t r = 0;
    while(sz > 0) {
        if(f->ungetc > 0) {
            *buf++ = (unsigned char)f->ungetc;
            f->ungetc = -1;
            sz--, r++;
            continue;
        }

        r = syscall_read(f->fd, (char *)buf, sz);
        if(r <= 0) f->flags |= F_EOF;
        else sz -= r;
        if(!(f->flags & F_BLOCK)) break;
    }
    return r;
}
