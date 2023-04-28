#include <kernel/process.h>
#include <kernel/kmem.h>
#include <string.h>

#include <kernel/arch/i386/arch.h>

static void _kidle(void);
static void _kburn(void);

tree_t *process_tree;
list_t *process_list;
list_t *process_queue;

process_local_t _this_core;

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
    init->wd_path = strdup("/");
    init->wd_node = fs_node_clone(TREE_TO_FS_NODE(fs_tree->root));

    init->fds = kmalloc(sizeof(fd_table_t));
    init->fds->refcount = 1;
    init->fds->sz = 0;
    init->fds->max_sz = 4;
    init->fds->entries = kmalloc(init->fds->max_sz * sizeof *init->fds->entries);
    memset(init->fds->entries, 0, init->fds->max_sz * sizeof *init->fds->entries);
    init->fds->modes = kmalloc(init->fds->max_sz * sizeof *init->fds->modes);
    init->fds->offs = kmalloc(init->fds->max_sz * sizeof *init->fds->offs);

    init->flags = PROC_FLAG_STARTED|PROC_FLAG_RUNNING;
    init->kernel_stack = (uintptr_t)kmalloc_a(KERNEL_STACK_SZ) + KERNEL_STACK_SZ;

    init->sched_node.value = (list_item_t)init;
    init->pd = this_core->current_pd;

    list_push(process_list, (list_item_t)init);
    return init;
}

/**
 * Bind a filesystem node to a new file descriptor in a process.
 */
int process_add_fd(process_t *proc, fs_node_t *node) {
    /* fill gaps */
    for(size_t i = 0; i < proc->fds->sz; i++) {
        if(proc->fds->entries[i]) continue;

        proc->fds->entries[i] = node;
        /* set by caller */
        proc->fds->modes[i] = 0;
        proc->fds->offs[i] = 0;

        return i;
    }

    /* extend */
    if(proc->fds->sz == proc->fds->max_sz) {
        size_t oldsz = proc->fds->max_sz;
        proc->fds->max_sz *= 2;
        proc->fds->entries = krealloc(proc->fds->entries,
                                      proc->fds->max_sz * sizeof *proc->fds->entries);
        proc->fds->modes   = krealloc(proc->fds->modes,
                                      proc->fds->max_sz * sizeof *proc->fds->modes);
        proc->fds->offs    = krealloc(proc->fds->offs,
                                      proc->fds->max_sz * sizeof *proc->fds->offs);

        /* make sure all new entries are NULL */
        memset(proc->fds->entries + oldsz, 0,
               (proc->fds->max_sz - oldsz) * sizeof *proc->fds->entries);
    }
    proc->fds->entries[proc->fds->sz] = node;
    proc->fds->modes[proc->fds->sz] = 0;
    proc->fds->offs[proc->fds->sz] = 0;
    return proc->fds->sz++;
}

void process_exit(__unused int ec) {
    /* TODO: SIGCHLD and task switching */
    for(;;) asm volatile("hlt");
}

void process_init(void) {
    process_tree = tree_create();
    process_list = list_create();
    process_queue = list_create();

    this_core->kernel_idle_proc = spawn_idle(1);
    this_core->current_proc = spawn_init();
}
