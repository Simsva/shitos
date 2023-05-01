#include <ctype.h>

inline int tolower(int c) {
    return isupper(c) ? c|' ' : c;
}
