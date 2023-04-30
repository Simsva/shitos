#include "stdio_impl.h"

int puts(const char *s) {
    return fputs(s, stdout) == EOF || fputc('\n', stdout) == EOF
        ? EOF : 0;
}
