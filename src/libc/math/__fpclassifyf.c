#include <math.h>
#include <stdint.h>

int __fpclassifyf(float x) {
    union {
        float f;
        uint32_t i;
    } y = { x };
    int e = y.i >> 23 & 0xff;
    if(!e) return y.i << 1 ? FP_SUBNORMAL : FP_ZERO;
    if(e == 0xff) return y.i << 9 ? FP_NAN : FP_INFINITE;
    return FP_NORMAL;
}
