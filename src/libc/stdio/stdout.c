#include <stdio.h>
#include "stdio_impl.h"

#undef stdout

#ifdef __is_libk
#include <kernel/console.h>

static size_t __stdout_write(__unused FILE *f, const unsigned char *s, size_t l) {
    return fs_write(console_dev, 0, l, (uint8_t *)s);
}

__hidden FILE __stdout_FILE = {
    .fd = 1,
    .flags = F_PERM | F_NORD,
    .write = __stdout_write,
};
#else
__hidden FILE __stdout_FILE = {0};
#endif

FILE *const stdout = &__stdout_FILE;
