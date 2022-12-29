#!/bin/sh

die() {
  echo "$1" >&2
  exit 1
}

usage="\
SYNOPSIS
    $0 [option]...

DESCRIPTION
    Run \"shitos.iso\" using QEMU.

    -d mode
          start debugging mode and connect with gdb
          mode can be any of the ones defined as \"debug_MODE\"
          in \"gdb/run.gdb\"
    -D    start debugging mode withouut connecting with gdb
    -h    print this help message"

qemu_args="-drive format=raw,file=shitos.iso"
gdb_args="-x gdb/run.gdb"

# Arguments
debug=0
run_gdb=0

while getopts ":hd:D" OPT; do case "$OPT" in
  d)  debug=1; run_gdb=1
      gdb_args="$gdb_args -ex debug_$OPTARG"
      ;;
  D)  debug=1 ;;
  *)  die "$usage" ;;
esac done;
shift $((OPTIND-1))

[ "$debug" -eq 1 ] && qemu_args="$qemu_args -S -s"

# Run QEMU + gdb
qemu_cmd="qemu-system-i386 $qemu_args"
gdb_cmd="gdb $gdb_args"

echo "Running QEMU: \"$qemu_cmd\""
# shellcheck disable=SC2086
setsid -f $qemu_cmd &

[ "$run_gdb" -eq 1 ] && {
  echo "Running GDB: \"$gdb_cmd\""
  $gdb_cmd
}
