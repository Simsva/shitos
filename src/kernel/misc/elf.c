#include <elf.h>
#include <kernel/fs.h>
#include <kernel/process.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <features.h>

/* just randomly chosen numbers */
#define USER_STACK_SZ       0x20000
#define USER_HEAP_SZ        0x40000
/* NOTE: maybe let the heap grow up to the stack? */
#define USER_HEAP_MAX_SZ    0x80000
#define USER_HEAP_INDEX_SZ  0x400

#include <kernel/arch/i386/arch.h>

int elf_exec(__unused const char *path, fs_node_t *file, int argc, char *const argv[], char *const envp[], __unused int interp_depth) {
    Elf32_Ehdr hdr;
    int ret = 0;

    fs_read(file, 0, sizeof(Elf32_Ehdr), (uint8_t *)&hdr);

    if(memcmp(hdr.e_ident, ELFMAG, SELFMAG)) {
        ret = -EINVAL;
        goto ret_close;
    }

    if(hdr.e_ident[EI_CLASS] != ELFCLASS32) {
        ret = -EINVAL;
        goto ret_close;
    }

    if(hdr.e_type != ET_EXEC) {
        ret = -EINVAL;
        goto ret_close;
    }

    /* SUID, TODO: not if tracing */
    if(file->mask & S_ISUID)
        this_core->current_proc->euid = file->uid;

    uintptr_t heap_base = 0;

    /* load program headers */
    /* TODO: dynamic segments */
    for(unsigned i = 0; i < hdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        fs_read(file, hdr.e_phoff + hdr.e_phentsize*i, sizeof(Elf32_Phdr), (uint8_t *)&phdr);

        if(phdr.p_type == PT_LOAD) {
            for(uintptr_t addr = phdr.p_vaddr; addr < phdr.p_vaddr + phdr.p_memsz; addr += PAGE_SIZE)
                vmem_frame_alloc(vmem_get_page(addr, VMEM_GET_CREATE), VMEM_FLAG_WRITE);

            fs_read(file, phdr.p_offset, phdr.p_filesz, (uint8_t *)phdr.p_vaddr);
            memset((void *)phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);

            /* place the heap after the last segment */
            if(phdr.p_vaddr + phdr.p_memsz > heap_base)
                heap_base = phdr.p_vaddr + phdr.p_memsz;
        }
    }

    /* create heap */
    heap_base = (heap_base + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    for(uintptr_t addr = heap_base; addr < heap_base + USER_HEAP_SZ; addr += PAGE_SIZE)
        vmem_frame_alloc(vmem_get_page(addr, VMEM_GET_CREATE), VMEM_FLAG_WRITE);

    vmem_heap_create((vmem_heap_t *)&this_core->current_proc->heap,
                     (void *)heap_base,
                     (void *)heap_base + USER_HEAP_SZ,
                     (void *)heap_base + USER_HEAP_MAX_SZ,
                     USER_HEAP_INDEX_SZ, 0);

    this_core->current_proc->entry = (uintptr_t)hdr.e_entry;

    fs_close(file);

    /* map stack */
    uintptr_t stack = UINT32_C(0x80000000);
    for(uintptr_t addr = stack - USER_STACK_SZ; addr < stack; addr += PAGE_SIZE)
        vmem_frame_alloc(vmem_get_page(addr, VMEM_GET_CREATE), VMEM_FLAG_WRITE);

    /* leave space for argv and envp */
    this_core->current_proc->user_stack = stack - 0x1000;

    /* push and align a value on the stack */
#define PUSH(type, arg) { \
        stack -= sizeof(type); \
        while(stack & (sizeof(type)-1)) stack--; \
        *((type *)stack) = arg; \
    }
#define PUSHSTR(s) { \
        size_t l = strlen(s); \
        do PUSH(char, (s)[l]) \
        while(l-- > 0); \
    }

    int envc;
    char **_argv, **_envp;
    {
        /* arguments */
        char *argv_ptrs[argc];
        for(int i = 0; i < argc; i++) {
            PUSHSTR(argv[i]);
            argv_ptrs[i] = (char *)stack;
        }

        /* environment */
        envc = 0;
        char *const *envp_tmp = envp;
        while(*envp_tmp) envp_tmp++, envc++;
        char *envp_ptrs[envc];
        for(int i = 0; i < envc; i++) {
            PUSHSTR(envp[i]);
            envp_ptrs[i] = (char *)stack;
        }

        /* main arguments */
        PUSH(uintptr_t, 0); /* envp NULL */
        for(int i = envc; i > 0; i--) {
            PUSH(char *, envp_ptrs[i-1]);
        }
        _envp = (char **)stack;
        PUSH(uintptr_t, 0); /* argv NULL */
        for(int i = argc; i > 0; i--) {
            PUSH(char *, argv_ptrs[i-1]);
        }
        _argv = (char **)stack;
        PUSH(int, argc);
    }

#undef PUSH
#undef PUSHSTR

    arch_set_kernel_stack(this_core->current_proc->kernel_stack);
    arch_enter_user(hdr.e_entry, argc, _argv, envc, _envp, stack);

    return -EINVAL;
ret_close:
    fs_close(file);
    return ret;
}
