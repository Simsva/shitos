#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    char buf[256];
    fs_init_test();
    buf[fs_read(fs_root, 0, SIZE_MAX, (uint8_t *)buf)] = '\0';
    printf("test_read: \"%s\"\n", buf);

    for(;;) asm("hlt");
}
