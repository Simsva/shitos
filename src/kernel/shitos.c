#include <kernel/tty/tm.h>
#include <kernel/fs.h>
#include <kernel/args.h>
#include <stdio.h>
#include <string.h>

#include <kernel/arch/i386/ports.h>

#include <kernel/hashmap.h>

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
    /* zero_install(); */
    /* random_install(); */
    /* ps2hid_install(); */
    /* ide_init(); */

    puts("Booting ShitOS (" EXPAND_STR(_ARCH) ")");

    printf("fs_tree:\n");
    tree_debug_dump(fs_tree, tree_print_fs);

    hashmap_t *map = hashmap_create_str(2);
    map->value_free = NULL;
    hashmap_set(map, "hello", (void *)13);
    hashmap_set(map, "world", (void *)37);
    hashmap_set(map, "foo", (void *)69);
    hashmap_set(map, "bar", (void *)96);

    hashmap_delete(map, "world");

    printf("hello world foo bar : %d %d %d %d\n",
           (int)hashmap_get(map, "hello"), (int)hashmap_get(map, "world"),
           (int)hashmap_get(map, "foo"), (int)hashmap_get(map, "bar"));

    hashmap_free(map);

    for(;;) asm volatile("hlt");
}
