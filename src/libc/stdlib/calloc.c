#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef __is_libk
void *calloc(size_t nmemb, size_t sz) {
    if(sz && nmemb > SIZE_MAX/sz) {
        errno = ENOMEM;
        return NULL;
    }
    void *p = malloc(nmemb * sz);
    return memset(p, 0, nmemb * sz);
}
#endif
