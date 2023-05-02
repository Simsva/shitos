#include "stdio_impl.h"

int fgetc(FILE *f) {
    char c;
    int r = fread(&c, 1, 1, f);
    if(r <= 0) {
        f->flags |= r == 0 ? F_EOF : F_ERR;
        return EOF;
    }
    return (unsigned char)c;
}

weak_alias(fgetc, getc);
