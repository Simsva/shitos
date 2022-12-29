# macros
define qq
  kill
  quit
end

# debug Master Boot Record
define debug_mbr
  file bin/mbr.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Will skip the relocation to 0x600
  break _highstart
  continue
end

# debug stage1 bootloader
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

# debug stage2 bootloader
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

# debug stage2 bootloader protected mode part
define debug_stage2pe
  file bin/stage2.elf

  set architecture i386

  break _entry32
  break bmain
  continue
end

# run
add-symbol-file gdb/structs.o 0

# connect to QEMU
target remote :1234
