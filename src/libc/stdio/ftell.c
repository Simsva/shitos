#include "stdio_impl.h"
#include <errno.h>

long ftell(FILE *f) {
    off_t pos = f->seek(f, 0, SEEK_CUR);
    /* f->seek sets errno in this case */
    if(pos < 0) return (long)pos;
    if(f->buf_i) pos += f->buf_i;
    if(pos > LONG_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    return (long)pos;
}
