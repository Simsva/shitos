# Macros
define qq
  kill
  quit
end

define debug_bootsect
  file bin/boot.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Break at boot sector
  break *0x7c00
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
end

define debug_stage2
  file bin/stage2.elf

# 16-bit real mode
  set tdesc filename gdb/target.xml
  set architecture i8086

# Break at boot sector
  break *0x7c00
  continue
  continue
end

define debug_kernel
  file bin/kernel.elf

  break *0x10000
  continue
end

# Run

# Connect to QEMU
target remote :1234
