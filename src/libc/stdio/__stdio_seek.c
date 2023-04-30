#include "stdio_impl.h"
#include <syscall.h>
#include <errno.h>

off_t __stdio_seek(FILE *f, off_t off, int whence) {
    off_t new_off = syscall_seek(f->fd, off, whence);
    if(new_off < 0) {
        errno = -new_off;
        return -1;
    }
    return f->off = new_off;
}
