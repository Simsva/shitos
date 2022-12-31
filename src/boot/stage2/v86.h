#ifndef V86_H_
#define V86_H_

#include "stdint.h"

#define INT_V86   0x31

/* v86 control flags */
#define V86F_ADDR  0x10000  /* segment:offset address */
#define V86F_FLAGS 0x20000  /* return flags */

struct _v86 {
    uint32_t ctl;   /* control flags */
    uint32_t addr;  /* interrupt number or address */
    uint32_t es;    /* v86 es register */
    uint32_t ds;    /* v86 ds register */
    uint32_t fs;    /* v86 fs register */
    uint32_t gs;    /* v86 gs register */
    uint32_t eax;   /* v86 eax register */
    uint32_t ecx;   /* v86 ecx register */
    uint32_t edx;   /* v86 edx register */
    uint32_t ebx;   /* v86 ebx register */
    uint32_t efl;   /* v86 eflags register */
    uint32_t ebp;   /* v86 ebp register */
    uint32_t esi;   /* v86 esi register */
    uint32_t edi;   /* v86 edi register */
};

extern struct _v86 _v86;  /* v86 interface structure */
void _v86int(void);

#define v86 _v86
#define v86int _v86int

#endif // V86_H_
