#include <kernel/syscall.h>
#include <kernel/process.h>
#include <syscall_nums.h>
#include <stdio.h>
#include <errno.h>

#include <kernel/arch/i386/arch.h>

typedef long (*syscall_fn)(long, long, long, long, long, long);

_Noreturn long sys_exit(int ec) {
    process_exit(ec & 0xff);
    __builtin_unreachable();
}

long sys_open(const char *path, unsigned flags, mode_t mode) {
    printf("open(\"%s\", %#x, %#o)\n", path, flags, mode);
    return 0;
}

long sys_close(int fd) {
    printf("close(%d)\n", fd);
    return 0;
}

long sys_read(int fd, char *buf, size_t sz) {
    printf("read(%d, %p, %zu)\n", fd, buf, sz);
    return 0;
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
