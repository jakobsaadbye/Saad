#!/usr/bin/env bash
# Links a Saad program for Linux into an executable: the object of 'saad build --elf' with the C functions of shim.c,
# and of shim_pq.c (libpq) and shim_ssl.c (OpenSSL) when the program uses them.
#
#   saad build main.sd --elf                      # build/out.o
#   tools/linux/link.sh build/out.o build/main    # build/main, a Linux program
#
# The shims are compiled once into build/linux of the repository. Requirements: gcc, and libpq-dev for std/dbms/postgres
# and libssl-dev for TLS (https)
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
OBJECT="$1"
OUTPUT="$2"
if [ -z "$OBJECT" ] || [ -z "$OUTPUT" ]; then
    echo "Usage: $0 <out.o> <executable>"
    exit 2
fi

CACHE="${SAAD_SHIM_CACHE:-$REPO/build/linux}"
mkdir -p "$CACHE"

# Compiles a shim into the cache when it isn't there or is older than its source: shim <name> [flags]
shim() {
    local object="$CACHE/$1.o"
    if [ ! -f "$object" ] || [ "$object" -ot "$HERE/$1.c" ]; then
        gcc -O1 -w -c "${@:2}" "$HERE/$1.c" -o "$object.tmp.$$"
        mv "$object.tmp.$$" "$object"
    fi
    echo "$object"
}

OBJECTS="$(shim shim -I"$REPO/src")"
LIBRARIES=""
SYMBOLS="$(nm "$OBJECT" 2>/dev/null || true)"
# libpq (std/dbms/postgres) and OpenSSL (std/net/tls.sd) only for the programs that call them
if printf '%s' "$SYMBOLS" | grep -q ' U ms_PQ'; then
    OBJECTS="$OBJECTS $(shim shim_pq -I/usr/include/postgresql)"
    LIBRARIES="$LIBRARIES -lpq"
fi
if printf '%s' "$SYMBOLS" | grep -qE ' U ms_(SSL_|TLS_|ERR_)'; then
    OBJECTS="$OBJECTS $(shim shim_ssl)"
    LIBRARIES="$LIBRARIES -lssl -lcrypto"
fi
gcc -no-pie -z noexecstack "$OBJECT" $OBJECTS $LIBRARIES -o "$OUTPUT" -lm -lpthread
