#include "stdio_impl.h"
#include <stdlib.h>
#include <syscall.h>

int __stdio_close(FILE *f) {
    fflush(f);
    int ret = syscall_close(f->fd);
    if(f->buf) free(f->buf);
    f->buf = NULL;

    if(f == &__stdin_FILE || f == &__stdout_FILE || f == &__stderr_FILE)
        return ret;
    else {
        if(f->prev) f->prev->next = f->next;
        if(f->next) f->next->prev = f->prev;
        if(f == __head) __head = f->next;

        free(f);
        return ret;
    }
}
