# macros
define qq
  kill
  quit
end

# 32-bit protected mode
define arch_pm
  unset tdesc filename
  set architecture i386
end

# 16-bit real mode
define arch_rm
  set tdesc filename gdb/target.xml
  set architecture i8086
end

# debug Master Boot Record
define debug_mbr
  file bin/mbr.elf
  arch_rm

# Will skip the relocation to 0x600
  break _highstart
  continue
end

# debug stage1 bootloader
define debug_stage1
  file bin/stage1.elf
  arch_rm

# Break at boot sector
  break *0x7c00
  continue
  continue
end

# debug stage2 bootloader
define debug_stage2
  file bin/stage2.elf
  arch_rm

# Break at where stage2 is loaded in RAM
  break *0x9000
  # break _entry32
  continue
end

# debug stage2 bootloader protected mode part
define debug_stage2pe
  file bin/stage2.elf
  arch_pm

  break _entry32
  break bmain
  continue
end

# debug kernel (kind of temporary)
define debug_kernel
  file bin/shitos.elf
  arch_pm

  break kmain
  continue
end

# v86 debugging
define p_v86_rm_stack
  p/x *(struct v86_rm_stack *)($arg0 - sizeof(struct v86_rm_stack))
end

define p_v86_kernel_stack
  p/x *(struct v86_kernel_stack *)($arg0 + 4 - sizeof(struct v86_kernel_stack))
end
define p_v86_rret_tramp_stack
  p/x *(struct v86_rret_tramp_stack *)($arg0 - sizeof(struct v86_rret_tramp_stack))
end


# run
add-symbol-file gdb/structs.o 0

# connect to QEMU
target remote :1234
