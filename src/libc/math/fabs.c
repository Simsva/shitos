#include <math.h>
#include <stdint.h>

double fabs(double x) {
    union { double d; uint64_t i; } u = { x };
    u.i &= 0x7fffFFFFffffFFFF;
    return u.d;
}
