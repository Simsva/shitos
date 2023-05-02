#include "stdio_impl.h"

int vfscanf(__unused FILE *restrict f, __unused const char *restrict fmt, __unused va_list ap) {
    return -1;
}
