#include "stdio_impl.h"

char *fgets(char *restrict s, int sz, FILE *restrict f) {
    int c;
    char *r = s;
    if(sz-- == 0) return s;
    while((c = fgetc(f)) > 0) {
        *s++ = c;
        *s = '\0';
        if(--sz == 0 || c == '\n') return r;
    }
    if(c == EOF) {
        f->eof = 1;
        return r == s ? NULL : r;
    }
    return NULL;
}
