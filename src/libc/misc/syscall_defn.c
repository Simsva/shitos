#include <syscall.h>
#include <syscall_nums.h>
#include <dirent.h>

DEFN_SYSCALL1(exit,       SYS_exit,       int);
DEFN_SYSCALL3(open,       SYS_open,       const char *, unsigned, unsigned);
DEFN_SYSCALL1(close,      SYS_close,      int);
DEFN_SYSCALL3(read,       SYS_read,       int, char *, size_t);
DEFN_SYSCALL3(write,      SYS_write,      int, const char *, size_t);
DEFN_SYSCALL3(seek,       SYS_seek,       int, long, int);
DEFN_SYSCALL2(sysfunc,    SYS_sysfunc,    long, void *);
DEFN_SYSCALL2(mknod,      SYS_mknod,      const char *, mode_t);
DEFN_SYSCALL1(unlink,     SYS_unlink,     const char *);
DEFN_SYSCALL3(ioctl,      SYS_ioctl,      int, long, void *);
DEFN_SYSCALL2(getcwd,     SYS_getcwd,     char *, size_t);
DEFN_SYSCALL1(chdir,      SYS_chdir,      const char *);
DEFN_SYSCALL3(readdir,    SYS_readdir,    int, long, struct dirent *);
