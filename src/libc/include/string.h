#ifndef STRING_H_
#define STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <features.h>

#include <stddef.h>
#include <stdint.h>

void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memchr(const void *, int, size_t);

int strcmp(const char *, const char *);

size_t strnlen(const char *, size_t);

#ifdef __cplusplus
}
#endif

#endif // STRING_H_
