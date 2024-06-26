#ifndef BOOT_OPTS_H_
#define BOOT_OPTS_H_

#include <stdint.h>

/* boot options */
#define HAS_OPT(x)   (boot_options&x)
#define BOOT_OPTS_DEFAULT 0x00
#include <boot/boot_opts.h>

extern uint8_t boot_options;

#endif // BOOT_OPTS_H_
