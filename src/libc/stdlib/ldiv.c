#include <stdlib.h>

ldiv_t ldiv(long a, long b) {
    return (ldiv_t){ a/b, a%b };
}
