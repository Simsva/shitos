#include <ctype.h>

inline int isascii(int c) {
    return !(c&~0x7f);
}
