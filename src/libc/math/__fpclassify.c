#include <math.h>
#include <stdint.h>

int __fpclassify(double x) {
    union {
        double d;
        uint64_t i;
    } y = { x };
    int e = y.i >> 52 & 0x7ff;
    if(!e) return y.i << 1 ? FP_SUBNORMAL : FP_ZERO;
    if(e == 0x7ff) return y.i << 12 ? FP_NAN : FP_INFINITE;
    return FP_NORMAL;
}
