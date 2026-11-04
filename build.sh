#!/usr/bin/env bash
# Builds the compiler on Linux / macOS. Equivalent of make.bat
#
# Note: The x64 backend targets Win64, so on these platforms `saad build` stops after
# assembling ./build/out.asm with nasm (linking and running are Windows only)
set -e

cd "$(dirname "$0")"

CFLAGS="-Wall -Wextra -pedantic -ggdb"
FILES="ast lexer parser scope"

mkdir -p build bin

for f in $FILES; do
    gcc $CFLAGS -c src/$f.c -o build/$f.o
done

# The LLVM backend (--llvm) when LLVM is installed with its C headers, e.g 'apt install llvm-18-dev'.
# SAAD_LLVM=0 ./build.sh builds without it
LLVM_FLAGS=""
LLVM_CONFIG="$(command -v llvm-config-18 || command -v llvm-config || true)"
if [ "${SAAD_LLVM:-1}" != "0" ] && [ -n "$LLVM_CONFIG" ] && [ -f "$("$LLVM_CONFIG" --includedir)/llvm-c/Core.h" ]; then
    LLVM_LIBDIR="$("$LLVM_CONFIG" --libdir)"
    LLVM_FLAGS="-DSAAD_LLVM -isystem $("$LLVM_CONFIG" --includedir) -L$LLVM_LIBDIR -Wl,-rpath,$LLVM_LIBDIR -lLLVM-$("$LLVM_CONFIG" --version | cut -d. -f1)"
fi

gcc $CFLAGS -o ./bin/saad build/ast.o build/lexer.o build/parser.o build/scope.o src/main.c $LLVM_FLAGS

if [ -n "$LLVM_FLAGS" ]; then echo "Built ./bin/saad (with LLVM)"; else echo "Built ./bin/saad"; fi
