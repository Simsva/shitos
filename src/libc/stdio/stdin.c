#include <stdio.h>

#undef stdin

#ifdef __is_libk
__hidden FILE __stdin_FILE = {0};
#else
__hidden FILE __stdin_FILE = {0};
#endif

FILE *const stdin = &__stdin_FILE;
