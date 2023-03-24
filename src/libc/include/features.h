#ifndef FEATURES_H_
#define FEATURES_H_

#if __STDC_VERSION__ >= 199901L
#define __restrict restrict
#endif

#if __STDC_VERSION__ >= 199901L || defined(__cplusplus)
#define __inline inline
#endif

#if __STDC_VERSION__ >= 201112L
#elif defined(__GNUC__)
#define _Noreturn __attribute__((__noreturn__))
#else
#define _Noreturn
#endif

#endif // FEATURES_H_
