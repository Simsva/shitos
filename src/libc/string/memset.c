#include <string.h>

void *memset(void *dst, int val, size_t n) {
    uint8_t *d = dst;
    while(n-- > 0) *d++ = val;
    return dst;
}
