#include "stdio_impl.h"

int setvbuf(FILE *restrict f, char *restrict buf, int type, size_t sz) {
    if(type == _IONBF) {
        f->buf = NULL;
        f->buf_sz = 0;
        f->buf_mode = type;
    } else if(type == _IOLBF || type == _IOFBF) {
        f->buf = (void *)buf;
        f->buf_sz = sz;
        f->buf_mode = type;
    } else {
        return -1;
    }
    return 0;
}
