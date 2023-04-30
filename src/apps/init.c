#include <features.h>
#include <fcntl.h>
#include <syscall.h>

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    int fd = syscall_open("/usr/include/_cheader.h", O_RDWR, 0);
    char buf[3] = "aa";
    syscall_read(fd, buf, sizeof buf);
    syscall_seek(fd, 10, 0);
    syscall_write(fd, buf, sizeof buf);
    syscall_close(fd);

    return 0;
}
