#include <string.h>

void *memrchr(const void *_s, int c, size_t n) {
    const unsigned char *s = _s;
    c = (unsigned char)c;
    while(n--) if(s[n] == c) return (void *)(s+n);
    return NULL;
}
