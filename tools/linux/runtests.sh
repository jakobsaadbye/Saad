#!/usr/bin/env bash
# Runs every test on Linux: tests/*.sd must compile and run with exit code 0,
# tests/should_fail/*.sd must be rejected by the compiler.
#
#   tools/linux/runtests.sh
#
# Requirements: gcc and nasm. Builds the compiler with ./build.sh first.

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
cd "$REPO"

bash ./build.sh > /dev/null || exit 1

RED=$'\e[31m'; GREEN=$'\e[32m'; RESET=$'\e[0m'
passed=0; failed=0

for f in tests/*.sd; do
    printf "%-56s " "$f"
    if ! output=$(./bin/saad build "$f" 2>&1); then
        echo "${RED}COMPILE ERROR${RESET}"
        echo "$output" | grep -iE 'error' | head -3 | sed 's/^/    /'
        failed=$((failed + 1))
        continue
    fi
    if output=$("$HERE/run.sh" "tests/build/out.asm" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}RUN FAILED${RESET}"
        echo "$output" | tail -3 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

for f in tests/should_fail/*.sd; do
    printf "%-56s " "$f"
    ./bin/saad build "$f" > /dev/null 2>&1
    code=$?
    # A crash (signal) is not the same as rejecting the program
    if [ $code -ne 0 ] && [ $code -lt 128 -o $code -eq 255 ]; then
        echo "${GREEN}OK (rejected)${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED (exit code $code)${RESET}"
        failed=$((failed + 1))
    fi
done

echo
echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
