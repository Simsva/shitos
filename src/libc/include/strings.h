#ifndef STRINGS_H_
#define STRINGS_H_

#include <_cheader.h>
#include <features.h>

_BEGIN_C_HEADER

typedef __typeof__(sizeof 0) size_t;

int ffs(int);
int ffsl(long);
int ffsll(long long);

_END_C_HEADER

#endif // STRINGS_H_
