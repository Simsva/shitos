#ifndef STDLIB_H_
#define STDLIB_H_

#include <features.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __cplusplus >= 201103L
# define NULL nullptr
#elif defined(__cplusplus)
# define NULL 0L
#else
# define NULL ((void*)0)
#endif

typedef __typeof__(sizeof 0) size_t;

typedef struct { int quot, rem; }       div_t;
typedef struct { long quot, rem; }      ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;

div_t div(int, int);
ldiv_t ldiv(long, long);
lldiv_t lldiv(long long, long long);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#ifdef __cplusplus
}
#endif

#endif // STDLIB_H_
