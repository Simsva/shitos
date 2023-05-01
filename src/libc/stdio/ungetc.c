#include "stdio_impl.h"

int ungetc(int c, FILE *f) {
    if(f->ungetc < 0)
        return f->ungetc = (unsigned char)c;
    return EOF;
}
