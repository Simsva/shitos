#include "stdio_impl.h"
#include <syscall.h>

size_t __stdio_write(FILE *f, const unsigned char *buf, size_t sz) {
    if(!f->buf) return syscall_write(f->fd, (const char *)buf, sz);

    size_t written;
    for(written = 0; sz > 0; written++, buf++, sz--) {
        f->buf[f->buf_i++] = *buf;
        if(f->buf_i == f->buf_sz || (f->buf_mode == _IOLBF && *buf == '\n'))
            fflush(f);
    }

    return written;
}
