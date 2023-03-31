#include <kernel/tty/tm.h>
#include <kernel/args.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/fs.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

struct kernel_args kernel_args;

/* TODO: move */
void ps2hid_install(void);
void ide_init(void);

static void tree_print_fs(tree_item_t item) {
    struct vfs_entry *entry = item;
    if(entry->file)
        printf("%s -> %p : %u\n", entry->name, entry->file->device, entry->file->inode);
    else
        printf("%s\n", entry->name);
}

void kmain(struct kernel_args *args) {
    tm_cur_x = args->tm_cursor % 80;
    tm_cur_y = args->tm_cursor / 80;
    memcpy(&kernel_args, args, sizeof kernel_args);

    vfs_install();
    vfs_map_directory("/dev");
    console_install();
    zero_install();
    random_install();
    ps2hid_install();
    ide_init();

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    char buf[16];
    snprintf(buf, sizeof buf, "/dev/ada%d", 1);
    printf("buf: %s\n", buf);

    for(;;) asm volatile("hlt");
}
