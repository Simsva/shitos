#include <ctype.h>

inline int toupper(int c) {
    return islower(c) ? c&0x5f : c;
}
