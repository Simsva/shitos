#include <stdlib.h>

extern void _fini(void);
extern void __stdio_fini(void);

_Noreturn void exit(int ec) {
    _fini();
    __stdio_fini();
    _Exit(ec);
}
