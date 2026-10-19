#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_dir=$(dirname -- "$script_dir")

"$script_dir/generate_rainbow_pict_resource.sh"

cmake -S "$project_dir" -B "$project_dir/build" \
    -DCMAKE_TOOLCHAIN_FILE=/Users/alex/Git/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build "$project_dir/build"
