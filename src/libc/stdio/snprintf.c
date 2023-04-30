#include "stdio_impl.h"

int snprintf(char *restrict buf, size_t n, const char *restrict fmt, ...) {
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return ret;
}
