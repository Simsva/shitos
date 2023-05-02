#include <string.h>

char *strrchr(const char *s, int c) {
    return memchr(s, c, strlen(s) + 1);
}
