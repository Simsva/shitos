#include <ctype.h>

inline int isalpha(int c) {
    return ((unsigned)c|' ')-'a' < 26;
}
