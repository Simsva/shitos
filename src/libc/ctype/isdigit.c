#include <ctype.h>

inline int isdigit(int c) {
    return (unsigned)c-'0' < 10;
}
