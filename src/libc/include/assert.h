#ifndef ASSERT_H_
#define ASSERT_H_

#include <_cheader.h>
#include <features.h>

#undef assert

#ifdef NDEBUG
#define assert(x) (void)0
#else
#define assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__),0)))
#endif

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#define static_assert _Static_assert
#endif

_BEGIN_C_HEADER

_Noreturn void __assert_fail (const char *, const char *, int, const char *);

_END_C_HEADER

#endif // ASSERT_H_
