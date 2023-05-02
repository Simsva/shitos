#include <ctype.h>

inline int islower(int c) {
    return (unsigned)c-'a' < 26;
}
