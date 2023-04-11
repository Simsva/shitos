#include <stdio.h>

#undef stderr

#ifdef __is_libk
__hidden FILE __stderr_FILE = {0};
#else
__hidden FILE __stderr_FILE = {0};
#endif

FILE *const stderr = &__stderr_FILE;
