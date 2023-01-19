CC=i686-elf-gcc
AS=nasm
LD=i686-elf-ld
STRIP=i686-elf-strip

# FIXME: fix compiler options
CFLAGS=-m32 -std=c99 -O2 -Wall -fno-pie -fno-stack-protector
CFLAGS+=-nostdlib -nostdinc -ffreestanding
# maybe bad for optimizations?
CFLAGS+=-fno-builtin-function -fno-builtin
# NOTE: very temporary
CFLAGS+=-g
ASFLAGS=-f elf32 -w+orphan-labels
LDFLAGS=

# source files
MBR=bin/mbr.bin
MBR_SRC=src/boot/mbr.asm
MBR_OBJ=$(MBR_SRC:.asm=_asm.o)

STAGE1=bin/stage1.bin
STAGE1_SRC=src/boot/stage1.asm
STAGE1_OBJ=$(STAGE1_SRC:.asm=_asm.o)

STAGE2=bin/stage2.bin
STAGE2_SRC_ASM=$(wildcard src/boot/stage2/*.asm)
STAGE2_SRC_C=$(wildcard src/boot/stage2/*.c)
STAGE2_OBJ=$(STAGE2_SRC_ASM:.asm=_asm.o) $(STAGE2_SRC_C:.c=_c.o)
STAGE2_LD=src/boot/stage2/link.ld

STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

# output files
DEBUG=$(MBR:.bin=.elf) $(STAGE1:.bin=.elf) $(STAGE2:.bin=.elf) $(STRUCTS_OBJ)
DIRS=bin partitions working
BUILDFILES=./bin/* $(MBR_OBJ) $(STAGE1_OBJ) $(STAGE2_OBJ) $(STRUCTS_OBJ)
BUILDFILES+=$(BOOTPART) $(FATPART) $(FATPART).tmp $(EXTPART) $(EXTPART).tmp
BUILDFILES+=$(ISO)

ISO=shitos.iso
BOOTPART=partitions/bootpart.img
FATPART=partitions/fatpart.img
FATPARTSZ=128k
EXTPART=partitions/extpart.img
EXTPARTSZ=128k


all: dirs $(MBR) $(STAGE1) $(STAGE2) $(BOOTPART) $(EXTPART)


clean:
	@echo Clean
	rm -f $(BUILDFILES)


# HACK: allow C and ASM files with the same basename
%_c.o: %.c
	@echo "CC	$@"
	@$(CC) -o $@ -c $< $(CFLAGS)


%_asm.o: %.asm
	@echo "AS	$@"
	@$(AS) -o $@ $< $(ASFLAGS)


# will probably not work for kernel due to it being an ELF binary
# and we do not want to include debug symbols in actual releases
$(MBR_OBJ): CFLAGS += -g
$(STAGE1_OBJ): CFLAGS += -g
$(STAGE2_OBJ): CFLAGS += -g


# NOTE: some targets depend on `dirs` but do not list it as a prerequisite,
# so make sure to run `make dirs` once manually in case of issues
dirs: $(DIRS)
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
	@$(LD) -o $(STAGE2:.bin=.elf) $^ $(LDFLAGS) -T$(STAGE2_LD) \
		--oformat=elf32-i386


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
$(FATPART):
	@echo "PART	$(FATPART)	$(FATPARTSZ) sectors"
	@rm $(FATPART).tmp 2>/dev/null || echo jank >/dev/null
	@dd if=/dev/zero of=$(FATPART).tmp bs=512 count=$(FATPARTSZ) \
		>/dev/null 2>&1
	@mkfs.vfat -F32 $(FATPART).tmp >/dev/null
# TODO: mount fs and do things
#	@mount -tvfat -oloop $(FATPART).tmp ./working/ >/dev/null
#	@echo "hello world" > ./working/shitos.elf
#	@umount ./working/
	@mv $(FATPART).tmp $(FATPART)
# HACK: touch output file to stop rebuilds due to `working` being newer
	@touch $(FATPART)


$(EXTPART): bin/shitos.elf
	@echo "PART	$(EXTPART)	$(EXTPARTSZ) sectors"
	@rm $(EXTPART).tmp 2>/dev/null || echo jank >/dev/null
	@dd if=/dev/zero of=$(EXTPART).tmp bs=512 count=$(EXTPARTSZ) \
		>/dev/null 2>&1
	@mkfs.ext2 -b4096 $(EXTPART).tmp >/dev/null

# TODO: maybe use FUSE for better cross-platform compatibility?
	@debugfs -wf src/debugfs_ext2 $(EXTPART).tmp
	@mv $(EXTPART).tmp $(EXTPART)
# HACK: touch output file to stop rebuilds due to `working` being newer
	@touch $(EXTPART)


# NOTE: temporary
bin/shitos.elf: src/shitos_c.o
	@echo "LD	shitos.elf"
	@$(LD) -o $@ $^ $(LDFLAGS) -Tsrc/shitos.ld
#	@$(STRIP) $@
#	@python -c 'print("A"*(4096*12) + "B", end="")' > $@


debug: dirs $(DEBUG)


# partition and combine to disk image
iso: dirs $(ISO)
$(ISO): $(MBR) $(BOOTPART) $(EXTPART)
	@echo "ISO	partition.sh"
	@rm $(ISO) 2>/dev/null || echo jank >/dev/null
	@./partition.sh -vfm "$(MBR)" "$(ISO)" \
		"$(BOOTPART):13::y" "$(EXTPART):linux"

.PHONY: clean debug structs mbr stage1 stage2 iso
