#include <kernel/syscall.h>
#include <syscall_nums.h>
#include <stdio.h>
#include <errno.h>

#include <kernel/arch/i386/arch.h>

typedef long (*syscall_fn)(long, long, long, long, long, long);

long sys_test0() {
    printf("test0\n");
    return 0;
}

long sys_test1(int a) {
    printf("test1: %d\n", a);
    return 0;
}

long sys_test2(int a, int b) {
    printf("test2: %d %d\n", a, b);
    return 0;
}

long sys_test3(int a, int b, int c) {
    printf("test3: %d %d %d\n", a, b, c);
    return 0;
}

long sys_test4(int a, int b, int c, int d) {
    printf("test4: %d %d %d %d\n", a, b, c, d);
    return 0;
}

long sys_test5(int a, int b, int c, int d, int e) {
    printf("test5: %d %d %d %d %d\n", a, b, c, d, e);
    return 0;
}

long sys_test6(int a, int b, int c, int d, int e, int f) {
    printf("test6: %d %d %d %d %d %d\n", a, b, c, d, e, f);
    return 0;
}

/* this system should work unless we use
 * floating point arguments (which we don't) */
#define SYSCALL(fn) ((syscall_fn)(uintptr_t)(fn))
static const syscall_fn syscalls[NUM_SYSCALLS] = {
    [SYS_test0] = SYSCALL(sys_test0),
    [SYS_test1] = SYSCALL(sys_test1),
    [SYS_test2] = SYSCALL(sys_test2),
    [SYS_test3] = SYSCALL(sys_test3),
    [SYS_test4] = SYSCALL(sys_test4),
    [SYS_test5] = SYSCALL(sys_test5),
    [SYS_test6] = SYSCALL(sys_test6),
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
