#include <stdio.h>

#ifdef __is_libk
#include <kernel/tty/tm.h>
#endif

int putchar(int ic) {
#ifdef __is_libk
    unsigned char c = (unsigned char)ic;
    tm_putc(c);
#endif
    return ic;
}
