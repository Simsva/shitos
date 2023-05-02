#include "stdio_impl.h"

int fgetc(FILE *f) {
    char c;
    int r = fread(&c, 1, 1, f);
    if(r < 0) {
        f->error = 1;
        return EOF;
    }
    if(r == 0) {
        f->eof = 1;
        return EOF;
    }
    return (unsigned char)c;
}

weak_alias(fgetc, getc);
