#ifndef SYS_STRING_H_
#define SYS_STRING_H_

#include <sys/stddef.h>
#include <sys/stdint.h>

static inline void *memset(void *dst, uint8_t val, size_t n) {
    uint8_t *d = dst;
    while(n-- > 0) *d++ = val;
    return d;
}

static inline void *memcpy(void *__restrict dst, const void *__restrict src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while(n-- > 0) *d++ = *s++;
    return d;
}

static inline int memcmp(const void *vl, const void *vr, size_t n) {
    const uint8_t *l=vl, *r=vr;
    for (; n && *l == *r; n--, l++, r++);
    return n ? *l-*r : 0;
}

static inline int strcmp(const char *l, const char *r) {
    for (; *l == *r && *l; l++, r++);
    return *(unsigned char *)l - *(unsigned char *)r;
}

#endif // SYS_STRING_H_
