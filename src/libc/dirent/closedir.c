#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include "__syscall.h"

int closedir(DIR *dir) {
    int ret = __syscall_ret(dir ? syscall_close(dir->fd) : -EBADF);
    if(dir) free(dir);
    return ret;
}
