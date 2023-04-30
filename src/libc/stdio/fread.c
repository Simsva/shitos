#include "stdio_impl.h"

size_t fread(void *restrict p, size_t sz, size_t nmemb, FILE *restrict f) {
    if(!f || !f->read) return 0;
    return f->read(f, p, sz*nmemb);
}
