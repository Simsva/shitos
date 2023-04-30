#include "stdio_impl.h"
#include <errno.h>

static size_t sn_write(FILE *f, const unsigned char *s, size_t l) {
    size_t ret = l;
    while(l-- && (size_t)f->off < f->buf_sz-1) f->buf[f->off++] = *s++;
    f->buf[f->off] = '\0';
    return ret;
}

int vsnprintf(char *restrict s, size_t n, const char *restrict fmt, va_list ap) {
    FILE f = {
        .buf = (unsigned char *)s,
        .buf_sz = n,
        .off = 0,
        .write = sn_write,
    };

    if(n > INT_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    *s = '\0';
    return vfprintf(&f, fmt, ap);
}
