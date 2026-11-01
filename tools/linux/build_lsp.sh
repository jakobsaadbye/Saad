#!/usr/bin/env bash
# Builds the language server (lsp/) for Linux, at build/linux/saad-lsp. See tools/linux/README.md
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"

"$REPO/bin/saad" build "$REPO/lsp/main.sd" > /dev/null || { "$REPO/bin/saad" build "$REPO/lsp/main.sd"; exit 3; }

CACHE="$REPO/build/linux"
mkdir -p "$CACHE"
SHIM_O="$CACHE/shim.o"
if [ ! -f "$SHIM_O" ] || [ "$SHIM_O" -ot "$HERE/shim.c" ] || [ "$SHIM_O" -ot "$REPO/src/runtime_support.c" ]; then
    gcc -O0 -w -c -I"$REPO/src" "$HERE/shim.c" -o "$SHIM_O"
fi
nasm -felf64 --prefix ms_ "$REPO/lsp/build/out.asm" -o "$CACHE/saad-lsp.o"
gcc -no-pie -z noexecstack "$CACHE/saad-lsp.o" "$SHIM_O" -o "$CACHE/saad-lsp" -lm
echo "$CACHE/saad-lsp"
