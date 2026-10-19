#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(dirname -- "$script_dir")
source_image=${1:-"$project_dir/rainbow.pict"}
output_file=${2:-"$project_dir/src/rainbow.pict"}

if [ ! -f "$source_image" ]; then
    echo "error: source PICT not found: $source_image" >&2
    exit 1
fi

file_size=$(wc -c < "$source_image" | tr -d ' ')
if [ "$file_size" -lt 10 ]; then
    echo "error: PICT file is too small: $source_image" >&2
    exit 1
fi

# Standalone PICT files conventionally contain a 512-byte file header. A PICT
# resource starts directly with the picture-size word and bounding rectangle.
offset=0
if [ "$file_size" -ge 522 ]; then
    set -- $(od -An -tu1 -N2 -j512 "$source_image")
    headered_size=$(($1 * 256 + $2))
    if [ "$headered_size" -eq $((file_size - 512)) ]; then
        offset=512
    fi
fi

payload_size=$((file_size - offset))
set -- $(od -An -tu1 -N2 -j"$offset" "$source_image")
declared_size=$(($1 * 256 + $2))
if [ "$declared_size" -ne 0 ] && [ "$declared_size" -ne "$payload_size" ]; then
    echo "error: PICT declares $declared_size bytes, but payload has $payload_size" >&2
    exit 1
fi

output_dir=$(dirname -- "$output_file")
mkdir -p "$output_dir"
temporary_file="$output_file.tmp.$$"
trap 'rm -f "$temporary_file"' EXIT HUP INT TERM
dd if="$source_image" of="$temporary_file" bs=1 skip="$offset" 2>/dev/null
mv "$temporary_file" "$output_file"
trap - EXIT HUP INT TERM

echo "Generated $output_file from $source_image ($payload_size-byte PICT resource)"
