#include <stdio.h>

_Noreturn void __assert_fail(const char *expr, const char *file, int line, const char *func) {
    /* TODO: fprintf and abort */
    printf("Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
    for(;;);
}
