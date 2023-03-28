#include <string.h>

char *strcpy(char *restrict d, const char *restrict s) {
    for(; (*d = *s); s++, d++);
    return d;
}
