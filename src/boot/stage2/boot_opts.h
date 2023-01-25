#ifndef BOOT_OPTS_H_
#define BOOT_OPTS_H_

#include <sys/stdint.h>

/* boot options */
#define HAS_OPT(x)   (boot_options&x)
#define OPTS_DEFAULT 0x00
#define OPT_VERBOSE  0x01

extern uint8_t boot_options;

#endif // BOOT_OPTS_H_
