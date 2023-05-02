#include <ctype.h>

inline int isupper(int c) {
    return (unsigned)c-'A' < 26;
}
