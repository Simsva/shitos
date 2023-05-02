#include "stdio_impl.h"

int fclose(FILE *f) {
    if(!f || !f->close) return -1;
    return f->close(f);
}
