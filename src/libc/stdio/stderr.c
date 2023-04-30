#include "stdio_impl.h"

#undef stderr

#ifdef __is_libk
__hidden FILE __stderr_FILE = {0};
#else
__hidden FILE __stderr_FILE = {
    .fd = 2,
    .write = __stdio_write,
    .seek = __stdio_seek,
    .close = __stdio_close,
    .buf_mode = _IONBF,
};
#endif

FILE *const stderr = &__stderr_FILE;
