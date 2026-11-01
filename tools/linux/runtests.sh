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

for f in tests/*.sd tests/std/*.sd tests/backend/*.sd; do
    printf "%-56s " "$f"
    if ! output=$(./bin/saad build "$f" 2>&1); then
        echo "${RED}COMPILE ERROR${RESET}"
        echo "$output" | grep -iE 'error' | head -3 | sed 's/^/    /'
        failed=$((failed + 1))
        continue
    fi
    if output=$("$HERE/run.sh" "$(dirname "$f")/build/out.asm" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}RUN FAILED${RESET}"
        echo "$output" | tail -3 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

# A should_fail test can require the error output to contain some text with lines like:
#   // expect-error: Conflicting types for type parameter 'T'
for f in tests/should_fail/*.sd; do
    printf "%-56s " "$f"
    output=$(./bin/saad build "$f" 2>&1)
    code=$?
    output=$(printf '%s' "$output" | sed 's/\x1b\[[0-9;]*m//g') # Without colors
    missing=""
    while IFS= read -r expected; do
        if ! printf '%s' "$output" | grep -qF -- "$expected"; then
            missing="$expected"
            break
        fi
    done < <(sed -n 's|^[[:space:]]*// expect-error: ||p' "$f")
    # A crash (signal) is not the same as rejecting the program
    if [ $code -eq 0 ] || [ $code -ge 128 -a $code -ne 255 ]; then
        echo "${RED}FAILED (exit code $code)${RESET}"
        failed=$((failed + 1))
    elif [ -n "$missing" ]; then
        echo "${RED}FAILED (wrong error)${RESET}"
        echo "    expected the error to contain: $missing"
        echo "$output" | grep -E 'error' | head -3 | sed 's/^/    /'
        failed=$((failed + 1))
    elif ! ./bin/saad check "$f" --json | grep -q '"success": false, "diagnostics": \[$'; then
        # 'saad check --json' must report it too, as an editor reads that
        echo "${RED}FAILED (saad check --json didn't report it)${RESET}"
        failed=$((failed + 1))
    else
        echo "${GREEN}OK (rejected)${RESET}"
        passed=$((passed + 1))
    fi
done

# 'saad index' must know every name of tests/index.sd (needs python3)
if command -v python3 > /dev/null; then
    printf "%-56s " "saad index tests/index.sd"
    if output=$(./bin/saad index tests/index.sd | python3 "$HERE/check_index.py" tests/index.sd 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | head -5 | sed 's/^/    /'
        failed=$((failed + 1))
    fi

    # The language server (lsp/), talked to like an editor does
    printf "%-56s " "language server lsp/"
    if output=$(bash "$HERE/build_lsp.sh" 2>&1 > /dev/null && timeout 120 python3 "$HERE/check_lsp.py" "$REPO/build/linux/saad-lsp" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | grep -v '^[a-zA-Z/]*$' | head -10 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
fi

echo
echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
