#include <kernel/tty.h>
#include <kernel/fs.h>
#include <kernel/console.h>
#include <kernel/video.h>
#include <kernel/args.h>
#include <ext2fs/ext2.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/psf.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

struct kernel_args kernel_args;

/* TODO: move */
void ps2hid_install(void);
void ide_init(void);
void dospart_init(void);
void bootpart_init(void);

void kmain(struct kernel_args *args) {
    memcpy(&kernel_args, args, sizeof kernel_args);

    vfs_install();
    vfs_map_directory("/dev");
    console_install();
    ide_init();
    dospart_init();
    bootpart_init();
    ext2fs_init();
    zero_install();
    random_install();
    ps2hid_install();

    /* TODO: automatically detect devices somehow */
    vfs_mount_type("dospart", "/dev/ada", NULL);
    vfs_mount_type("ext2fs", "/dev/ada2,rw,verbose", "/");
    vfs_mount_type("bootpart", "/dev/ada1,verbose", "/boot");

    fb_init();
    if(kernel_args.video_mode == VIDEO_TEXT)
        tm_term_install();
    else
        fb_term_install("/usr/share/consolefonts/default8x16.psfu");

    puts("Welcome to ShitOS (" EXPAND_STR(_ARCH) ")");

    int i, j, n;
    for(i = 0; i < 11; i++) {
        for(j = 0; j < 10; j++) {
            n = 10 * i + j;
            if(n > 108) break;
            printf("\033[%dm %3d\033[m", n, n);
        }
        printf("\n");
    }

    printf("\033[48;5;114m                ");

    for(;;) asm volatile("hlt");
}
