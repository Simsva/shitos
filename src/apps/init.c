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
    fseek(file, 0, SEEK_END);
    char buf[ftell(file)];
    rewind(file);

    fread(buf, 1, sizeof buf - 1, file);
    buf[sizeof buf - 1] = '\0';
    puts(buf);

    fclose(file);
    return EXIT_SUCCESS;
}
