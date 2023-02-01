#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <inttypes.h>

#define F_ALT_FORM (UINT32_C(1)<<'#'-' ')
#define F_ZERO_PAD (UINT32_C(1)<<'0'-' ')
#define F_PAD_POS  (UINT32_C(1)<<' '-' ')

#define FLAGMASK (F_ALT_FORM|F_ZERO_PAD|F_PAD_POS)

int vprintf(const char *restrict fmt, va_list ap) {
    int r = 1;
    while(*fmt) if((r = putchar(*fmt++)) < 0) return r;
    return r;
}
