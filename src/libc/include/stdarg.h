#ifndef STDARG_H_
#define STDARG_H_

#include <_cheader.h>

_BEGIN_C_HEADER

#ifndef _VA_LIST_DECLARED
# define _VA_LIST_DECLARED
typedef __builtin_va_list va_list;
#endif

#define va_start(ap, last)   __builtin_va_start((ap), (last))
#define va_arg(ap, type)     __builtin_va_arg((ap), type)
#define va_copy(dest, src)   __builtin_va_copy((dest), (src))
#define va_end(ap)           __builtin_va_end(ap)

_END_C_HEADER

#endif // STDARG_H_
