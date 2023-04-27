#ifndef UNISTD_H_
#define UNISTD_H_

#include <_cheader.h>
#include <sys/types.h>

_BEGIN_C_HEADER

/* FIXME: no */
typedef int intptr_t;

int execv(const char *, char *const[]);
int execve(const char *, char *const[], char *const[]);
int execvp(const char *, char *const[]);

pid_t fork(void);
pid_t getpid(void);

_END_C_HEADER

#endif // UNISTD_H_
