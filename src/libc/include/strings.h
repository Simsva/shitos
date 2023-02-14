#ifndef STRINGS_H_
#define STRINGS_H_

#include <features.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __typeof__(sizeof 0) size_t;

int ffs(int);
int ffsl(long);
int ffsll(long long);

#ifdef __cplusplus
}
#endif

#endif // STRINGS_H_
