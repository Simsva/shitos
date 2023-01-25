CFLAGS=-m32 -std=c99 -O2 -Wall -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
CFLAGS+=-fno-builtin-function -fno-builtin

ASFLAGS=-f elf32 -w+orphan-labels

LDFLAGS=

# NOTE: unused
MAKEOPTS=
