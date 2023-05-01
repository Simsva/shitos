#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    FILE *file = fopen("/usr/include/_cheader.h", "r");
    char buf[32];
    while(fgets(buf, sizeof buf, file))
        printf("%s", buf);
    fclose(file);

    return EXIT_SUCCESS;
}
