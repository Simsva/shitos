#include "stdio_impl.h"

#undef stdin

#ifdef __is_libk
__hidden FILE __stdin_FILE = {0};
#else
__hidden FILE __stdin_FILE = {
    .fd = 0,
    .read = __stdio_read,
    .seek = __stdio_seek,
    .close = __stdio_close,
    .buf_mode = _IONBF,
};
#endif

FILE *const stdin = &__stdin_FILE;
