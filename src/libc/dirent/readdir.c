#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "__syscall.h"

struct dirent *readdir(DIR *dir) {
    int r = __syscall_ret(syscall_readdir(dir->fd, ++dir->cur_entry, &dir->ent));
    if(r <= 0) {
        /* end of directory or error */
        memset(&dir->ent, 0, sizeof dir->ent);
        return NULL;
    }
    return &dir->ent;
}
