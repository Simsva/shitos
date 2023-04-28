#include <features.h>
#include <fcntl.h>
#include <syscall.h>

int main(__unused int argc, __unused char *argv[]) {
    int fd = syscall_open("/usr/include/_cheader.h", O_RDWR, 0);
    char buf[3] = "aa";
    syscall_read(fd, buf, sizeof buf - 1);
    syscall_seek(fd, 10, 0);
    syscall_write(fd, buf, sizeof buf);
    syscall_close(fd);

    return 0;
}
