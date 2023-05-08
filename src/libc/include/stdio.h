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

#define BUFSIZ 1024

#define _IONBF 2
#define _IOLBF 1
#define _IOFBF 0

#define SEEK_SET 0  /* seek from beginning of file */
#define SEEK_CUR 1  /* seek from current position */
#define SEEK_END 2  /* seek from end of file */

struct __FILE;

typedef struct __FILE {
    int (*close)(struct __FILE *);
    size_t (*read)(struct __FILE *, unsigned char *, size_t);
    size_t (*write)(struct __FILE *, const unsigned char *, size_t);
    int (*seek)(struct __FILE *, off_t, int);

    unsigned char *buf;
    size_t buf_i, buf_sz;
    int buf_mode;

    int fd;

    off_t off;
    unsigned flags;
    int ungetc;

    struct __FILE *prev, *next;
} FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

#define stdin  (stdin)
#define stdout (stdout)
#define stderr (stderr)

int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);

int fputs(const char *__restrict, FILE *__restrict);
int puts(const char *);

int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int ungetc(int, FILE *);

char *fgets(char *__restrict, int, FILE *__restrict);
char *gets(char *);

FILE *fopen(const char *__restrict, const char *__restrict);
int fclose(FILE *);
size_t fread(void *__restrict, size_t, size_t, FILE *__restrict);
size_t fwrite(const void *__restrict, size_t, size_t, FILE *__restrict);
int fflush(FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
void rewind(FILE *);

void clearerr(FILE *);
int feof(FILE *);
int ferror(FILE *);

int rename(const char *, const char*);
int remove(const char *);

int printf(const char *__restrict, ...) __attribute__((format(printf, 1, 2)));
int fprintf(FILE *__restrict, const char *__restrict, ...) __attribute__((format(printf, 2, 3)));
int sprintf(char *__restrict, const char *__restrict, ...) __attribute__((format(printf, 2, 3)));
int snprintf(char *__restrict, size_t, const char *__restrict, ...) __attribute__((format(printf, 3, 4)));

int vprintf(const char *__restrict, va_list);
int vfprintf(FILE *__restrict, const char *__restrict, va_list);
int vsprintf(char *__restrict, const char *__restrict, va_list);
int vsnprintf(char *__restrict, size_t, const char *__restrict, va_list);

/* TODO: implement vfscanf */
int scanf(const char *__restrict, ...);
int fscanf(FILE *__restrict, const char *__restrict, ...);
int sscanf(const char *__restrict, const char *__restrict, ...);
int vscanf(const char *__restrict, va_list);
int vfscanf(FILE *__restrict, const char *__restrict, va_list);
int vsscanf(const char *__restrict, const char *__restrict, va_list);

int setvbuf(FILE *__restrict, char *__restrict, int, size_t);
void setbuf(FILE *__restrict, char *__restrict);

#ifndef __is_libk
void perror(const char *);
#endif

_END_C_HEADER

#endif // STDIO_H_
