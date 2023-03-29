#ifndef KERNEL_UTILS_H_
#define KERNEL_UTILS_H_


#ifndef asm
# define asm __asm__
#endif
#ifndef volatile
# define volatile __volatile__
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif // KERNEL_UTILS_H_
