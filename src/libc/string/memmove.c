#include <string.h>

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    if(d == s) return d;
    if((uintptr_t)s - (uintptr_t)d - n <= -2*n) return memcpy(d, s, n);

    if(d < s) {
        while(n-- > 0) *d++ = *s++;
    } else {
        while(n) n--, d[n] = s[n];
    }

    return dst;
}
