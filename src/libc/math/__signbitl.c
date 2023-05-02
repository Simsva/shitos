#include "libm.h"

int __signbitl(long double x) {
#if (LDBL_MANT_DIG == 64 || LDBL_MANT_DIG == 113) && LDBL_MAX_EXP == 16384
    union ldshape u = { x };
    return u.i.se >> 15;
#elif LDBL_MANT_DIG == 53 && LDBL_MAX_EXP == 1024
    return __signbit(x);
#endif
}
