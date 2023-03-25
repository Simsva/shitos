#ifndef STDLIB_H_
#define STDLIB_H_

#include <_cheader.h>
#include <features.h>

_BEGIN_C_HEADER

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

_END_C_HEADER

#endif // STDLIB_H_
