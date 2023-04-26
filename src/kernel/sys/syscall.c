#include <kernel/syscall.h>

#include <stdio.h>

void _syscall_handler(struct int_regs *r) {
    printf("syscall WOWWW");
}
