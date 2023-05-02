#include "stdio_impl.h"

void clearerr(FILE *f) {
    f->eof = f->error = 0;
}
