#include <stdlib.h>

extern void _fini(void);

_Noreturn void exit(int ec) {
    _fini();
    _Exit(ec);
}
