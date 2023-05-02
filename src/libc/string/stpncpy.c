#include <string.h>

char *stpncpy(char *restrict d, const char *restrict s, size_t n) {
    for(; n && (*d = *s); d++, s++, n--);
    return memset(d, 0, n);
}
