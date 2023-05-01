#include "stdio_impl.h"
#include "__syscall.h"

int remove(const char *path) {
    return __syscall_ret(syscall_unlink(path));
}
