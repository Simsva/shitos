#ifndef STDARG_H_
#define STDARG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VA_LIST_DECLARED
# define _VA_LIST_DECLARED
typedef __builtin_va_list va_list;
#endif

#define va_start(ap, last)   __builtin_va_start((ap), (last))
#define va_arg(ap, type)     __builtin_va_arg((ap), type)
#define va_copy(dest, src)   __builtin_va_copy((dest), (src))
#define va_end(ap)           __builtin_va_end(ap)

#ifdef __cplusplus
}
#endif

#endif // STDARG_H_
