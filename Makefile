CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld

CFLAGS=-m32 -std=c99 -O2 -g -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
# Maybe bad for optimizations?
CFLAGS+=-fno-builtin-function -fno-builtin
ASFLAGS=-f elf32 -w+orphan-labels
LDFLAGS=

BOOTSECT=boot.bin
BOOTSECT_SRC=src/boot.s
BOOTSECT_OBJ=$(BOOTSECT_SRC:.s=.o)

KERNEL=kernel.bin
KERNEL_SRC_C=$(wildcard src/kernel/*.c)
KERNEL_SRC_S=$(wildcard src/kernel/*.s)
KERNEL_OBJ=$(KERNEL_SRC_C:.c=.o) $(KERNEL_SRC_S:.s=.o)

ISO=shitos.iso

all: dirs bootsect kernel

clean:
	rm -f $(BOOTSECT_OBJ) $(KERNEL_OBJ) $(ISO)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.S
	$(AS) -o $@ $< $(ASFLAGS)

dirs:
	mkdir -p bin

bootsect: $(BOOTSECT_OBJ)
	$(LD) -o ./bin/$(BOOTSECT) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(BOOTSECT:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00

kernel: $(KERNEL_OBJ)
	$(LD) -o ./bin/$(KERNEL) $^ $(LDFLAGS) -Tsrc/link.ld

# used for debugging
	$(LD) -o ./bin/$(KERNEL:.bin=.elf) $^ $(LDFLAGS) -Tsrc/link.ld --oformat=elf32-i386

iso: dirs bootsect kernel
	dd if=/dev/zero of=boot.iso bs=512 count=2880
	dd if=./bin/$(BOOTSECT) of=boot.iso conv=notrunc bs=512 seek=0 count=1
	dd if=./bin/$(KERNEL) of=boot.iso conv=notrunc bs=512 seek=1 count=2048
