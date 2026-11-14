#!/usr/bin/env bash
# Runs tests/raylib/*.sd, which call the functions of raymath (std/vendor/raymath.sd) and the operators of raylib.sd, on
# Linux. raylib.dll is for Windows, so raymath.h of packages/raylib/include is compiled here instead, with the calling
# convention of Windows (ms_abi) that the compiler uses for extern functions, and its functions renamed to the 'ms_' names of
# shim.c. The object is put together with the object of the program, and linked like any program.
#
#   tools/linux/check_raymath.sh            with the x64 backend
#   tools/linux/check_raymath.sh --llvm     with the LLVM backend
#
# Requirements: gcc, objcopy and ld (binutils)
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/raymath.c" <<'C'
typedef struct Vector2 { float x, y; } Vector2;
typedef struct Vector3 { float x, y, z; } Vector3;
typedef struct Vector4 { float x, y, z, w; } Vector4;
typedef Vector4 Quaternion;
typedef struct Matrix { float m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15; } Matrix;
#define RL_VECTOR2_TYPE
#define RL_VECTOR3_TYPE
#define RL_VECTOR4_TYPE
#define RL_QUATERNION_TYPE
#define RL_MATRIX_TYPE
#define RAYMATH_IMPLEMENTATION
#include "raymath.h"
C
# With BUILD_LIBTYPE_SHARED, raymath.h declares its functions with __attribute__((visibility("default"))), which becomes ms_abi
gcc -O1 -w -c -I"$REPO/packages/raylib/include" -DBUILD_LIBTYPE_SHARED '-Dvisibility(x)=ms_abi' "$TMP/raymath.c" -o "$TMP/raymath0.o"
nm "$TMP/raymath0.o" | awk '$2 == "T" { print $3 " ms_" $3 }' > "$TMP/names"
objcopy --redefine-syms="$TMP/names" "$TMP/raymath0.o" "$TMP/raymath.o"

failed=0
for f in "$REPO"/tests/raylib/*.sd; do
    if ! "$REPO/bin/saad" build "$f" --elf $1 > "$TMP/build.txt" 2>&1; then
        echo "$f: COMPILE ERROR"; grep -i error "$TMP/build.txt" | head -3; failed=1; continue
    fi
    ld -r "$(dirname "$f")/build/out.o" "$TMP/raymath.o" -o "$TMP/program.o" 2> /dev/null
    bash "$HERE/link.sh" "$TMP/program.o" "$TMP/program" 2> /dev/null
    if ! timeout 10 "$TMP/program"; then echo "$f: RUN FAILED"; failed=1; fi
done
exit $failed
