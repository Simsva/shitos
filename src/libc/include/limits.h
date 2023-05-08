#ifndef LIMITS_H_
#define LIMITS_H_

/* support default unsigned char */
#if '\xff' > 0
#define CHAR_MIN   (0)
#define CHAR_MAX   (255)
#else
#define CHAR_MIN   (-1-CHAR_MAX)
#define CHAR_MAX   (0x7f)
#endif
#define CHAR_BIT   (8)
#define SCHAR_MIN  (-1-SCHAR_MAX)
#define SCHAR_MAX  (0x7f)
#define UCHAR_MAX  (0xff)

#define SHRT_MIN   (-1-SHRT_MAX)
#define SHRT_MAX   (0x7fff)
#define USHRT_MAX  (0xffff)

#define INT_MIN    (-1-INT_MAX)
#define INT_MAX    (0x7fffffff)
#define UINT_MAX   (0xffffffffU)

#define LONG_BIT   32
#define LONG_MIN   (-1-LONG_MAX)
#if LONG_BIT == 32
#define LONG_MAX   (0x7fffffffL)
#define ULONG_MAX  (0xffffffffUL)
#else
#define LONG_MAX   (0x7fffffffffffffffL)
#define ULONG_MAX  (0xffffffffffffffffUL)
#endif

#define LLONG_MIN  (-1-LLONG_MAX)
#define LLONG_MAX  (0x7fffffffffffffffLL)
#define ULLONG_MAX (0xffffffffffffffffULL)

#define MB_LEN_MAX (1)

#define NL_ARGMAX  (9)

#define PATH_MAX   4096

#endif // LIMITS_H_
