#include <stdio.h>

int sprintf(char *restrict buf, const char *restrict fmt, ...) {
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsprintf(buf, fmt, ap);
    va_end(ap);
    return ret;
}
