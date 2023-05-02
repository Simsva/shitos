#include "libm.h"

long double fabsl(long double x) {
#if LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
    return fabs(x);
#elif (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
    union ldshape u = { x };
    u.i.se &= 0x7fff;
    return u.f;
#else
    return NAN;
#endif
}
