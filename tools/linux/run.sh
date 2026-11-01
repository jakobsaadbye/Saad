#!/usr/bin/env bash
# Compiles and runs a Saad program on Linux.
#
#   tools/linux/run.sh path/to/program.sd    compile with bin/saad, then run
#   tools/linux/run.sh path/to/out.o         run a program compiled with 'saad build --elf'
#   tools/linux/run.sh path/to/out.asm       run generated assembly, assembled with nasm
#
# The exit code is the exit code of the program (asserts exit with 1).
# Requirements: gcc (and nasm for .asm). Build the compiler first with ./build.sh
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
INPUT="$1"

if [ -z "$INPUT" ]; then
    echo "Usage: $0 <file.sd | out.o | out.asm>"
    exit 2
fi

case "$INPUT" in
    *.sd)
        # The compiler's own assembler writes an ELF object, with the 'ms_' names of shim.c
        "$REPO/bin/saad" build "$INPUT" --elf > /dev/null || { "$REPO/bin/saad" build "$INPUT" --elf; exit 3; }
        OBJECT="$(cd "$(dirname "$INPUT")" && pwd)/build/out.o"
        ;;
    *.o)
        OBJECT="$INPUT"
        ;;
    *)
        ASM="$INPUT"
        ;;
esac

# The shim (the C wrappers) is compiled once and cached
CACHE="$REPO/build/linux"
mkdir -p "$CACHE"
SHIM_O="$CACHE/shim.o"
if [ ! -f "$SHIM_O" ] || [ "$SHIM_O" -ot "$HERE/shim.c" ]; then
    gcc -O0 -w -c -I"$REPO/src" "$HERE/shim.c" -o "$SHIM_O"
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -z "$OBJECT" ]; then
    nasm -felf64 --prefix ms_ "$ASM" -o "$TMP/program.o"
    OBJECT="$TMP/program.o"
fi
gcc -no-pie -z noexecstack "$OBJECT" "$SHIM_O" -o "$TMP/program" -lm

set +e
timeout 10 "$TMP/program"
