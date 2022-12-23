CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld

CFLAGS=-m32 -std=c99 -O2 -g -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
# Maybe bad for optimizations?
CFLAGS+=-fno-builtin-function -fno-builtin
ASFLAGS=-f elf32 -w+orphan-labels
LDFLAGS=

# Source
MBR=mbr.bin
MBR_SRC=src/boot/mbr.asm
MBR_OBJ=$(MBR_SRC:.asm=.o)

STAGE1=stage1.bin
STAGE1_SRC=src/boot/stage1.asm
STAGE1_OBJ=$(STAGE1_SRC:.asm=.o)

# NOTE: old, do not use
STAGE2=stage2.bin
STAGE2_SRC_ASM=$(wildcard src/boot/stage2/*.asm)
STAGE2_SRC_C=$(wildcard   src/boot/stage2/*.c)
STAGE2_OBJ=$(STAGE2_SRC_ASM:.asm=.o) $(STAGE2_SRC_C:.c=.o)

KERNEL=kernel.bin
KERNEL_SRC_C=$(wildcard src/kernel/*.c)
KERNEL_SRC_ASM=$(wildcard src/kernel/*.asm)
KERNEL_OBJ=$(KERNEL_SRC_C:.c=.o) $(KERNEL_SRC_ASM:.asm=.o)

STRUCTS=structs.o
STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

ISO=shitos.iso


all: dirs mbr stage1 stage2 structs


clean:
	rm -f ./bin/* $(MBR_OBJ) $(STAGE1_OBJ) $(STAGE2_OBJ) $(STRUCTS_OBJ) \
		$(ISO)


%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)


%.o: %.asm
	$(AS) -o $@ $< $(ASFLAGS)


dirs:
	mkdir -p bin


mbr: $(MBR_OBJ)
	$(LD) -o ./bin/$(MBR) $^ $(LDFLAGS) -Ttext 0x0 --oformat=binary
# used for debugging
	$(LD) -o ./bin/$(MBR:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x600


stage1: $(STAGE1_OBJ)
	$(LD) -o ./bin/$(STAGE1) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary
# used for debugging
	$(LD) -o ./bin/$(STAGE1:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00


stage2: $(STAGE2_OBJ)
	$(LD) -o ./bin/$(STAGE2) $^ $(LDFLAGS) -Tsrc/boot/stage2/link.ld
# used for debugging
	$(LD) -o ./bin/$(STAGE2:.bin=.elf) $^ $(LDFLAGS) \
		-Tsrc/boot/stage2/link.ld --oformat=elf32-i386


# NOTE: old, do not use
kernel: $(KERNEL_OBJ)
	$(LD) -o ./bin/$(KERNEL) $^ $(LDFLAGS) -Tsrc/link.ld
# used for debugging
	$(LD) -o ./bin/$(KERNEL:.bin=.elf) $^ $(LDFLAGS) -Tsrc/link.ld --oformat=elf32-i386


structs: dirs
	$(CC) -o $(STRUCTS_OBJ) -c $(STRUCTS_SRC) -O0 -g


iso: all
# create "bootloader partition"
	cat bin/$(STAGE1) bin/$(STAGE2) > bootpart.bin

# create fat partition
	dd if=/dev/zero of=fatpart.bin bs=512 count=128k
	mkfs.vfat -F32 fatpart.bin
# TODO: mount fs and do things

# partition and combine
	./partition.sh -vfm "bin/$(MBR)" "$(ISO)" \
		"bootpart.bin:13::y" "fatpart.bin:0b"

	rm fatpart.bin bootpart.bin
