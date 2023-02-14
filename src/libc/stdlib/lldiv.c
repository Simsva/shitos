#include <stdlib.h>

lldiv_t lldiv(long long a, long long b) {
    return (lldiv_t){ a/b, a%b };
}
