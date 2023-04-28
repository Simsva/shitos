#include <syscall.h>
#include <syscall_nums.h>

DEFN_SYSCALL0(test0, SYS_test0);
DEFN_SYSCALL1(test1, SYS_test1, int);
DEFN_SYSCALL2(test2, SYS_test2, int, int);
DEFN_SYSCALL3(test3, SYS_test3, int, int, int);
DEFN_SYSCALL4(test4, SYS_test4, int, int, int, int);
DEFN_SYSCALL5(test5, SYS_test5, int, int, int, int, int);
DEFN_SYSCALL6(test6, SYS_test6, int, int, int, int, int, int);
