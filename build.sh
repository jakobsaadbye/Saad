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

gcc $CFLAGS -o ./bin/saad build/ast.o build/lexer.o build/parser.o build/scope.o src/main.c

echo "Built ./bin/saad"
