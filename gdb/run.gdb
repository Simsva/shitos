# Macros
define qq
  kill
  quit
end

define debug_mbr
  file bin/mbr.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Will skip the relocation to 0x600
  break _highstart
  continue
end

define debug_stage1
  file bin/stage1.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Break at boot sector
  break *0x7c00
  continue
  continue
end

define debug_stage2
  file bin/stage2.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Break at where stage2 is loaded in RAM
  break *0x9000
  # break _entry32
  continue
end

define debug_stage2pe
  file bin/stage2.elf

  set architecture i386

  break _entry32
  break bmain
  continue
end

# NOTE: old, do not use
define debug_kernel
  file bin/kernel.elf

  break *0x10000
  continue
end

# Run
add-symbol-file gdb/structs.o 0

# Connect to QEMU
target remote :1234
