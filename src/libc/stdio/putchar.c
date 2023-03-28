#include <stdio.h>

#ifdef __is_libk
#include <kernel/fs.h>
#endif

int putchar(int ic) {
#ifdef __is_libk
    unsigned char c = (unsigned char)ic;
    fs_write(console_dev, 0, 1, &c);
#endif
    return ic;
}
