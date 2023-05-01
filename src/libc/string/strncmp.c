#include <string.h>

int strncmp(const char *l, const char *r, size_t n) {
    if(n-- == 0) return 0;
    for(; *l && *r && n && *l == *r; l++, r++, n--);
    return *l - *r;
}
