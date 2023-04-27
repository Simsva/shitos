#include <stdlib.h>

_Noreturn void _Exit(__unused int ec) {
    for(;;);
}
