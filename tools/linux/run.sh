#!/usr/bin/env bash
# Compiles and runs a Saad program on Linux.
#
#   tools/linux/run.sh path/to/program.sd [args]    compile with bin/saad, then run with the arguments
#   tools/linux/run.sh path/to/out.o         run a program compiled with 'saad build --elf'
#   tools/linux/run.sh path/to/out.asm       run generated assembly, assembled with nasm
#
# The exit code is the exit code of the program (asserts exit with 1).
# Requirements: gcc (and nasm for .asm, libpq-dev for std/dbms/postgres, libssl-dev for https). Build the compiler first
# with ./build.sh. To keep the program, link it with tools/linux/link.sh
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
        "$REPO/bin/saad" build "$INPUT" --elf $SAAD_BUILD_FLAGS > /dev/null || { "$REPO/bin/saad" build "$INPUT" --elf $SAAD_BUILD_FLAGS; exit 3; }
        OBJECT="$(cd "$(dirname "$INPUT")" && pwd)/build/out.o"
        ;;
    *.o)
        OBJECT="$INPUT"
        ;;
    *)
        ASM="$INPUT"
        ;;
esac

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -z "$OBJECT" ]; then
    nasm -felf64 --prefix ms_ "$ASM" -o "$TMP/program.o"
    OBJECT="$TMP/program.o"
fi
# With the shims, libpq and OpenSSL that it uses
bash "$HERE/link.sh" "$OBJECT" "$TMP/program"

set +e
timeout 10 "$TMP/program" "${@:2}"
