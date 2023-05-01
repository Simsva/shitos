#include "stdio_impl.h"

#undef stdout

#ifdef __is_libk
#include <kernel/console.h>

static size_t __kstdout_write(__unused FILE *f, const unsigned char *s, size_t l) {
    return fs_write(console_dev, 0, l, (uint8_t *)s);
}

__hidden FILE __stdout_FILE = {
    .fd = 1,
    .write = __kstdout_write,
};
#else
__hidden FILE __stdout_FILE = {
    .fd = 1,
    .write = __stdio_write,
    .seek = __stdio_seek,
    .close = __stdio_close,
    .buf_sz = BUFSIZ,
    .buf_mode = _IOLBF,
    .ungetc = -1,
};
#endif

FILE *const stdout = &__stdout_FILE;
