CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld

CFLAGS=-m32 -std=c99 -O2 -g -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
# Maybe bad for optimizations?
CFLAGS+=-fno-builtin-function -fno-builtin
ASFLAGS=-f elf32 -w+orphan-labels
LDFLAGS=

# Old, do not use
BOOTSECT=boot.bin
BOOTSECT_SRC=src/boot.s
BOOTSECT_OBJ=$(BOOTSECT_SRC:.s=.o)

MBR=mbr.bin
MBR_SRC=src/boot/mbr.asm
MBR_OBJ=$(MBR_SRC:.asm=.o)

STAGE1=stage1.bin
STAGE1_SRC=src/boot/stage1.asm
STAGE1_OBJ=$(STAGE1_SRC:.asm=.o)

STAGE2=stage2.bin
STAGE2_SRC=src/boot/stage2.asm
STAGE2_OBJ=$(STAGE2_SRC:.asm=.o)

KERNEL=kernel.bin
KERNEL_SRC_C=$(wildcard src/kernel/*.c)
KERNEL_SRC_S=$(wildcard src/kernel/*.s)
KERNEL_OBJ=$(KERNEL_SRC_C:.c=.o) $(KERNEL_SRC_S:.s=.o)

STRUCTS=structs.o
STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

ISO=shitos.iso

all: dirs mbr stage1 stage2 structs

clean:
	rm -f $(BOOTSECT_OBJ) $(STAGE1_OBJ) $(STAGE2_OBJ) $(STRUCTS_OBJ) $(KERNEL_OBJ) $(ISO)

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

mbr: $(MBR_OBJ)
	$(LD) -o ./bin/$(MBR) $^ $(LDFLAGS) -Ttext 0x0 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(MBR:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x600

stage1: $(STAGE1_OBJ)
	$(LD) -o ./bin/$(STAGE1) $^ $(LDFLAGS) -Ttext 0x0 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(STAGE1:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00

stage2: $(STAGE2_OBJ)
	$(LD) -o ./bin/$(STAGE2) $^ $(LDFLAGS) -Ttext 0x0000 --oformat=binary

# used for debugging
	$(LD) -o ./bin/$(STAGE2:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00

kernel: $(KERNEL_OBJ)
	$(LD) -o ./bin/$(KERNEL) $^ $(LDFLAGS) -Tsrc/link.ld

# used for debugging
	$(LD) -o ./bin/$(KERNEL:.bin=.elf) $^ $(LDFLAGS) -Tsrc/link.ld --oformat=elf32-i386

structs: dirs
	$(CC) -o $(STRUCTS_OBJ) -c $(STRUCTS_SRC) -O0 -g

iso_old: dirs bootsect_old kernel
	dd if=/dev/zero of=$(ISO) bs=512 count=2880
	dd if=bin/$(BOOTSECT) of=$(ISO) conv=notrunc bs=512 seek=0 count=1
	dd if=bin/$(KERNEL) of=$(ISO) conv=notrunc bs=512 seek=1 count=2048

iso: all
	./partition.sh $(ISO) 64K

	dd if=/dev/zero of=fatpart.iso bs=512 count=64k
	mkfs.vfat -F32 fatpart.iso
# TODO: mount fs and do things

# combine
	dd if=bin/$(MBR) of=$(ISO) conv=notrunc bs=446 seek=0 count=1
	dd if=fatpart.iso of=$(ISO) conv=notrunc bs=512 seek=2048 count=64k
	dd if=bin/$(STAGE1) of=$(ISO) conv=notrunc bs=512 seek=2048 count=1

	rm fatpart.iso
