#include <syscall.h>
#include <syscall_nums.h>

DEFN_SYSCALL1(exit,       SYS_exit,       int);
DEFN_SYSCALL3(open,       SYS_open,       const char *, unsigned, unsigned);
DEFN_SYSCALL1(close,      SYS_close,      int);
DEFN_SYSCALL3(read,       SYS_read,       int, char *, size_t);
DEFN_SYSCALL3(write,      SYS_write,      int, const char *, size_t);
DEFN_SYSCALL3(seek,       SYS_seek,       int, long, int);
