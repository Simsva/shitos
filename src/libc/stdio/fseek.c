#include "stdio_impl.h"

int fseek(FILE *f, long off, int whence) {
    return f->seek(f, off, whence) < 0 ? -1 : 0;
}
