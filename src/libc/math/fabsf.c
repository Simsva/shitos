#include <math.h>
#include <stdint.h>

float fabsf(float x) {
    union { float f; uint64_t i; } u = { x };
    u.i &= 0x7fffFFFF;
    return u.f;
}
