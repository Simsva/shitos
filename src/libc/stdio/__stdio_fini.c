#include "stdio_impl.h"

void __stdio_fini(void) {
    /* close std* buffers? */
    if(stdout) fflush(stdout);
    if(stderr) fflush(stderr);
    while(__head) fclose(__head);
}
