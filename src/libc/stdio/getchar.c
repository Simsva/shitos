#include "stdio_impl.h"

int getchar(void) {
    return fgetc(stdin);
}
