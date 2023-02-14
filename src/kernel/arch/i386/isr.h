#ifndef I386_ISR_H_
#define I386_ISR_H_

#define FAULT_ZERO_DIV          0x0
#define FAULT_DEBUG             0x1
#define FAULT_NMI               0x2
#define FAULT_BREAK             0x3
#define FAULT_OVERFLOW          0x4
#define FAULT_OOB               0x5
#define FAULT_INVALID_OP        0x6
#define FAULT_NO_CO_CPU         0x7
#define FAULT_DOUBLE_FAULT      0x8
#define FAULT_CO_CPU_SEGMENT    0x9
#define FAULT_BAD_TSS           0xa
#define FAULT_INVALID_SEGMENT   0xb
#define FAULT_STACK             0xc
#define FAULT_GPF               0xd
#define FAULT_PAGE              0xe
#define FAULT_UNKNOWN_INTERRUPT 0xf
#define FAULT_CO_CPU            0x10
#define FAULT_ALIGNMENT_CHECK   0x11
#define FAULT_MACHINE_CHECK     0x12
#define FAULT_COUNT             0x20

void isrs_install(void);

#endif // I386_ISR_H_
