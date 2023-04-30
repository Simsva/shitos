#include "stdio_impl.h"
#include <syscall.h>
#include <errno.h>

int fflush(FILE *f) {
    if(!f) {
        FILE *cur = __head;
        while(cur) {
            fflush(cur);
            cur = cur->next;
        }
        return 0;
    }
    if(!f->buf) return 0;
    if(f->buf_i) {
        int err;
        if((err = syscall_write(f->fd, (const char *)f->buf, f->buf_i)) < 0) {
            errno = -err;
            return EOF;
        }
        f->buf_i = 0;
    }
    return 0;
}
