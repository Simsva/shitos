#ifndef CTYPE_H_
#define CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <features.h>

/* TODO: implement as function */
#define isdigit(c) (((unsigned)(c)-'0') < 10)

#ifdef __cplusplus
}
#endif

#endif // CTYPE_H_
