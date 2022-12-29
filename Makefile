CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld

# FIXME: fix compiler options
CFLAGS=-m32 -std=c99 -O2 -g -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
# maybe bad for optimizations?
CFLAGS+=-fno-builtin-function -fno-builtin
ASFLAGS=-f elf32 -w+orphan-labels
LDFLAGS=

# source files
MBR=bin/mbr.bin
MBR_SRC=src/boot/mbr.asm
MBR_OBJ=$(MBR_SRC:.asm=.o)

STAGE1=bin/stage1.bin
STAGE1_SRC=src/boot/stage1.asm
STAGE1_OBJ=$(STAGE1_SRC:.asm=.o)

STAGE2=bin/stage2.bin
STAGE2_SRC_ASM=$(wildcard src/boot/stage2/*.asm)
STAGE2_SRC_C=$(wildcard src/boot/stage2/*.c)
STAGE2_OBJ=$(STAGE2_SRC_ASM:.asm=.o) $(STAGE2_SRC_C:.c=.o)
STAGE2_LD=src/boot/stage2/link.ld

STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

# output files
DEBUG=$(MBR:.bin=.elf) $(STAGE1:.bin=.elf) $(STAGE2:.bin=.elf) $(STRUCTS_OBJ)
DIRS=bin

ISO=shitos.iso
BOOTPART=bin/bootpart.bin
FATPART=bin/fatpart.bin
FATPARTSZ=128k


all: bin $(MBR) $(STAGE1) $(STAGE2) $(BOOTPART) $(FATPART)


clean:
	@echo Clean
	rm -f ./bin/* $(MBR_OBJ) $(STAGE1_OBJ) $(STAGE2_OBJ) $(STRUCTS_OBJ) $(BOOTPART) $(FATPART) $(ISO)

%.o: %.c
	@echo "CC	$@"
	@$(CC) -o $@ -c $< $(CFLAGS)


%.o: %.asm
	@echo "AS	$@"
	@$(AS) -o $@ $< $(ASFLAGS)


# NOTE: some targetrs depend on `bin` but does not list it as a prerequisite,
# so make sure to run `make bin` once manually in case of issues
$(DIRS):
	@echo "MKDIR	$@"
	@mkdir -p $@


mbr: $(MBR) $(MBR:.bin=.elf)
$(MBR): $(MBR_OBJ)
	@echo "LD	$(MBR)"
	@$(LD) -o $(MBR) $^ $(LDFLAGS) -Ttext 0x0 --oformat=binary
# used for debugging
$(MBR:.bin=.elf): $(MBR_OBJ)
	@echo "LD	$(MBR:.bin=.elf)"
	@$(LD) -o $(MBR:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x600


stage1: $(STAGE1) $(STAGE1:.bin=.elf)
$(STAGE1): $(STAGE1_OBJ)
	@echo "LD	$(STAGE1)"
	@$(LD) -o $(STAGE1) $^ $(LDFLAGS) -Ttext 0x7c00 --oformat=binary
# used for debugging
$(STAGE1:.bin=.elf): $(STAGE1_OBJ)
	@echo "LD	$(STAGE1:.bin=.elf)"
	@$(LD) -o $(STAGE1:.bin=.elf) $^ $(LDFLAGS) -Ttext 0x7c00


stage2: $(STAGE2) $(STAGE2:.bin=.elf)
$(STAGE2): $(STAGE2_OBJ)
	@echo "LD	$(STAGE2)"
	@$(LD) -o $(STAGE2) $^ $(LDFLAGS) -T$(STAGE2_LD)
# used for debugging
$(STAGE2:.bin=.elf): $(STAGE2_OBJ)
	@echo "LD	$(STAGE2:.bin=.elf)"
	@$(LD) -o $(STAGE2:.bin=.elf) $^ $(LDFLAGS) -T$(STAGE2_LD) --oformat=elf32-i386


# only used in GDB
structs: $(STRUCTS_OBJ)
$(STRUCTS_OBJ): $(STRUCTS_SRC)
	@echo "CC	$(STRUCTS_OBJ)"
	@$(CC) -o $(STRUCTS_OBJ) -c $(STRUCTS_SRC) -O0 -g


# create "bootloader partition"
$(BOOTPART): $(STAGE1) $(STAGE2)
	@echo "PART	$(BOOTPART)"
	@cat $(STAGE1) $(STAGE2) > $(BOOTPART)


# create fat partition
# TODO: mount fs and do things
$(FATPART):
	@echo "PART	$(FATPART)	$(FATPARTSZ) sectors"
	@dd if=/dev/zero of=$(FATPART) bs=512 count=$(FATPARTSZ) >/dev/null 2>&1
	@mkfs.vfat -F32 $(FATPART) >/dev/null


debug: bin $(DEBUG)


# partition and combine to disk image
# NOTE: for some reason debug causes rebuilds of iso, so run `make debug iso`
# instead of `make iso debug` when using both targets
iso: $(ISO)
$(ISO): bin $(MBR) $(BOOTPART) $(FATPART)
	@echo "ISO	partition.sh"
	@./partition.sh -vfm "$(MBR)" "$(ISO)" "$(BOOTPART):13::y" "$(FATPART):0b"

.PHONY: clean debug structs mbr stage1 stage2 iso
