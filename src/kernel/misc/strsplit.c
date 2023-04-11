#include <kernel/strsplit.h>

#include <string.h>

/**
 * Split a string on delim and output all tokens to v, returns the count
 */
int strsplit(char *s, char *delim, char **v) {
    char *pch, *save;
    int c = 0;
    pch = strtok_r(s, delim, &save);
    while(pch != NULL) v[c++] = pch, pch = strtok_r(NULL, delim, &save);
    v[c] = NULL;
    return c;
}
