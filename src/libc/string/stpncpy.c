#include <string.h>

char *stpncpy(char *restrict d, const char *restrict s, size_t n) {
    for(; (*d = *s) && n; d++, s++, n--);
    return memset(d, 0, n);
}
