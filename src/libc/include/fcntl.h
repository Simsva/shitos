#ifndef FCNTL_H_
#define FCNTL_H_

#include <_cheader.h>

_BEGIN_C_HEADER

#define O_ACCMODE   07
#define O_RDONLY    00
#define O_WRONLY    01
#define O_RDWR      02
#define O_PATH      03

#define O_APPEND    000010
#define O_CREAT     001000
#define O_TRUNC     002000
#define O_EXCL      004000
#define O_NOFOLLOW  010000
#define O_NONBLOCK  020000
#define O_DIRECTORY 040000

#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4

#define F_GETLK  5
#define F_SETLK  6
#define F_SETLKW 7

#define F_RDLCK  0
#define F_WRLCK  1
#define F_UNLCK  2

_END_C_HEADER

#endif // FCNTL_H_
