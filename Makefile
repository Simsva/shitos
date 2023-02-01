ROOT=./
include include/mk/def_flags.mk
include include/mk/programs.mk

# binaries
MBR=bin/mbr.bin
STAGE1=bin/stage1.bin
STAGE2=bin/stage2.bin

STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

# output files
DIRS=bin partitions $(SYSROOT)
BUILDFILES=$(BOOTPART) $(EXTPART) $(ISO) $(STRUCTS_OBJ)

ISO=shitos.iso
BOOTPART=partitions/bootpart.img
EXTPART=partitions/extpart.img
EXTPARTSZ=128k


all: dirs mbr stage1 stage2 kernel $(BOOTPART) $(EXTPART)


clean:
	@echo Cleaning root
	rm -f $(BUILDFILES)

	@(cd $(ROOT)/src/boot/$(ARCH) && env make clean)
	@(cd $(ROOT)/src/kernel && env make clean)
	@(cd $(ROOT)/src/libc && env make clean)


clean_root:
	@echo Cleaning sysroot
	@rm -rf $(SYSROOT)/*


include include/mk/compile.mk


# NOTE: some targets depend on `dirs` but do not list it as a prerequisite,
# so make sure to run `make dirs` once manually in case of issues
dirs: $(DIRS)
$(DIRS):
	@echo "MKDIR	$@"
	@mkdir -p $@


headers:
	@(cd $(ROOT)/src/boot/$(ARCH) && env make install_headers)
	@(cd $(ROOT)/src/kernel && env make install_headers)
	@(cd $(ROOT)/src/libc && env make install_headers)
mbr: headers
	@(cd $(ROOT)/src/boot/$(ARCH) && env make mbr)
stage1: headers
	@(cd $(ROOT)/src/boot/$(ARCH) && env make stage1)
stage2: headers libc
	@(cd $(ROOT)/src/boot/$(ARCH) && env make stage2)
# FIXME: kernel depends on headers in stage2
kernel: headers libc
	@(cd $(ROOT)/src/kernel && env make kernel install)
libc: headers
	@(cd $(ROOT)/src/libc && env make all install)


# only used in GDB
structs: $(STRUCTS_OBJ)
$(STRUCTS_OBJ): $(STRUCTS_SRC)
	@echo "CC	$(shell basename $@)"
	@$(CC) -o $(STRUCTS_OBJ) -c $(STRUCTS_SRC) -O0 -g


# create "bootloader partition"
$(BOOTPART): stage1 stage2
	@echo "PART	$(shell basename $@)"
	@cat $(STAGE1) $(STAGE2) > $(BOOTPART)


$(EXTPART): kernel
	@echo "PART	$(shell basename $@)	$(EXTPARTSZ) sectors"
	@dd if=/dev/zero of=$(EXTPART) bs=512 count=$(EXTPARTSZ) >/dev/null 2>&1
	@mke2fs -b4096 -d$(SYSROOT) $(EXTPART)


debug: dirs structs


# partition and combine to disk image
iso: dirs $(ISO)
$(ISO): mbr $(BOOTPART) $(EXTPART)
	@echo "ISO	partition.sh"
	@rm $(ISO) 2>/dev/null || echo jank >/dev/null
	@./partition.sh -vfm "$(MBR)" "$(ISO)" \
		"$(BOOTPART):13::y" "$(EXTPART):linux"


.PHONY: all clean dirs debug structs mbr stage1 stage2 kernel libc iso
