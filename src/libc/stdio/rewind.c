#include "stdio_impl.h"

void rewind(FILE *f) {
    (void)fseek(f, 0L, SEEK_SET);
}
