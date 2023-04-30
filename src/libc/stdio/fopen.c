#include "stdio_impl.h"
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

FILE *fopen(const char *restrict path, const char *restrict mode) {
    FILE *f;
    int fd;
    unsigned flags;

    if(!strchr("rwa", *mode)) {
        errno = EINVAL;
        return NULL;
    }
    flags = __fmodeflags(mode);

    fd = syscall_open(path, flags, 0666);
    if(fd < 0) return NULL;

    f = malloc(sizeof *f);
    f->fd = fd;
    f->buf = malloc(BUFSIZ);
    f->buf_sz = BUFSIZ;
    f->buf_mode = _IOFBF;

    /* file operations */
    f->read = __stdio_read;
    f->write = __stdio_write;
    f->seek = __stdio_seek;
    f->close = __stdio_close;

    /* open file list */
    f->next = __head;
    if(__head) __head->prev = f;
    __head = f;

    return f;
}
