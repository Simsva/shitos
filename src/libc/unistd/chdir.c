#include <unistd.h>
#include "__syscall.h"

int chdir(const char *path) {
    return __syscall_ret(syscall_chdir(path));
}
