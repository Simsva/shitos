#include <kernel/process.h>
#include <kernel/kmem.h>
#include <string.h>

static void _kidle(void);
static void _kburn(void);

tree_t *process_tree;
list_t *process_list;
list_t *process_queue;

static void _kidle(void) {
    for(;;) arch_pause();
}

static void _kburn(void) {
    for(;;) {
        arch_pause();
        /* switch_next(); */
    }
}

process_t *spawn_idle(int bsp) {
    process_t *idle = kmalloc(sizeof(process_t));
    memset(idle, 0, sizeof(process_t));
    idle->pid = -1;
    idle->name = strdup("[idle]");
    idle->flags = PROC_FLAG_TASKLET|PROC_FLAG_STARTED|PROC_FLAG_RUNNING;
    idle->kernel_stack = (uintptr_t)kmalloc_a(KERNEL_STACK_SZ) + KERNEL_STACK_SZ;

    idle->ip = bsp ? (uintptr_t)&_kidle : (uintptr_t)&_kburn;
    idle->sp = idle->kernel_stack;
    idle->bp = idle->kernel_stack;

    idle->pd = vmem_clone_dir(this_core->current_pd);
    return idle;
}

process_t *spawn_init(void) {
    process_t *init = kmalloc(sizeof(process_t));
    memset(init, 0, sizeof(process_t));
    tree_set_root(process_tree, (tree_item_t)init);

    init->tree_entry = process_tree->root;
    init->pid = 1;
    init->uid = init->euid = 0;
    init->gid = init->egid = 0;
    init->name = strdup("init");
    init->cmdline = NULL;

    /* TODO: file descriptors */

    init->flags = PROC_FLAG_STARTED|PROC_FLAG_RUNNING;
    init->kernel_stack = (uintptr_t)kmalloc_a(KERNEL_STACK_SZ) + KERNEL_STACK_SZ;

    init->sched_node.value = (list_item_t)init;
    init->pd = this_core->current_pd;

    list_push(process_list, (list_item_t)init);
    return init;
}

void process_init(void) {
    process_tree = tree_create();
    process_list = list_create();
    process_queue = list_create();

    this_core->kernel_idle_proc = spawn_idle(1);
    this_core->current_proc = spawn_init();
}
