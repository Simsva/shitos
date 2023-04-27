#ifndef KERNEL_PROCESS_H_
#define KERNEL_PROCESS_H_

#include <kernel/vmem.h>
#include <kernel/tree.h>
#include <kernel/list.h>
#include <sys/types.h>

#define KERNEL_STACK_SZ 0x8000

#define PROC_FLAG_TASKLET   0x01
#define PROC_FLAG_FINISHED  0x02
#define PROC_FLAG_STARTED   0x04
#define PROC_FLAG_RUNNING   0x08
#define PROC_FLAG_SLEEP     0x10
#define PROC_FLAG_SUSPENDED 0x20

extern tree_t *process_tree;  /* process tree */
extern list_t *process_list;  /* process flat list */
extern list_t *process_queue; /* scheduler queue */

typedef struct process {
    pid_t pid;
    uid_t uid, euid;
    gid_t gid, egid;

    unsigned flags;

    char *name, **cmdline;

    uintptr_t sp, bp, ip;
    uintptr_t user_stack, kernel_stack, heap;
    uintptr_t entry;

    tree_node_t *tree_entry;
    list_node_t sched_node;

    page_directory_t *pd;
} process_t;

typedef struct processor_local {
    volatile process_t *current_proc;
    process_t *kernel_idle_proc;
    page_directory_t *current_pd;
} process_local_t;

extern process_local_t _this_core;
static process_local_t *const this_core = &_this_core;

process_t *spawn_idle(int bsp);
void process_init(void);

int binfmt_exec(const char *path, int argc, char *const argv[], char *const envp[], int interp_depth);
int binfmt_system(const char *path, int argc, char *const argv[], char *const envp[]);

#endif // KERNEL_PROCESS_H_
