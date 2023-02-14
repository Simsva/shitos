#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <stdint.h>

static inline int a_ctz_32(uint32_t i) {
    static const char debruijn32[32] = {
        0, 1, 23, 2, 29, 24, 19, 3, 30, 27, 25, 11, 20, 8, 4, 13,
        31, 22, 28, 18, 26, 10, 7, 12, 21, 17, 9, 6, 16, 5, 15, 14
    };
    return debruijn32[(i&-i)*0x076be629 >> 27];
}

static inline int a_ctz_64(uint64_t i)
{
    static const char debruijn64[64] = {
        0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
        62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
    };
    if (sizeof(long) < 8) {
        uint32_t j = i;
        if (!j) {
            j = i >> 32;
            return 32 + a_ctz_32(j);
        }
        return a_ctz_32(j);
    }
    return debruijn64[(i&-i)*0x022fdd63cc95386dull >> 58];
}

static inline int a_ctz_l(unsigned long i) {
    return (sizeof(long) < 8) ? a_ctz_32(i) : a_ctz_64(i);
}

#endif // ATOMIC_H_
