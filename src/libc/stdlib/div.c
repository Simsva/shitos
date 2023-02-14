#include <stdlib.h>

div_t div(int a, int b) {
    return (div_t){ a/b, a%b };
}
