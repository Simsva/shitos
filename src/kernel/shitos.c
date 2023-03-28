#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/kmem.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

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

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    vfs_install();
    vfs_map_directory("/dev");
    random_install();

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    fs_node_t *random = kopen("/dev/random", 0);
    uint8_t buf[8];
    fs_read(random, 0, sizeof buf, buf);

    printf("random bytes: ");
    for(size_t i = 0; i < sizeof buf; i++)
        printf("%02X", buf[i]);
    putchar('\n');

    fs_close(random);
    kfree(random);

    for(;;) asm("hlt");
}
