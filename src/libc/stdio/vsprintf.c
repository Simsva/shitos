#include <stdio.h>

int vsprintf(char *restrict buf, const char *restrict fmt, va_list ap) {
    return vsnprintf(buf, INT_MAX, fmt, ap);
}
