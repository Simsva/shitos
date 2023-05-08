#ifndef UNISTD_H_
#define UNISTD_H_

#include <_cheader.h>
#include <sys/types.h>
#include <stddef.h>

_BEGIN_C_HEADER

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

char *getcwd(char *, size_t);
int chdir(const char *);

/* FIXME: no */
typedef int intptr_t;

int execv(const char *, char *const[]);
int execve(const char *, char *const[], char *const[]);
int execvp(const char *, char *const[]);

pid_t fork(void);
pid_t getpid(void);

_END_C_HEADER

#endif // UNISTD_H_
