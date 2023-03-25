#ifndef SYS_TYPES_H_
#define SYS_TYPES_H_

/* TODO: fix type definitions */

#include <features.h>
#include <limits.h>

/* TODO: move this */
#define __BITS 32

#if __BITS == 32
# define __SWORD_TYPE_T int
# define __UWORD_TYPE_T unsigned
#else
# define __SWORD_TYPE_T long
# define __UWORD_TYPE_T unsigned long
#endif

typedef __SWORD_TYPE_T ssize_t;
typedef __SWORD_TYPE_T off_t;
typedef __SWORD_TYPE_T time_t;
typedef __SWORD_TYPE_T clock_t;
typedef __UWORD_TYPE_T useconds_t;
typedef __SWORD_TYPE_T suseconds_t;

/* ids */
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef int id_t;
typedef unsigned dev_t;
typedef unsigned ino_t;
typedef unsigned mode_t;

#endif // SYS_TYPES_H_
