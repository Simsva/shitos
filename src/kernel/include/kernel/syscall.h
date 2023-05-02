#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#include <kernel/process.h>
#include <kernel/kmem.h>
#include <stdint.h>
#include <errno.h>

#include <kernel/arch/i386/idt.h>

#define FD_INRANGE(fd) ((fd) < (int)this_core->current_proc->fds->sz && (fd) >= 0)
#define FD_ENTRY(fd) (this_core->current_proc->fds->entries[fd])
#define FD_CHECK(fd) (FD_INRANGE(fd) && FD_ENTRY(fd))
#define FD_OFFSET(fd) (this_core->current_proc->fds->offs[fd])
#define FD_MODE(fd) (this_core->current_proc->fds->modes[fd])

#define PTR_INRANGE(ptr) \
    ((uintptr_t)(ptr) > this_core->current_proc->entry && \
    ((uintptr_t)(ptr) < KERNEL_MAP))
#define PTR_VALIDATE(ptr) { if(ptr_validate((const void *)(ptr))) return -EINVAL; }
#define PTR_CHECK(ptr, sz, flags) \
    { if(!vmem_validate_user_ptr(ptr, sz, flags)) return -EFAULT; }

int ptr_validate(const void *ptr);

void _syscall_handler(struct int_regs *r);

#endif // KERNEL_SYSCALL_H_
