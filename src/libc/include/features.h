#ifndef FEATURES_H_
#define FEATURES_H_

#if __STDC_VERSION__ >= 199901L
#define __restrict restrict
#endif

#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
#define __inline inline
#endif

#ifndef asm
# define asm __asm__
#endif

#ifndef volatile
# define volatile __volatile__
#endif

#if __STDC_VERSION__ >= 201112L
#elif defined(__GNUC__)
#define _Noreturn __attribute__((__noreturn__))
#else
#define _Noreturn
#endif

#define __weak __attribute__((__weak__))
#define __hidden __attribute__((__visibility__("hidden")))
#define weak_alias(old, new) \
    extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

#ifdef __GNUC__
#define __unused __attribute__((unused))
#else
#define __unused
#endif

#ifdef __GNUC__
#define __fallthrough __attribute__((fallthrough))
#else
#define __fallthrough
#endif

#endif // FEATURES_H_
