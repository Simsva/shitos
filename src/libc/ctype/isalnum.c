#include <ctype.h>

inline int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}
