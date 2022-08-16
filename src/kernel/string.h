#ifndef STRING_H_
#define STRING_H_

#include "stddef.h"
#include "stdint.h"

static void *memset(void *dst, uint8_t val, size_t n) {
    uint8_t *d = dst;
    while(n-- > 0) *d++ = val;
    return d;
}

static inline void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;

    while(n-- > 0) *d++ = *s++;

    return d;
}

/* TODO: memmove, memcmp */

#endif // STRING_H_
