#include <string.h>

char *stpcpy(char *restrict d, const char *restrict s) {
    for(; (*d = *s); s++, d++);
    return d;
}
