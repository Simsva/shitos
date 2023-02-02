#include <stdio.h>
#include <stdarg.h>

int printf(const char *restrict fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}