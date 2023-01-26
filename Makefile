ROOT=./
include include/mk/programs.mk
include include/mk/def_flags.mk

# binaries
MBR=bin/mbr.bin
STAGE1=bin/stage1.bin
STAGE2=bin/stage2.bin

STRUCTS_SRC=gdb/structs.c
STRUCTS_OBJ=$(STRUCTS_SRC:.c=.o)

# output files
DIRS=bin partitions working
BUILDFILES=$(BOOTPART) $(EXTPART) $(EXTPART).tmp $(ISO)

ISO=shitos.iso
BOOTPART=partitions/bootpart.img
EXTPART=partitions/extpart.img
EXTPARTSZ=128k


all: dirs mbr stage1 stage2 kernel $(BOOTPART) $(EXTPART)


clean:
	@echo Cleaning root
	rm -f $(BUILDFILES)

	@(cd $(ROOT)/src/boot && env make clean)
	@(cd $(ROOT)/src/kernel && env make clean)


include include/mk/compile.mk


# NOTE: some targets depend on `dirs` but do not list it as a prerequisite,
# so make sure to run `make dirs` once manually in case of issues
dirs: $(DIRS)
$(DIRS):
	@echo "MKDIR	$@"
	@mkdir -p $@


mbr:
	@(cd $(ROOT)/src/boot && env make mbr)
stage1:
	@(cd $(ROOT)/src/boot && env make stage1)
stage2:
	@(cd $(ROOT)/src/boot && env make stage2)
kernel:
	@(cd $(ROOT)/src/kernel && env make)


# only used in GDB
structs: $(STRUCTS_OBJ)
$(STRUCTS_OBJ): $(STRUCTS_SRC)
	@echo "CC	$(shell basename $(STRUCTS_OBJ))"
	@$(CC) -o $(STRUCTS_OBJ) -c $(STRUCTS_SRC) -O0 -g


# create "bootloader partition"
$(BOOTPART): stage1 stage2
	@echo "PART	$(shell basename $(BOOTPART))"
	@cat $(STAGE1) $(STAGE2) > $(BOOTPART)


$(EXTPART): kernel
	@echo "PART	$(shell basename $(EXTPART))	$(EXTPARTSZ) sectors"
	@rm $(EXTPART).tmp 2>/dev/null || echo jank >/dev/null
	@dd if=/dev/zero of=$(EXTPART).tmp bs=512 count=$(EXTPARTSZ) \
		>/dev/null 2>&1
	@mkfs.ext2 -b4096 $(EXTPART).tmp >/dev/null

# TODO: maybe use FUSE for better cross-platform compatibility?
	@debugfs -wf src/debugfs_ext2 $(EXTPART).tmp
	@mv $(EXTPART).tmp $(EXTPART)
# HACK: touch output file to stop rebuilds due to `working` being newer
	@touch $(EXTPART)


# FIXME: for the time being debug (except structs) is half-forced on all builds
debug: dirs structs #$(DEBUG)


# partition and combine to disk image
iso: dirs $(ISO)
$(ISO): mbr $(BOOTPART) $(EXTPART)
	@echo "ISO	partition.sh"
	@rm $(ISO) 2>/dev/null || echo jank >/dev/null
	@./partition.sh -vfm "$(MBR)" "$(ISO)" \
		"$(BOOTPART):13::y" "$(EXTPART):linux"


.PHONY: all clean dirs debug structs mbr stage1 stage2 iso
