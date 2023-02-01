#include <stdio.h>

int puts(const char *s) {
    int r;
    while(*s) if((r = putchar(*s++)) < 0) return r;
    r = putchar('\n');
    return r;
}
