#include "stdio_impl.h"

int sscanf(const char *restrict str, const char *restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsscanf(str, fmt, ap);
    va_end(ap);
    return r;
}
