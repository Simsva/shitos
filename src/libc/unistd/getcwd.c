#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "__syscall.h"

char *getcwd(char *buf, size_t sz) {
    sz = sz ? sz : PATH_MAX;
    if(!buf) buf = malloc(sz);
    return __syscall_retp(syscall_getcwd(buf, sz));
}
