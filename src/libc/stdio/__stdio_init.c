#include "stdio_impl.h"
#include <stdlib.h>

void __stdio_init(void) {
    __stdout_FILE.buf = malloc(BUFSIZ);
}
