#!/usr/bin/env bash
# Compiles and runs a Saad program on Linux.
#
#   tools/linux/run.sh path/to/program.sd    compile with bin/saad, then run
#   tools/linux/run.sh path/to/out.asm       run already generated assembly
#
# The exit code is the exit code of the program (asserts exit with 1).
# Requirements: gcc and nasm. Build the compiler first with ./build.sh
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
INPUT="$1"

if [ -z "$INPUT" ]; then
    echo "Usage: $0 <file.sd | out.asm>"
    exit 2
fi

case "$INPUT" in
    *.sd)
        "$REPO/bin/saad" build "$INPUT" > /dev/null || { "$REPO/bin/saad" build "$INPUT"; exit 3; }
        ASM="$(cd "$(dirname "$INPUT")" && pwd)/build/out.asm"
        ;;
    *)
        ASM="$INPUT"
        ;;
esac

# The shim (C wrappers + runtime_support.c) is compiled once and cached
CACHE="$REPO/build/linux"
mkdir -p "$CACHE"
SHIM_O="$CACHE/shim.o"
if [ ! -f "$SHIM_O" ] || [ "$SHIM_O" -ot "$HERE/shim.c" ] || [ "$SHIM_O" -ot "$REPO/src/runtime_support.c" ]; then
    gcc -O0 -w -c -I"$REPO/src" "$HERE/shim.c" -o "$SHIM_O"
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

nasm -felf64 --prefix ms_ "$ASM" -o "$TMP/program.o"
gcc -no-pie -z noexecstack "$TMP/program.o" "$SHIM_O" -o "$TMP/program" -lm

set +e
timeout 10 "$TMP/program"
