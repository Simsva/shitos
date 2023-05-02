#include "stdio_impl.h"

int fscanf(FILE *restrict f, const char *restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfscanf(f, fmt, ap);
    va_end(ap);
    return r;
}
