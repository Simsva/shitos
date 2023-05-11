#include "stdio_impl.h"
#include <string.h>
#include <errno.h>

#ifndef __is_libk
void perror(const char *s) {
    fprintf(stderr, "%s: %s\n", s, strerror(errno));
}
#endif
