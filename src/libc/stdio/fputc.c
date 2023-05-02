#include "stdio_impl.h"

int fputc(int ic, FILE *f) {
    if(!f || !f->write) return EOF;
    unsigned char s[] = { ic };
    f->write(f, s, 1);
    return ic;
}

weak_alias(fputc, putc);
