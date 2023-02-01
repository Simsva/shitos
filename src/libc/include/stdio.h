#ifndef STDIO_H_
#define STDIO_H_

#include <features.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#undef  EOF
#define EOF (-1)

int putchar(int);

int puts(const char *);

int printf(const char *__restrict, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *__restrict, va_list);

#ifdef __cplusplus
}
#endif

#endif // STDIO_H_
