#ifndef SYSCALL_H_
#define SYSCALL_H_

#include <_cheader.h>

_BEGIN_C_HEADER

#define DECL_SYSCALL0(fn)                   long syscall_##fn(void)
#define DECL_SYSCALL1(fn, A)                long syscall_##fn(A)
#define DECL_SYSCALL2(fn, A, B)             long syscall_##fn(A, B)
#define DECL_SYSCALL3(fn, A, B, C)          long syscall_##fn(A, B, C)
#define DECL_SYSCALL4(fn, A, B, C, D)       long syscall_##fn(A, B, C, D)
#define DECL_SYSCALL5(fn, A, B, C, D, E)    long syscall_##fn(A, B, C, D, E)
#define DECL_SYSCALL6(fn, A, B, C, D, E, F) long syscall_##fn(A, B, C, D, E, F)

/* calling convention:
 * eax - num/return
 * ebx, ecx, edx, esi, edi, ebp - arguments 1 to 6 */
#define DEFN_SYSCALL0(fn, num) \
    long syscall_##fn() { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res)); \
        return res; \
    }

#define DEFN_SYSCALL1(fn, num, A) \
    long syscall_##fn(A a) { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res), "b"((long)a)); \
        return res; \
    }

#define DEFN_SYSCALL2(fn, num, A, B) \
    long syscall_##fn(A a, B b) { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res), "b"((long)a), "c"((long)b)); \
        return res; \
    }

#define DEFN_SYSCALL3(fn, num, A, B, C) \
    long syscall_##fn(A a, B b, C c) { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res), "b"((long)a), "c"((long)b), "d"((long)c)); \
        return res; \
    }

#define DEFN_SYSCALL4(fn, num, A, B, C, D) \
    long syscall_##fn(A a, B b, C c, D d) { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res), "b"((long)a), "c"((long)b), "d"((long)c), \
                        "S"((long)d)); \
        return res; \
    }

#define DEFN_SYSCALL5(fn, num, A, B, C, D, E) \
    long syscall_##fn(A a, B b, C c, D d, E e) { \
        long res = num; \
        __asm__ __volatile__("int $0x30" \
            : "=a"(res) \
            : "a"(res), "b"((long)a), "c"((long)b), "d"((long)c), \
                        "S"((long)d), "D"((long)e)); \
        return res; \
    }

#define DEFN_SYSCALL6(fn, num, A, B, C, D, E, F) \
    long syscall_##fn(A a, B b, C c, D d, E e, F f) { \
        long res = num; \
        __asm__ __volatile__( \
            "push %%ebp\n" \
            "mov %7, %%ebp\n" \
            "int $0x30\n" \
            "pop %%ebp" \
            : "=a"(res) \
            : "a"(res), "b"((long)a), "c"((long)b), "d"((long)c), \
                        "S"((long)d), "D"((long)e), "m"((long)f)); \
        return res; \
    }

DECL_SYSCALL0(test0);
DECL_SYSCALL1(test1, int);
DECL_SYSCALL2(test2, int, int);
DECL_SYSCALL3(test3, int, int, int);
DECL_SYSCALL4(test4, int, int, int, int);
DECL_SYSCALL5(test5, int, int, int, int, int);
DECL_SYSCALL6(test6, int, int, int, int, int, int);

_END_C_HEADER

#endif // SYSCALL_H_
