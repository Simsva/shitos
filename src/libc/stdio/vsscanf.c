#include "stdio_impl.h"
#include <ctype.h>

/* taken from from ToaruOS */
int vsscanf(const char *restrict str, const char *restrict fmt, va_list ap) {
    int count = 0;
    while (*fmt) {
        if (*fmt == ' ') {
            /* handle white space */
            while (*str && isspace(*str)) {
                str++;
            }
        } else if (*fmt == '%') {
            /* Parse */
            fmt++;
            /* int _long = 0; */

            if (*fmt == 'l') {
                fmt++;
                if (*fmt == 'l') {
                    /* _long = 1; */
                    fmt++;
                }
            }

            if (*fmt == 'd') {
                int i = 0;
                int sign = 1;
                while (isspace(*str)) str++;
                if (*str == '-') {
                    sign = -1;
                    str++;
                }
                while (*str && *str >= '0' && *str <= '9') {
                    i = i * 10 + *str - '0';
                    str++;
                }
                int * out = (int *)va_arg(ap, int*);
                count++;
                *out = i * sign;
            } else if (*fmt == 'u') {
                unsigned int i = 0;
                while (isspace(*str)) str++;
                while (*str && *str >= '0' && *str <= '9') {
                    i = i * 10 + *str - '0';
                    str++;
                }
                unsigned int * out = (unsigned int *)va_arg(ap, unsigned int*);
                count++;
                *out = i;
            }
        } else {
            /* Expect exact character? */
            if (*str == *fmt) {
                str++;
            } else {
                break;
            }
        }
        fmt++;
    }
    return count;
}
