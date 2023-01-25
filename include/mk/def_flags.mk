INCS=-I$(ROOT)/include
LIBS=

CFLAGS=-m32 -std=c99 -O2 -Wall -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
CFLAGS+=-fno-builtin-function -fno-builtin
CFLAGS+=$(INCS)

ASFLAGS=-f elf32 -w+orphan-labels

LDFLAGS=$(LIBS)

# NOTE: unused
MAKEOPTS=
