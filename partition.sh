#!/bin/sh

print_help() {
  echo "Usage: $0 <output file> <sectors>" >&2
  exit 1
}

cmd() {
  echo "$@"
  $@
}

[ -z "$1" ] || [ -z "$2" ] && print_help

outfile="$1"

# Jank
numfmt --from=iec "$2" >/dev/null || exit 1
sectors=$(numfmt --from=iec "$2")
[ "$sectors" -lt 65536 ] && { echo "At least 65536 sectors are needed" >&2; exit 1; }

rm -f "$outfile"
cmd dd if=/dev/zero of="$outfile" bs=512 count=$((sectors + 2048))

cmd fdisk "$1" << EOF
n
p

2048
+$((sectors - 1))
t
0b
a
p
w
EOF
