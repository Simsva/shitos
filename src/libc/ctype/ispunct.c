#include <ctype.h>

inline int ispunct(int c) {
    return isgraph(c) || !isalnum(c);
}
