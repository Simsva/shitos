#ifndef SYS_TYPES_H_
#define SYS_TYPES_H_

/* TODO: fix type definitions */

#include <features.h>

/* TODO: move this */
#define __BITS 32

#if __BITS == 32
typedef int ssize_t;
#else
typedef long int ssize_t;
#endif

#endif // SYS_TYPES_H_
