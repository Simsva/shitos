#ifndef STDINT_H_
#define STDINT_H_

/* TODO: move this */
#define __BITS 32

/* fixed size int types */
typedef char               int8_t;
typedef short              int16_t;
typedef int                int32_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

#if __BITS == 32
typedef long long          int64_t;
typedef unsigned long long uint64_t;
#else
typedef long               int64_t;
typedef unsigned long      uint64_t;
#endif

typedef int8_t             int_fast8_t;
typedef int16_t            int_fast16_t;
typedef int32_t            int_fast32_t;
typedef int64_t            int_fast64_t;

typedef int8_t             int_least8_t;
typedef int16_t            int_least16_t;
typedef int32_t            int_least32_t;
typedef int64_t            int_least64_t;

typedef uint8_t            uint_fast8_t;
typedef uint16_t           uint_fast16_t;
typedef uint32_t           uint_fast32_t;
typedef uint64_t           uint_fast64_t;

typedef uint8_t            uint_least8_t;
typedef uint16_t           uint_least16_t;
typedef uint32_t           uint_least32_t;
typedef uint64_t           uint_least64_t;

/* additional int types */
#if __BITS == 32
typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;
#else
typedef int64_t            intptr_t;
typedef uint64_t           uintptr_t;
#endif
typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;

/* fixed size int type sizes */
#define INT8_WIDTH     (8)
#define INT16_WIDTH    (16)
#define INT32_WIDTH    (32)
#define INT64_WIDTH    (64)

#define UINT8_WIDTH    (8)
#define UINT16_WIDTH   (16)
#define UINT32_WIDTH   (32)
#define UINT64_WIDTH   (64)

#define INT8_MIN       (-1-INT8_MAX)
#define INT16_MIN      (-1-INT16_MAX)
#define INT32_MIN      (-1-INT32_MAX)
#define INT64_MIN      (-1-INT64_MAX)

#define INT8_MAX       (INT8_C(0x7f))
#define INT16_MAX      (INT16_C(0x7fff))
#define INT32_MAX      (INT32_C(0x7fffffff))
#define INT64_MAX      (INT64_C(0x7fffffffffffffff))

#define UINT8_MAX      (UINT8_C(0xff))
#define UINT16_MAX     (UINT16_C(0xffff))
#define UINT32_MAX     (UINT32_C(0xffffffff))
#define UINT64_MAX     (UINT64_C(0xffffffffffffffff))

/* additional int types sizes */
#if __BITS == 32
# define INTPTR_WIDTH  INT32_WIDTH
# define UINTPTR_WIDTH UINT32_WIDTH
# define INTPTR_MIN    INT32_MIN
# define INTPTR_MAX    INT32_MAX
# define UINTPTR_MAX   UINT32_MAX
#else
# define INTPTR_WIDTH  INT64_WIDTH
# define UINTPTR_WIDTH UINT64_WIDTH
# define INTPTR_MIN    INT64_MIN
# define INTPTR_MAX    INT64_MAX
# define UINTPTR_MAX   UINT64_MAX
#endif

#define INTMAX_WIDTH  INT64_WIDTH
#define UINTMAX_WIDTH UINT64_WIDTH
#define INTMAX_MIN    INT64_MIN
#define INTMAX_MAX    INT64_MAX
#define UINTMAX_MAX   UINT64_MAX

/* constant macros */
#define INT8_C(c)     c
#define INT16_C(c)    c
#define INT32_C(c)    c

#define UINT8_C(c)    c
#define UINT16_C(c)   c
#define UINT32_C(c)   c ## U

#if __BITS == 32
# define INT64_C(c)   c ## LL
# define UINT64_C(c)  c ## ULL
# define INTMAX_C(c)  c ## LL
# define UINTMAX_C(c) c ## ULL
#else
# define INT64_C(c)   c ## L
# define UINT64_C(c)  c ## UL
# define INTMAX_C(c)  c ## L
# define UINTMAX_C(c) c ## UL
#endif

#endif // STDINT_H_
