#include <sys/stat.h>
#include "__syscall.h"

int mkdir(const char *path, mode_t mode) {
    return __syscall_ret(syscall_mknod(path, (mode&07777) | S_IFDIR));
}
