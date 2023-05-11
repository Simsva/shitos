#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include "__syscall.h"

DIR *opendir(const char *path) {
    int fd = syscall_open(path, O_RDONLY, 0);
    if(fd < 0) {
        errno = -fd;
        return NULL;
    }

    DIR *dir = malloc(sizeof(DIR));
    dir->fd = fd;
    dir->cur_entry = -1;
    return dir;
}
