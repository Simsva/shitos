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
BOOTSECT_SRC=src/boot/stage1.asm
BOOTSECT_OBJ=$(BOOTSECT_SRC:.asm=.o)
BOOTSECT_SRC_OLD=src/boot.s
BOOTSECT_OBJ_OLD=$(BOOTSECT_SRC_OLD:.s=.o)

KERNEL=kernel.bin
KERNEL_SRC_C=$(wildcard src/kernel/*.c)
KERNEL_SRC_S=$(wildcard src/kernel/*.s)
KERNEL_OBJ=$(KERNEL_SRC_C:.c=.o) $(KERNEL_SRC_S:.s=.o)

ISO=shitos.iso

all: dirs bootsect kernel

clean:
	rm -f $(BOOTSECT_OBJ) $(BOOTSECT_OBJ_OLD) $(KERNEL_OBJ) $(ISO)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.S
	$(AS) -o $@ $< $(ASFLAGS)

%.o: %.asm
	$(AS) -o $@ $< $(ASFLAGS)

dirs:
	mkdir -p bin

bootsect_old: $(BOOTSECT_OBJ_OLD)
	$(LD) -o ./bin/$(BOOTSECT) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(BOOTSECT:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00

bootsect: $(BOOTSECT_OBJ)
	$(LD) -o ./bin/$(BOOTSECT) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(BOOTSECT:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00

kernel: $(KERNEL_OBJ)
	$(LD) -o ./bin/$(KERNEL) $^ $(LDFLAGS) -Tsrc/link.ld

# used for debugging
	$(LD) -o ./bin/$(KERNEL:.bin=.elf) $^ $(LDFLAGS) -Tsrc/link.ld --oformat=elf32-i386

iso_old: dirs bootsect_old kernel
	dd if=/dev/zero of=$(ISO) bs=512 count=2880
	dd if=bin/$(BOOTSECT) of=$(ISO) conv=notrunc bs=512 seek=0 count=1
	dd if=bin/$(KERNEL) of=$(ISO) conv=notrunc bs=512 seek=1 count=2048

iso: dirs bootsect
	./partition.sh $(ISO) 64K

	dd if=/dev/zero of=fatpart.iso bs=512 count=64k
	mkfs.vfat -F32 fatpart.iso
# TODO: mount fs and do things

# combine
	dd if=bin/$(BOOTSECT) of=$(ISO) conv=notrunc bs=466 seek=0 count=1
	dd if=fatpart.iso of=$(ISO) conv=notrunc bs=512 seek=2048 count=64k

	rm fatpart.iso
