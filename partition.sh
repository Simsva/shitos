#!/bin/sh

die() {
  echo "$1" >&2
  exit 1
}

cmd() {
  if [ "$verbosity" -gt 1 ]; then
    echo "$@"
    "$@"
  else
    "$@" >/dev/null 2>&1
  fi
}

parse_part() {
  part_file="$(echo "$1" | cut -d: -f1)"
  part_type="$(echo "$1" | cut -d: -f2)"
  part_sect="$(echo "$1" | cut -d: -f3)"
  part_boot="$(echo "$1" | cut -d: -f4)"

  [ "$part_file" ] && [ ! -r "$part_file" ] \
    && die "Provided file in partition $part_num does not exist or the user\
does not have read access"

  if [ "$part_sect" ]; then
    numfmt --from=iec "$part_sect" >/dev/null 2>&1 \
      || die "Invalid sectors format in partition $part_num"
    part_sect="$(numfmt --from=iec "$part_sect")"
  else
    part_sect=0
  fi

  # FIXME: assumes a block size of 512
  tmp_sect=0
  [ "$part_file" ] && tmp_sect="$(stat -c %b "$part_file")"
  [ "$part_sect" -lt "$tmp_sect" ] && part_sect="$tmp_sect"

  [ "$part_boot" ] && part_boot="*"
}

usage="\
SYNOPSIS
    $0 [option]... [--] output_file [partition]...

DESCRIPTION
    Create a disk image with the specified partitions and/or MBR sector.

    See section PARTITION FORMAT for information on how to add partitions.

    -m    MBR image to use
    -v    increment verbosity level
          0 = default
          1 = log debug information
          2 = log commands being run and their output
    -f    force overwrite output_file
    -h    print this help message

PARTITION FORMAT
    \"file:type:sectors:active\"

    Sectors will be determined by the size of file if it is provided, and if
    both are provided, the maximum of the two will be chosen. If file is not
    provided the created partition will be filled with zeros.
    
    Type needs to be a partition type recognized by sfdisk and sectors can be
    provided as a number, optionally using IEC prefixes for binary multiples
    (eg. 64K or 4M).

    If active is not empty the partition will be marked as active."

out_file=""
mbr_file=""
verbosity=0
force=0

while getopts "m:vfh" OPT; do case "$OPT" in
  m) mbr_file="$OPTARG" ;;
  v) verbosity=$((verbosity+1)) ;;
  f) force=1 ;;
  *) die "$usage" ;;
esac done
shift "$((OPTIND-1))"

# mbr_file validation
[ "$mbr_file" ] && [ ! -r "$mbr_file" ] \
  && die "provided MBR file does not exist or you do not have read access"

# output_file validation
[ -z "$1" ] && die "$usage"
[ -e "$1" ] && [ ! -f "$1" ] \
  && die "output_file exists and is not a regular file"
[ -f "$1" ] && [ "$force" -eq 0 ] \
  && die "output_file exists and force overwrite is disabled"
[ -f "$1" ] && [ ! -w "$1" ] \
  && die "output_file exists without write access"
out_file="$1"
shift

[ "$verbosity" -gt 0 ] && {
  echo "Options:"
  echo "  mbr_file : $mbr_file"
  echo "  out_file : $out_file"
  echo "  verbosity: $verbosity"
}

# parse partitions
part_num=0
out_sect=0
out_parts=""
out_part_files=""
while [ "$1" ]; do
  part_num=$((part_num+1))
  parse_part "$1"
  out_sect=$((out_sect+part_sect))

  [ "$verbosity" -gt 0 ] && {
    echo "Parsing partition $part_num:"
    echo "  file   : $part_file"
    echo "  type   : $part_type"
    echo "  sectors: $part_sect"
    echo "  active : $part_boot"
  }

  out_parts="$out_parts
, $part_sect, $part_type, $part_boot"
  if [ "$part_file" ]; then
    out_part_files="$out_part_files
$part_num:dd if=$part_file of=$out_file conv=notrunc bs=512 seek=%d count=$part_sect"
  else
    out_part_files="$out_part_files
$part_num:"
  fi

  shift
done
out_sect=$((out_sect + part_num*2048))
[ "$part_num" -gt 4 ] && die "Too many partitions"

# create and partition out_file
cmd rm "$out_file"
cmd dd if=/dev/zero of="$out_file" bs=512 count="$out_sect"
cmd sfdisk "$out_file" << EOF
label: dos
$out_parts
write
EOF

# copy mbr_file into out_file, up to the partition table
[ "$mbr_file" ] && {
  echo "Copying custom MBR into $out_file"
  cmd dd if="$mbr_file" of="$out_file" conv=notrunc bs=446 seek=0 count=1
}

# copy separate partitions into out_file
parts="$(fdisk -l "$out_file" \
  | sed -E "/^$out_file/!d;s/^$out_file([0-9]+)[* ]+([0-9]+).*/\1:\2/")"
for p in $parts; do
  num="$(echo "$p" | cut -d: -f1)"
  first="$(echo "$p" | cut -d: -f2)"

  # shellcheck disable=SC2059
  dd_cmd="$(printf "$(echo "$out_part_files" \
    | sed -E "/^$num/!d;s/^$num:(.*)/\1/")" "$first")"
  [ "$dd_cmd" ] && {
    echo "Copying custom partition $num into $out_file"
    # shellcheck disable=SC2086
    cmd $dd_cmd
  }
done
