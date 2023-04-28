#include <kernel/syscall.h>
#include <kernel/process.h>
#include <sys/stat.h>
#include <syscall_nums.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <kernel/arch/i386/arch.h>

typedef long (*syscall_fn)(long, long, long, long, long, long);

int ptr_validate(void *ptr) {
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

    printf("open(\"%s\", %#o, %#o)\n", path, flags, mode);

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
        if(!(err = fs_mknod(path, (mode & 0777) | S_IFREG)))
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
    printf("close(%d)\n", fd);
    return 0;
}

long sys_read(int fd, char *buf, size_t sz) {
    if(!FD_CHECK(fd)) return -EBADFD;
    PTR_CHECK(buf, sz, VMEM_PTR_FLAG_WRITE);

    printf("read(%d, %p, %zu)\n", fd, buf, sz);

    fs_node_t *node = FD_ENTRY(fd);
    if(!(FD_MODE(fd) & R_OK)) return -EACCES;
    ssize_t out = fs_read(node, FD_OFFSET(fd), sz, (uint8_t *)buf);
    if(out < 0) return out;
    FD_OFFSET(fd) += out;

    return (long)out;
}

long sys_write(int fd, const char *buf, size_t sz) {
    printf("write(%d, %p, %zu)\n", fd, buf, sz);
    return 0;
}

long sys_seek(int fd, long off, int whence) {
    printf("seek(%d, %ld, %d)\n", fd, off, whence);
    return 0;
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
