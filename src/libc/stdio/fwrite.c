#include "stdio_impl.h"

size_t fwrite(const void *restrict p, size_t sz, size_t nmemb, FILE *restrict f) {
    if(!f || !f->write) return 0;
    return f->write(f, p, sz*nmemb);
}
