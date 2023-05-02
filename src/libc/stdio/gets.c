#include "stdio_impl.h"

/* DEPRECATED */
char *gets(char *s) {
    int c;
    char *r = s;
    while((c = fgetc(stdin)) >= 0 && c != '\n') *s++ = c;
    *s = '\0';
    return c != '\n' && (!feof(stdin) || r == s) ? NULL : r;
}
