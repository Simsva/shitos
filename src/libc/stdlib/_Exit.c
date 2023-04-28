#include <stdlib.h>
#include <syscall.h>

_Noreturn void _Exit(__unused int ec) {
    for(;;) syscall_exit(ec);
}
