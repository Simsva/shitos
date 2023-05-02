#include "stdio_impl.h"

int putchar(int ic) {
    return fputc(ic, stdout);
}
