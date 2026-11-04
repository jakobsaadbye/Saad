#!/usr/bin/env bash
# Runs tests/db/*.sd against a Postgres server.
#
#   tools/linux/dbtests.sh            with the x64 backend
#   tools/linux/dbtests.sh --llvm     with the LLVM backend
#
# With SAAD_TEST_POSTGRES set to a connection string (e.g "host=localhost dbname=test user=postgres"), that
# server is used. Otherwise a throwaway server is made in a temporary folder and removed afterwards.
# With SAAD_UPDATE_GENERATED=1, the files the generator writes (tests/db/model and examples/blog/db) are written
# again from the schemas, e.g after changing std/dbms/postgres/generate.sd. Look at the diff before committing
# Requirements: gcc, PostgreSQL (initdb, pg_ctl) and libpq-dev, e.g 'apt install postgresql libpq-dev'

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
cd "$REPO"
export SAAD_BUILD_FLAGS=""
[ "$1" = "--llvm" ] && export SAAD_BUILD_FLAGS="--llvm"
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

if [ "$SAAD_UPDATE_GENERATED" = 1 ]; then
    export SAAD_UPDATE_GOLDEN=1   # tests/db/generate.sd writes tests/db/model
    PSQL="psql"
    [ -n "$BIN" ] && [ -x "$BIN/psql" ] && PSQL="$BIN/psql"
    "$PSQL" "$SAAD_TEST_POSTGRES" -q -v ON_ERROR_STOP=1 -f examples/blog/schema.sql > /dev/null || exit 1
    rm -rf examples/blog/db && bash "$HERE/run.sh" examples/blog/generate.sd "$SAAD_TEST_POSTGRES" examples/blog/db > /dev/null || exit 1
    echo "Wrote examples/blog/db again"
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

# The blog example: its schema, the files the generator writes of it, which must be the ones in examples/blog/db, the tour
# of dbms, and the backend, which demo.sd calls over HTTP
printf "%-56s " "examples/blog"
PSQL="psql"
[ -n "$BIN" ] && [ -x "$BIN/psql" ] && PSQL="$BIN/psql"
OUT="$(mktemp -d)"
if ! output=$("$PSQL" "$SAAD_TEST_POSTGRES" -q -v ON_ERROR_STOP=1 -f examples/blog/schema.sql 2>&1); then
    echo "${RED}FAILED (schema.sql)${RESET}"
    echo "$output" | tail -5 | sed 's/^/    /'
    failed=$((failed + 1))
elif ! output=$(bash "$HERE/run.sh" examples/blog/generate.sd "$SAAD_TEST_POSTGRES" "$OUT/db" 2>&1) || ! diff -r "$OUT/db" examples/blog/db > /dev/null; then
    echo "${RED}FAILED (examples/blog/db is not what the generator writes)${RESET}"
    diff -r "$OUT/db" examples/blog/db | head -10 | sed 's/^/    /'
    failed=$((failed + 1))
elif ! output=$(bash "$HERE/run.sh" examples/blog/tour.sd "$SAAD_TEST_POSTGRES" 2>&1) || ! printf '%s' "$output" | grep -q "Grace Hopper: 1 posts, 300 views"; then
    echo "${RED}FAILED (tour.sd)${RESET}"
    echo "$output" | tail -5 | sed 's/^/    /'
    failed=$((failed + 1))
elif ! output=$(timeout 120 bash "$HERE/run.sh" examples/blog/demo.sd "$SAAD_TEST_POSTGRES" 2>&1) || ! printf '%s' "$output" | grep -q '"name":"Grace Hopper","posts":3,"views":511,"followers":1'; then
    echo "${RED}FAILED (demo.sd, the backend)${RESET}"
    echo "$output" | tail -5 | sed 's/^/    /'
    failed=$((failed + 1))
else
    echo "${GREEN}OK${RESET}"
    passed=$((passed + 1))
fi
rm -rf "$OUT"

echo
echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
