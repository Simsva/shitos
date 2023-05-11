#include <kernel/syscall.h>
#include <kernel/process.h>
#include <sys/stat.h>
#include <syscall_nums.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <kernel/arch/i386/arch.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef long (*syscall_fn)(long, long, long, long, long, long);

int ptr_validate(const void *ptr) {
    if(ptr) {
        if(!PTR_INRANGE(ptr)) {
            /* TODO: SIGSEGV */
            return 1;
        }
        if(!vmem_validate_user_ptr(ptr, 0, 0)) return 1;
    }
    return 0;
}

_Noreturn long sys_exit(int ec) {
    process_exit(ec & 0xff);
    __builtin_unreachable();
}

long sys_open(const char *path, unsigned flags, mode_t mode) {
    int ret = 0;

    PTR_VALIDATE(path);
    if(!path) return -EFAULT;

    fs_node_t *node = kopen(path, flags);

    /* excl */
    if(node && (flags & O_CREAT) && (flags & O_EXCL)) {
        ret = -EEXIST;
        goto close_ret;
    }

    /* creat */
    if(!node && (flags & O_CREAT)) {
        /* TODO: check dir permission, EACCESS */
        int err;
        if(!(err = fs_mknod(path, (mode & 07777) | S_IFREG)))
            node = kopen(path, flags);
        else
            return err;

        /* if kopen failed */
        if(!node) return -errno;
    }
    /* ENOENT if file does not exist and was not created */
    if(!node) return -ENOENT;

    unsigned access = 0;
    /* read access */
    if((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_RDWR) {
        /* TODO: check permission, EACCESS */
        access |= R_OK;
    }

    /* write access */
    if((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
        /* TODO: check permission, EACCESS */
        if(FS_ISDIR(node->flags)) {
            ret = -EISDIR;
            goto close_ret;
        }
        access |= W_OK;
    }

    /* directory */
    if((flags & O_DIRECTORY) && !FS_ISDIR(node->flags)) {
        ret = -ENOTDIR;
        goto close_ret;
    }

    /* trunc */
    if(flags & O_TRUNC) {
        if(!(access & 02)) {
            ret = -EINVAL;
            goto close_ret;
        }
        fs_truncate(node, 0);
    }

    int fd = process_add_fd((process_t *)this_core->current_proc, node);
    FD_MODE(fd) = access;
    FD_OFFSET(fd) = (flags & O_APPEND) ? node->sz : 0;
    return fd;
close_ret:
    fs_close(node);
    return ret;
}

long sys_close(int fd) {
    if(!FD_CHECK(fd)) return -EBADFD;

    fs_close(FD_ENTRY(fd));
    FD_ENTRY(fd) = NULL;
    return 0;
}

long sys_read(int fd, char *buf, size_t sz) {
    if(!FD_CHECK(fd)) return -EBADFD;
    PTR_CHECK(buf, sz, VMEM_PTR_FLAG_WRITE);

    fs_node_t *node = FD_ENTRY(fd);
    if(!(FD_MODE(fd) & R_OK)) return -EACCES;
    ssize_t out = fs_read(node, FD_OFFSET(fd), sz, (uint8_t *)buf);
    if(out < 0) return out;
    FD_OFFSET(fd) += out;

    return (long)out;
}

long sys_write(int fd, const char *buf, size_t sz) {
    if(!FD_CHECK(fd)) return -EBADFD;
    PTR_CHECK(buf, sz, 0);

    fs_node_t *node = FD_ENTRY(fd);
    if(!(FD_MODE(fd) & W_OK)) return -EACCES;
    ssize_t out = fs_write(node, FD_OFFSET(fd), sz, (uint8_t *)buf);
    if(out < 0) return out;
    FD_OFFSET(fd) += out;

    return (long)out;
}

/* TODO: prevent negative offs? */
long sys_seek(int fd, long off, int whence) {
    if(!FD_CHECK(fd)) return -EBADFD;
    if(FS_ISCHR(FD_ENTRY(fd)->flags) || FS_ISFIFO(FD_ENTRY(fd)->flags)
       || FS_ISSOCK(FD_ENTRY(fd)->flags))
        return -ESPIPE;

    switch(whence) {
    case SEEK_SET: return FD_OFFSET(fd) = off;
    case SEEK_CUR: return FD_OFFSET(fd) += off;
    case SEEK_END: return FD_OFFSET(fd) = FD_ENTRY(fd)->sz + off;
    default:       return -EINVAL;
    }
}

/* AKA random hacks, the syscall */
long sys_sysfunc(long request, void *args) {
    switch(request) {
    case 0: /* malloc */
        PTR_VALIDATE(args);
        return this_core->current_proc->heap.start
            ? (long)vmem_heap_alloc((vmem_heap_t *)&this_core->current_proc->heap,
                                    *(size_t *)args, 0)
            : 0;
    case 1: /* realloc */
        PTR_VALIDATE(args);
        return this_core->current_proc->heap.start
            ? (long)vmem_heap_realloc((vmem_heap_t *)&this_core->current_proc->heap,
                                      (void *)((uintptr_t *)args)[0],
                                      (size_t)((uintptr_t *)args)[1])
            : 0;
    case 2: /* free */
        PTR_VALIDATE(args);
        if(this_core->current_proc->heap.start)
            vmem_heap_free((vmem_heap_t *)&this_core->current_proc->heap,
                           *(void **)args);
        return 0;
    case 3: /* vmem_heap_dump */
        if(this_core->current_proc->heap.start)
            vmem_heap_dump((vmem_heap_t *)&this_core->current_proc->heap);
        return 0;

    case 4: /* millis */
        PTR_VALIDATE(args);
        extern uintmax_t pit_ticks;
        *((uintmax_t *)args) = pit_ticks;
        return 0;

    default:
        printf("Bad sysfunc: %ld\n", request);
        return -EINVAL;
    }
}

long sys_mknod(const char *path, mode_t mode) {
    PTR_VALIDATE(path);
    if(!path) return -EFAULT;

    return (long)fs_mknod(path, mode);
}

long sys_unlink(const char *path) {
    PTR_VALIDATE(path);
    if(!path) return -EFAULT;

    return (long)fs_unlink(path);
}

long sys_ioctl(int fd, long request, void *argp) {
    if(!FD_CHECK(fd)) return -EBADFD;
    PTR_VALIDATE(argp);
    return fs_ioctl(FD_ENTRY(fd), request, argp);
}

long sys_getcwd(char *buf, size_t sz) {
    PTR_CHECK(buf, sz, VMEM_PTR_FLAG_WRITE);
    size_t len = strlen(this_core->current_proc->wd_path) + 1;
    if(sz < len) return -ERANGE;
    return (long)memcpy(buf, this_core->current_proc->wd_path, len);
}

long sys_chdir(const char *newpath) {
    PTR_VALIDATE(newpath);
    int ret = 0;
    size_t len;
    char *path = canonicalize_path(this_core->current_proc->wd_path, newpath, &len);

    fs_node_t *node = kopen(path, 0);
    if(!node) {
        ret = -ENOENT;
        goto ret_free;
    }
    if(!FS_ISDIR(node->flags)) {
        fs_close(node);
        ret = -ENOTDIR;
        goto ret_free;
    }
    /* TODO: check permission */

    kfree(this_core->current_proc->wd_path);
    fs_close(this_core->current_proc->wd_node);
    this_core->current_proc->wd_path = path;
    this_core->current_proc->wd_node = node; /* is this actually used anywhere? */
    return 0;
ret_free:
    kfree(path);
    return ret;
}

long sys_readdir(int fd, long idx, struct dirent *ent) {
    if(!FD_CHECK(fd)) return -EBADFD;
    if(!ent) return -EFAULT;
    PTR_VALIDATE(ent);
    struct dirent *kent = fs_readdir(FD_ENTRY(fd), (off_t)idx);
    if(!kent) return 0;
    memcpy(ent, kent, sizeof *ent);
    kfree(kent);
    return 1;
}

/* this system should work unless we use
 * floating point arguments (which we don't) */
#define SYSCALL(fn) ((syscall_fn)(uintptr_t)(fn))
static const syscall_fn syscalls[NUM_SYSCALLS] = {
    [SYS_exit]      = SYSCALL(sys_exit),
    [SYS_open]      = SYSCALL(sys_open),
    [SYS_close]     = SYSCALL(sys_close),
    [SYS_read]      = SYSCALL(sys_read),
    [SYS_write]     = SYSCALL(sys_write),
    [SYS_seek]      = SYSCALL(sys_seek),
    [SYS_sysfunc]   = SYSCALL(sys_sysfunc),
    [SYS_mknod]     = SYSCALL(sys_mknod),
    [SYS_unlink]    = SYSCALL(sys_unlink),
    [SYS_ioctl]     = SYSCALL(sys_ioctl),
    [SYS_getcwd]    = SYSCALL(sys_getcwd),
    [SYS_chdir]     = SYSCALL(sys_chdir),
    [SYS_readdir]   = SYSCALL(sys_readdir),
};
#undef SYSCALL

void _syscall_handler(struct int_regs *r) {
    long num = arch_syscall_num(r);

    if(num >= NUM_SYSCALLS) {
        arch_syscall_ret(r, -EINVAL);
        goto ret;
    }

    syscall_fn fn = syscalls[num];
    long res = fn(arch_syscall_arg0(r), arch_syscall_arg1(r), arch_syscall_arg2(r),
                  arch_syscall_arg3(r), arch_syscall_arg4(r), arch_syscall_arg5(r));
    arch_syscall_ret(r, res);

ret:
    /* prevent optimizations which clobber the stack */
    asm volatile("");
    return;
}
