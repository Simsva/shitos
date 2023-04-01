#include <kernel/tty/tm.h>
#include <kernel/fs.h>
#include <kernel/args.h>
#include <ext2fs/ext2.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/hashmap.h>
#include <kernel/kmem.h>

#define STR(s) #s
#define EXPAND_STR(s) STR(s)

struct kernel_args kernel_args;

/* TODO: move */
void ps2hid_install(void);
void ide_init(void);
void dospart_init(void);

static void tree_print_fs(tree_item_t item) {
    struct vfs_entry *entry = item;
    printf("%s", entry->name);
    if(entry->fs_type)
        printf("[%s]", entry->fs_type);
    if(entry->file)
        printf(" -> %s : %u", entry->file->name, entry->file->inode);
    putchar('\n');
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
    dospart_init();
    ext2fs_init();

    /* TODO: automatically detect devices somehow */
    vfs_mount_type("dospart", "/dev/ada", NULL);
    vfs_mount_type("ext2fs", "/dev/ada1,rw,verbose", "/");

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    fs_node_t *kernel = kopen("/shitos.elf", 0),
              *random = kopen("/dev/random", 0);
    if(kernel) {
        printf("found kernel ELF: '%s'\nnow modifying self\n",
               kernel->name);

        /* address of "Booting ShitOS" string: 0xb598 */
        uint8_t buf[16];
        fs_read(kernel, 0xb598, sizeof buf, buf);

        /* randomize 5 chars, it is completely deterministic so it will always
         * produce the same string */
        fs_read(random, 0, 5, buf);
        for(uint8_t i = 0; i < 5; i++) buf[i] = 'A' + buf[i]%26;

        fs_write(kernel, 0xb598, sizeof buf, buf);
    }

    kfree(kernel);
    kfree(random);

    for(;;) asm volatile("hlt");
}
