#include <stdio.h>

#ifdef __is_libk
# include <kernel/console.h>
#endif

int putc(int ic, __unused FILE *f) {
#ifdef __is_libk
    unsigned char c = (unsigned char)ic;
    fs_write(console_dev, 0, 1, &c);
#endif
    return ic;
}
