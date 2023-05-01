#include <ctype.h>

inline int iscntrl(int c) {
    return (unsigned)c < ' ' || c == 0x7f;
}
