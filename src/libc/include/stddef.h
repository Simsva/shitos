#ifndef STDDEF_H_
#define STDDEF_H_

#define NULL ((void *)0)

typedef __typeof__(sizeof 0)              size_t;
typedef __typeof__((void *)1 - (void *)0) ptrdiff_t;

#endif // STDDEF_H_
