#ifndef STDIO_H_
#define STDIO_H_

#include <_cheader.h>
#include <features.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stddef.h>

_BEGIN_C_HEADER

#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

#undef  EOF
#define EOF (-1)

struct __FILE;

typedef struct __FILE {
    unsigned flags;
    int (*close)(struct __FILE *);
    size_t (*read)(struct __FILE *, unsigned char *, size_t);
    size_t (*write)(struct __FILE *, const unsigned char *, size_t);
    off_t (*seek)(struct __FILE *, off_t, int);
    unsigned char *buf;
    size_t buf_size;
    int fd;
    int mode;
    off_t off;
} FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

int putc(int, FILE *);
int putchar(int);

int puts(const char *);

int printf(const char *__restrict, ...) __attribute__((format(printf, 1, 2)));
int fprintf(FILE *__restrict, const char *__restrict, ...) __attribute__((format(printf, 2, 3)));
int sprintf(char *__restrict, const char *__restrict, ...) __attribute__((format(printf, 2, 3)));
int snprintf(char *__restrict, size_t, const char *__restrict, ...) __attribute__((format(printf, 3, 4)));

int vprintf(const char *__restrict, va_list);
int vfprintf(FILE *__restrict, const char *__restrict, va_list);
int vsprintf(char *__restrict, const char *__restrict, va_list);
int vsnprintf(char *__restrict, size_t, const char *__restrict, va_list);

_END_C_HEADER

#endif // STDIO_H_
