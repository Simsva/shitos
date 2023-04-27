#include <features.h>

int main(__unused int argc, __unused char *argv[]) {
    asm volatile("int $0x30");

    return 0;
}
