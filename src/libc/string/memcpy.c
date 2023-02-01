#include <string.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while(n-- > 0) *d++ = *s++;
    return dst;
}
