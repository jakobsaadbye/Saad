#!/usr/bin/env bash
# Runs tests/db/*.sd against a Postgres server.
#
#   tools/linux/dbtests.sh
#
# With SAAD_TEST_POSTGRES set to a connection string (e.g "host=localhost dbname=test user=postgres"), that
# server is used. Otherwise a throwaway server is made in a temporary folder and removed afterwards.
# Requirements: gcc, PostgreSQL (initdb, pg_ctl) and libpq-dev, e.g 'apt install postgresql libpq-dev'

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
cd "$REPO"
bash ./build.sh > /dev/null || exit 1

RED=$'\e[31m'; GREEN=$'\e[32m'; RESET=$'\e[0m'

stop_server() { :; }
if [ -z "$SAAD_TEST_POSTGRES" ]; then
    BIN="$(pg_config --bindir 2>/dev/null)"
    [ -x "$BIN/initdb" ] || BIN="$(ls -d /usr/lib/postgresql/*/bin 2>/dev/null | sort -V | tail -1)"
    if [ ! -x "$BIN/initdb" ]; then
        echo "No PostgreSQL found (initdb). Install it, or set SAAD_TEST_POSTGRES to a connection string"
        exit 1
    fi

    DIR="$(mktemp -d /tmp/saad-pg-XXXXXX)"
    PORT=$((50000 + RANDOM % 10000))
    # Postgres doesn't run as root
    AS=""
    if [ "$(id -u)" = 0 ]; then
        chown postgres "$DIR"
        AS="su postgres -s /bin/bash -c"
    fi
    run_pg() { if [ -n "$AS" ]; then $AS "$*"; else bash -c "$*"; fi; }

    run_pg "'$BIN/initdb' -D '$DIR/data' -U postgres --auth=trust" > "$DIR/initdb.log" 2>&1 || { cat "$DIR/initdb.log"; exit 1; }
    run_pg "'$BIN/pg_ctl' -D '$DIR/data' -l '$DIR/server.log' -w -o \"-p $PORT -k $DIR -c listen_addresses=localhost\" start" > /dev/null || { cat "$DIR/server.log"; exit 1; }
    stop_server() {
        run_pg "'$BIN/pg_ctl' -D '$DIR/data' -m immediate stop" > /dev/null 2>&1
        rm -rf "$DIR"
    }
    trap stop_server EXIT
    export SAAD_TEST_POSTGRES="host=localhost port=$PORT dbname=postgres user=postgres"
fi

passed=0; failed=0
# generate.sd runs first: it checks the files that shop_tables.sd uses
for f in tests/db/*.sd; do
    printf "%-56s " "$f"
    if output=$(bash "$HERE/run.sh" "$f" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | tail -5 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

# The program std/dbms/tools/generate.sd writes the same files as tests/db/generate.sd compares (with the schema it left)
printf "%-56s " "std/dbms/tools/generate.sd"
OUT="$(mktemp -d)"
if output=$(bash "$HERE/run.sh" std/dbms/tools/generate.sd "$SAAD_TEST_POSTGRES" "$OUT/db" saad_shop 2>&1) && diff -r "$OUT/db" tests/db/model > /dev/null; then
    echo "${GREEN}OK${RESET}"
    passed=$((passed + 1))
else
    echo "${RED}FAILED${RESET}"
    echo "$output" | tail -5 | sed 's/^/    /'
    failed=$((failed + 1))
fi
rm -rf "$OUT"

echo
echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
