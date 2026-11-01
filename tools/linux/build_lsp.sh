#!/usr/bin/env bash
# Builds the language server (lsp/) for Linux, at build/linux/saad-lsp. See tools/linux/README.md
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"

"$REPO/bin/saad" build "$REPO/lsp/main.sd" --elf > /dev/null || { "$REPO/bin/saad" build "$REPO/lsp/main.sd" --elf; exit 3; }

CACHE="$REPO/build/linux"
mkdir -p "$CACHE"
SHIM_O="$CACHE/shim.o"
if [ ! -f "$SHIM_O" ] || [ "$SHIM_O" -ot "$HERE/shim.c" ]; then
    gcc -O0 -w -c -I"$REPO/src" "$HERE/shim.c" -o "$SHIM_O"
fi
gcc -no-pie -z noexecstack "$REPO/lsp/build/out.o" "$SHIM_O" -o "$CACHE/saad-lsp" -lm
echo "$CACHE/saad-lsp"
