#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

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

    vfs_install();
    vfs_map_directory("/dev");
    zero_install();
    random_install();
    console_install();
    ps2hid_install();
    ide_init();

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    fs_node_t *ada = kopen("/dev/ada", 0);
    uint8_t buf[32];
    fs_read(ada, 0, sizeof buf, buf);
    printf("/dev/ada LBA 0: ");
    for(uint8_t i = 0; i < sizeof buf; i++)
        printf("%02X ", buf[i]);

    for(;;) asm("hlt");
}
