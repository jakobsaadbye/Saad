# Running Saad programs on Linux

The x64 backend targets Windows (Win64 ABI, linked against msvcrt). These scripts let you
(or Claude) compile **and run** Saad programs on Linux anyway, which is handy for testing
the backend without a Windows machine.

```sh
./build.sh                                  # builds bin/saad
tools/linux/run.sh tests/structs.sd         # compile + run one program
tools/linux/runtests.sh                     # run the whole test suite
tools/linux/build_lsp.sh                    # build the language server (lsp/) as build/linux/saad-lsp
python3 tools/linux/check_lsp.py build/linux/saad-lsp   # talk to it like an editor
python3 tools/linux/check_recovery.py       # one error for a mistake, and the index still has the rest
python3 tools/linux/fuzz_recovery.py 1000 7 # 1000 broken programs (seed 7): no crash, always an index
```

Requirements: `gcc` (and `nasm` to run an out.asm).

## How it works

1. `bin/saad build file.sd --elf` assembles the program with the compiler's own assembler and writes it as an ELF
   object, `build/out.o`, instead of `build/out.exe` (Win64 code either way). With an out.asm,
   `nasm -felf64 --prefix ms_` does the same. In both, every
   global and extern symbol gets an `ms_` prefix (`main` becomes `ms_main`, `printf` becomes `ms_printf`, ...).
2. `shim.c` defines those `ms_` symbols as `__attribute__((ms_abi))` wrappers around the real
   libc functions. gcc handles the Win64 calling convention at the
   boundary, so the generated code runs unchanged. A real `main` calls `ms_main`.

Because gcc's `ms_abi` code relies on the Win64 rules (16 byte stack alignment, shadow space,
callee-saved registers), ABI mistakes in the backend usually show up as crashes here too.

## Limitations

- Only the C functions wrapped in `shim.c` can be called. A missing one shows up as a link
  error like `undefined reference to ms_sinf`: add a wrapper to `shim.c`.
- raylib and Windows APIs are not available.
- `printf` is reimplemented in the shim (it walks the format string and reads each argument
  from the Win64 `va_list`), so exotic format specifiers may not be supported.
