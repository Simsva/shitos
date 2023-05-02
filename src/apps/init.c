#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <string.h>

#define MAX_STR 32

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    /* emulate blocking I/O in libc */
    stdin->flags |= 4;

    /* TODO: ascii + display characters as they are typed */
    /* this is all disgusting jank because we don't have proper ttys */
    char buf[MAX_STR];
    for(;;) {
        printf("$ ");
        fflush(stdout);
        fgets(buf, sizeof buf, stdin);
        printf("you entered: ");
        size_t sz = strlen(buf);
        for(size_t i = 0; i < sz; i++)
            printf("%02X", (unsigned char)buf[i]);
        putchar('\n');
    }

    return EXIT_SUCCESS;
}
