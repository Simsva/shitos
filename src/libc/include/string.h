#ifndef STRING_H_
#define STRING_H_

#include <_cheader.h>
#include <features.h>
#include <stddef.h>
#include <stdint.h>

_BEGIN_C_HEADER

void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memchr(const void *, int, size_t);
void *memrchr(const void *, int, size_t);
void *memmem(const void *, size_t, const void *, size_t);

char *stpcpy(char *__restrict, const char *__restrict);
char *stpncpy(char *__restrict, const char *__restrict, size_t);
char *strcpy(char *__restrict, const char *__restrict);
char *strncpy(char *__restrict, const char *__restrict, size_t);

int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);

char *strdup(const char *);

size_t strlen(const char *);
size_t strnlen(const char *, size_t);

char *strcat(char *__restrict, const char *__restrict);

char *strchr(const char *, int);
char *strrchr(const char *, int);

size_t strcspn(const char *, const char *);
size_t strspn(const char *, const char *);
char *strstr(const char *, const char *);
char *strtok(char *__restrict, const char *__restrict);
char *strtok_r(char *__restrict, const char *__restrict, char **__restrict);

char *strchrnul(const char *, int);

#ifndef __is_libk
char *strerror(int);
#endif

_END_C_HEADER

#endif // STRING_H_
