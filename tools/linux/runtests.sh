#!/usr/bin/env bash
# Runs every test on Linux: tests/*.sd and the lessons examples/[0-9]*.sd must compile and run with exit code 0,
# tests/check_memory/*.sd too when they are built with --check-memory, and tests/should_fail/*.sd must be rejected by
# the compiler.
#
#   tools/linux/runtests.sh            with the x64 backend
#   tools/linux/runtests.sh --llvm     with the LLVM backend, also for the language server
#
# With SAAD_CODELLDB set to an unpacked CodeLLDB, it also debugs a program like the VS Code extension does.
#
# Requirements: gcc, and LLVM (e.g llvm-18-dev) for --llvm. Builds the compiler with ./build.sh first.

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
cd "$REPO"

export SAAD_BUILD_FLAGS=""
[ "$1" = "--llvm" ] && export SAAD_BUILD_FLAGS="--llvm"

bash ./build.sh > /dev/null || exit 1

RED=$'\e[31m'; GREEN=$'\e[32m'; RESET=$'\e[0m'
passed=0; failed=0

for f in tests/*.sd tests/std/*.sd tests/backend/*.sd examples/[0-9]*.sd; do
    printf "%-56s " "$f"
    if ! output=$(./bin/saad build "$f" --elf $SAAD_BUILD_FLAGS 2>&1); then
        echo "${RED}COMPILE ERROR${RESET}"
        echo "$output" | grep -iE 'error' | head -3 | sed 's/^/    /'
        failed=$((failed + 1))
        continue
    fi
    if output=$(bash "$HERE/run.sh" "$(dirname "$f")/build/out.o" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}RUN FAILED${RESET}"
        echo "$output" | tail -3 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
done

# tests/check_memory: built with --check-memory, which finds uses of the memory of arenas that were cleared or freed
for f in tests/check_memory/*.sd; do
    printf "%-56s " "$f"
    if ! output=$(./bin/saad build "$f" --elf --check-memory $SAAD_BUILD_FLAGS 2>&1); then
        echo "${RED}COMPILE ERROR${RESET}"
        echo "$output" | grep -iE 'error' | head -3 | sed 's/^/    /'
        failed=$((failed + 1))
        continue
    fi
    if output=$(bash "$HERE/run.sh" "$(dirname "$f")/build/out.o" 2>&1); then
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

    # After a mistake, the compiler reports it and goes on, so the index has the rest of the program
    printf "%-56s " "error recovery (check_recovery.py)"
    if output=$(python3 "$HERE/check_recovery.py" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | grep -A3 FAILED | head -10 | sed 's/^/    /'
        failed=$((failed + 1))
    fi

    # Broken programs don't crash the compiler (a fixed seed, so the run is the same each time)
    printf "%-56s " "error recovery (fuzz_recovery.py 150 cases)"
    if output=$(timeout 300 python3 "$HERE/fuzz_recovery.py" 150 1 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | tail -5 | sed 's/^/    /'
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

# https (TLS with OpenSSL) against a local openssl s_server (needs openssl and libssl-dev)
if command -v openssl > /dev/null && [ -f /usr/include/openssl/ssl.h ]; then
    printf "%-56s " "https (check_tls.sh)"
    if output=$(bash "$HERE/check_tls.sh" 2>&1); then
        echo "${GREEN}OK${RESET}"
        passed=$((passed + 1))
    else
        echo "${RED}FAILED${RESET}"
        echo "$output" | head -6 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
fi

# Debugging like the VS Code extension does, with CodeLLDB, when SAAD_CODELLDB is the folder of an unpacked CodeLLDB
# (the 'extension' folder of codelldb-linux-x64.vsix) and node is installed
if [ -n "$SAAD_CODELLDB" ] && command -v node > /dev/null; then
    for backend in "" "--llvm"; do
        if [ -n "$backend" ] && ! ./bin/saad build tests/debug/variables.sd --elf --llvm > /dev/null 2>&1; then continue; fi
        printf "%-56s " "VS Code debugging (check_vscode_debug.js $backend)"
        if output=$(timeout 180 node "$HERE/check_vscode_debug.js" "$SAAD_CODELLDB" $backend 2>&1); then
            echo "${GREEN}OK${RESET}"
            passed=$((passed + 1))
        else
            echo "${RED}FAILED${RESET}"
            echo "$output" | grep "FAILED" | head -5 | sed 's/^/    /'
            failed=$((failed + 1))
        fi
    done
fi

# The debug information of both backends, in LLDB (needs lldb, and a saad built with LLVM for the LLVM backend)
if command -v lldb > /dev/null; then
    for backend in "" "--llvm"; do
        if [ -n "$backend" ] && ! ./bin/saad build tests/debug/variables.sd --elf --llvm > /dev/null 2>&1; then continue; fi
        printf "%-56s " "debug information (check_debug.sh $backend)"
        if output=$(bash "$HERE/check_debug.sh" $backend 2>&1); then
            echo "${GREEN}OK${RESET}"
            passed=$((passed + 1))
        else
            echo "${RED}FAILED${RESET}"
            echo "$output" | grep "missing" | head -5 | sed 's/^/    /'
            failed=$((failed + 1))
        fi
    done
fi

echo
echo "$passed passed, $failed failed"
[ $failed -eq 0 ]
