#!/usr/bin/env bash
# Checks the debug information with LLDB: builds tests/debug/variables.sd with --debug (and --llvm when given),
# stops in it, and checks that what LLDB shows has each 'expect-debug' line of the program. It also checks the line of
# the call in the calls (bt) and the line of the 'break here' stop.
#
#   tools/linux/check_debug.sh            the x64 backend (dwarf.c)
#   tools/linux/check_debug.sh --llvm     the LLVM backend
#
# Requirements: lldb, and for --llvm a saad built with LLVM. Build the compiler first with ./build.sh
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
PROGRAM="$REPO/tests/debug/variables.sd"
cd "$REPO/tests/debug"

FLAGS="--elf --debug $1"
"$REPO/bin/saad" build variables.sd $FLAGS > /dev/null || { "$REPO/bin/saad" build variables.sd $FLAGS; exit 1; }
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
gcc -O0 -w -c -I"$REPO/src" "$HERE/shim.c" -o "$TMP/shim.o" && gcc -no-pie -z noexecstack build/out.o "$TMP/shim.o" -o "$TMP/variables" -lm -lpthread || exit 1

BREAK_LINE=$(grep -n "// break here" "$PROGRAM" | head -1 | cut -d: -f1)
DESCRIBE_LINE=$(grep -n "// stop in describe" "$PROGRAM" | head -1 | cut -d: -f1)
output=$(timeout 60 lldb -b \
    -o "command script import $REPO/tools/lldb/saad_formatters.py" \
    -o "breakpoint set -f variables.sd -l $DESCRIBE_LINE -c 'i == 1'" \
    -o "breakpoint set -f variables.sd -l $BREAK_LINE" \
    -o run -o "frame variable" -o bt \
    -o continue -o "frame variable" -o "frame variable q.next->x" -o "bt 1" \
    -o kill "$TMP/variables" 2>&1)

CALL_LINE=$(grep -n "r := describe" "$PROGRAM" | head -1 | cut -d: -f1)
missing=0
for expected in "\`describe(p=Point" "\`main at variables.sd:$CALL_LINE" "\`main at variables.sd:$BREAK_LINE"; do
    printf '%s' "$output" | grep -qF -- "$expected" || { echo "missing: $expected"; missing=1; }
done
while IFS= read -r expected; do
    if ! printf '%s' "$output" | grep -qF -- "$expected"; then
        echo "missing: $expected"
        missing=1
    fi
done < <(sed -n 's|^[[:space:]]*// expect-debug: ||p' "$PROGRAM")
if [ $missing -ne 0 ]; then
    echo "--- what LLDB showed:"
    echo "$output"
    exit 1
fi
