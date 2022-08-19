#!/bin/sh

help_str="\
Usage:
  $0 [options]

Options:
  -h    print this help message
  -d mode
        start debugging mode and connect with gdb
        mode can be either \"bootsect\" or \"kernel\"
  -D    start debugging mode withouut connecting with gdb"

print_help() {
  echo "$help_str"
}

qemu_args="-drive format=raw,file=shitos.iso"
gdb_args="-x gdb/run.gdb"

# Arguments
debug=0
run_gdb=0

while getopts ":hd:D" o; do case "$o" in
  d) debug=1 && run_gdb=1
    case "$OPTARG" in
      bootsect) gdb_args="$gdb_args -ex debug_bootsect" ;;
      stage1) gdb_args="$gdb_args -ex debug_stage1" ;;
      stage2) gdb_args="$gdb_args -ex debug_stage2" ;;
      kernel) gdb_args="$gdb_args -ex debug_kernel" ;;
    esac ;;
  D) debug=1 ;;
  *) print_help && exit 1 ;;
esac done;
shift $((OPTIND-1))

[ "$debug" = 1 ] && qemu_args="$qemu_args -S -s"

# Run QEMU + gdb
qemu_cmd="qemu-system-i386 $qemu_args"
gdb_cmd="gdb $gdb_args"

echo "Running QEMU: \"$qemu_cmd\""
setsid -f $qemu_cmd &

if [ "$run_gdb" = 1 ]; then
  echo "Running gdb: \"$gdb_cmd\""
  $gdb_cmd
fi
