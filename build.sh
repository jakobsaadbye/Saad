#!/usr/bin/env bash
# Builds the compiler on Linux / macOS. Equivalent of make.bat
#
# Note: The x64 backend targets Win64, so on these platforms `saad build` stops after
# assembling ./build/out.asm with nasm (linking and running are Windows only)
set -e

cd "$(dirname "$0")"

# Optimized, with debug information. SAAD_DEBUG=1 ./build.sh builds without optimizations, for debugging the compiler
OPTIMIZE="-O2"
if [ "${SAAD_DEBUG:-0}" = "1" ]; then OPTIMIZE="-O0"; fi
CFLAGS="-Wall -Wextra -pedantic -ggdb $OPTIMIZE"
FILES="ast lexer parser scope"

mkdir -p build bin

for f in $FILES; do
    gcc $CFLAGS -c src/$f.c -o build/$f.o
done

# The LLVM backend (--llvm) when LLVM is installed with its C headers, e.g 'apt install llvm-18-dev libzstd-dev'.
# LLVM is linked into saad (its static libraries, only the parts for x64), so saad needs no LLVM to run. When the
# static libraries can't be linked, saad uses LLVM's shared library instead. SAAD_LLVM=0 ./build.sh builds without LLVM
LLVM_CONFIG="$(command -v llvm-config-18 || command -v llvm-config || true)"
if [ "${SAAD_LLVM:-1}" = "0" ] || [ -z "$LLVM_CONFIG" ] || [ ! -f "$("$LLVM_CONFIG" --includedir)/llvm-c/Core.h" ]; then
    gcc $CFLAGS -o ./bin/saad build/ast.o build/lexer.o build/parser.o build/scope.o src/main.c
    echo "Built ./bin/saad"
    exit 0
fi

LLVM_LIBDIR="$("$LLVM_CONFIG" --libdir)"
gcc $CFLAGS -DSAAD_LLVM -isystem "$("$LLVM_CONFIG" --includedir)" -c src/main.c -o build/main.o
OBJECTS="build/ast.o build/lexer.o build/parser.o build/scope.o build/main.o"

STATIC_LIBS="$("$LLVM_CONFIG" --link-static --libs x86codegen x86asmparser passes 2>/dev/null || true)"
if [ -n "$STATIC_LIBS" ] && g++ -o ./bin/saad $OBJECTS -L"$LLVM_LIBDIR" $STATIC_LIBS \
        -Wl,--as-needed $("$LLVM_CONFIG" --link-static --system-libs) -static-libstdc++ -static-libgcc 2> build/llvm_link.txt; then
    echo "Built ./bin/saad (with LLVM)"
else
    echo "note: Couldn't link LLVM's static libraries (see build/llvm_link.txt), so saad uses LLVM's shared library"
    g++ -o ./bin/saad $OBJECTS -L"$LLVM_LIBDIR" -Wl,-rpath,"$LLVM_LIBDIR" -lLLVM-$("$LLVM_CONFIG" --version | cut -d. -f1)
    echo "Built ./bin/saad (with LLVM, shared)"
fi
