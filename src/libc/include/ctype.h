#ifndef CTYPE_H_
#define CTYPE_H_

#include <_cheader.h>
#include <features.h>

_BEGIN_C_HEADER

/* TODO: implement as function */
#define isdigit(c) (((unsigned)(c)-'0') < 10)

_END_C_HEADER

#endif // CTYPE_H_
