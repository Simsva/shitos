#include <features.h>
#include <syscall.h>

int main(__unused int argc, __unused char *argv[]) {
    syscall_test0();
    syscall_test6(1, 2, 3, 4, 5, 6);

    return 0;
}
