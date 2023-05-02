#include "stdio_impl.h"

int vprintf(const char *restrict fmt, va_list ap) {
    return vfprintf(stdout, fmt, ap);
}
