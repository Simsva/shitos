#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/list.h>
#include <kernel/tree.h>
#include <kernel/vmem.h>

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

    tree_t *tree = tree_create();

    tree_set_root(tree, (tree_item_t)1);
    tree_node_t *node1 = tree_insert_item(tree, tree->root, (tree_item_t)2);
    tree_node_t *node2 = tree_insert_item(tree, tree->root, (tree_item_t)3);
    tree_insert_item(tree, node1, (tree_item_t)4);
    tree_insert_item(tree, node2, (tree_item_t)5);

    printf("tree heap:\n");
    vmem_heap_dump(&kheap);

    tree_free(tree);

    printf("tree heap post-free:\n");
    vmem_heap_dump(&kheap);

    for(;;) asm("hlt");
}
