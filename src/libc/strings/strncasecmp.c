#include <strings.h>
#include <ctype.h>

int strncasecmp(const char *_l, const char *_r, size_t n) {
    const unsigned char *l = (void *)_l, *r = (void *)_r;
    if(n-- == 0) return 0;
    for (; *l && *r && n && tolower(*l) == tolower(*r); l++, r++, n--);
    return tolower(*l) - tolower(*r);
}
