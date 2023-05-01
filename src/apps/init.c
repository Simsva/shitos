#include <features.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <stdint.h>
#include <string.h>
#include <kernel/video.h>

uint32_t *fb = (void *)0x70000000;

int main(__unused int argc, __unused char *argv[]) {
    /* TODO: /dev/null and fix these later (tty) */
    syscall_open("/dev/kbd", O_RDONLY, 0); /* fd 0: stdin */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 1: stdout */
    syscall_open("/dev/console", O_WRONLY, 0); /* fd 2: stderr */

    FILE *fbfile = fopen("/dev/fb0", "r");
    syscall_ioctl(fbfile->fd, IOCTL_VID_MAP, &fb);
    memset(fb, 0xff, 1280*4*10);

    fclose(fbfile);
    return EXIT_SUCCESS;
}
