#ifndef STDDEF_H_
#define STDDEF_H_

#if __cplusplus >= 201103L
#define NULL nullptr
#elif defined(__cplusplus)
#define NULL 0L
#else
#define NULL ((void*)0)
#endif

typedef __typeof__(sizeof 0)              size_t;
typedef __typeof__((void *)1 - (void *)0) ptrdiff_t;

#if __GNUC__ > 3
# define offsetof(type, member) __builtin_offsetof(type, member)
#else
# define offsetof(type, member) ((size_t)( (char *)&(((type *)0)->member) - (char *)0 ))
#endif

#endif // STDDEF_H_
