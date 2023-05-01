#include <ctype.h>

inline int isxdigit(int c) {
    return isdigit(c) || ((unsigned)c|' ')-'a' < 6;
}
