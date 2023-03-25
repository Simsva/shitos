#include <kernel/tty/tm.h>
#include <boot/def.h>
#include <boot/boot_opts.h>
#include <sys/utils.h>
#include <stdio.h>

#include <kernel/fs.h>
#include <kernel/list.h>
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

    int i;
    list_t *list = list_create(), *list2;
    for(i = 0; i < 10; i++)
        list_push_item(list, (list_item_t)i);

    i = 0;
    list_foreach(node, list)
        printf("item %d: %d\n", i++, (intptr_t)node->value);

    list2 = list_copy(list);
    list_insert_item(list2, 2, (list_item_t)42);

    i = list2->sz - 1;
    list_foreachr(node, list2)
        printf("item2 %d: %d\n", i--, (intptr_t)node->value);

    list_free(list);
    list_free(list2);

    for(;;) asm("hlt");
}
