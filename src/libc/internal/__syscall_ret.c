#include "__syscall.h"
#include <errno.h>

long __syscall_ret(long r) {
    if(r < 0) {
        errno = -r;
        return -1;
    }
    return r;
}
