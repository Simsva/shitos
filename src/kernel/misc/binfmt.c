#include <kernel/process.h>
#include <kernel/fs.h>
#include <kernel/kmem.h>
#include <string.h>
#include <errno.h>
#include <elf.h>

#define MAX_FMT_LEN 4

int elf_exec(const char *, fs_node_t *, int, char *const[], char *const[], int);

typedef int (*exec_func)(const char *, fs_node_t *, int, char *const[], char *const[], int);
struct binfmt_entry {
    exec_func exec;
    uint8_t magic[MAX_FMT_LEN];
    unsigned len;
    char *name;
} binfmt_fmts[] = {
    {elf_exec, {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3}, 4, "ELF"},
};

int binfmt_exec(const char *path, int argc, char *const argv[], char *const envp[], int interp_depth) {
    fs_node_t *file = kopen(path, 0);
    if(!file) return -ENOENT;
    /* TODO: check permission */

    uint8_t magic[MAX_FMT_LEN];
    fs_read(file, 0, sizeof magic, magic);

    this_core->current_proc->name = strdup(path);

    for(struct binfmt_entry *e = binfmt_fmts; e < binfmt_fmts + sizeof(binfmt_fmts) / sizeof(*binfmt_fmts); e++)
        if(!memcmp(e->magic, magic, e->len))
            return e->exec(path, file, argc, argv, envp, interp_depth);
    return -ENOEXEC;
}

int binfmt_system(const char *path, int argc, char *const argv[], char *const envp[]) {
    char **argv_ = kmalloc(sizeof(char *) * (argc + 1));
    for(int j = 0; j < argc; j++)
        argv_[j] = strdup(argv[j]);
    argv_[argc] = NULL;

    char *null_env[] = {NULL};
    this_core->current_proc->pd = vmem_clone_dir(NULL);
    vmem_set_dir(this_core->current_proc->pd);
    this_core->current_proc->cmdline = argv_;
    return binfmt_exec(path, argc, argv_, envp ? envp : null_env, 0);
}
