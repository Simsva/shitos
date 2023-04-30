#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syscall.h>

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    char nl = '\n';

    syscall_sysfunc(3, NULL);
    volatile char *ptr = malloc(10);
    volatile char *ptr2 = malloc(5);

    syscall_write(1, &nl, 1);
    syscall_sysfunc(3, NULL);
    ptr = realloc((void *)ptr, 20);

    syscall_write(1, &nl, 1);
    syscall_sysfunc(3, NULL);
    ptr2 = realloc((void *)ptr2, 10);

    syscall_write(1, &nl, 1);
    syscall_sysfunc(3, NULL);

    return 0;
}
