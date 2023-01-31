#include <string.h>

void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while(n-- > 0) *d++ = *s++;
    return dst;
}
