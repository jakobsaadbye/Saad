# The backend: from IR to out.exe

```
IR (ir_lower.c) -> registers (regalloc.c) -> assembly text (x64_emit.c) -> machine code (x64_asm.c) -> out.exe (object_writer.c)
```

The compiler makes the program itself. No assembler or linker has to be installed: `saad.exe` and `std/` are all it
needs, and `packages/` for the libraries of the standard library, e.g `raylib.dll`.

* **x64_emit.c** writes the assembly in NASM's syntax, to `build/out.asm`, which is kept for reading.
* **x64_asm.c** is an assembler for the part of NASM's syntax that x64_emit.c writes: the instructions it uses (about 45),
  the sections `.text`, `.data` and `.rdata`, labels, and `db`/`dd`/`dq`/`times`/`align`. Jumps to labels are made as short
  as they can be. A new instruction in x64_emit.c needs a case in `asm_encode()`; the assembler reports one it doesn't
  know with the line of out.asm.
* **object_writer.c** writes what the assembler made:
  * `pe_write()`: `build/out.exe`. Each function of a DLL is called through a thunk, `jmp [rip + slot]`, where slot is
    its entry of the import address table, which Windows fills in when it loads the program. The entry point calls `main`,
    then `exit()` of the C runtime with what main returns, so the C runtime's buffers are written. The program is a
    console program that is loaded at 0x140000000 (it has no relocations), with 8 MB of stack.
  * `elf_write()`: `build/out.o` for running the program on Linux (`saad build --elf`, see tools/linux).
  * `coff_write()`: an object file like `nasm -fwin64` writes, for comparing the two.

On the 2.5 seconds that nasm and gcc took for `lsp/main.sd`, the assembler and the PE writer take 0.04 seconds.

## Libraries

An extern block says the library of its functions. `extern "C"` is the C runtime (`msvcrt.dll`):

```odin
extern "C" {
    printf :: (format: c.string, args: ...any) -> int;
}

extern "kernel32.dll" {
    Sleep :: (dwMilliseconds: u32);
}

extern "raylib.dll" {
    InitWindow :: (width: int, height: int, title: c.string);
}
```

A DLL that isn't part of Windows is copied next to `out.exe` from `packages/<name>/lib/`, e.g
`packages/raylib/lib/raylib.dll`.

## The runtime

The functions that the generated code calls, `runtime_compare_strings` (comparing strings) and
`runtime_builtin_assert` (a failed `assert`), are written in assembly by x64_emit.c into the programs that use them, so
there is no runtime library to link.

## LLVM (optimized builds)

```
IR (ir_lower.c) -> LLVM module (llvm_emit.c, the C API of LLVM) -> optimized -> out.o / out.obj
```

`saad build main.sd --llvm` makes the program with LLVM, optimized like `-O2`. `-O0` to `-O3` give the level. The
compiler builds the module in memory with LLVM's C library, optimizes it with LLVM's passes (`default<O2>`) and writes the
object file itself, so no clang is run.

* A vreg is an LLVM value. One used in another block than the one defining it lives in an alloca, which mem2reg makes a
  value again. A slot is an alloca, a data global is a packed struct of its bytes and a `ptr` for each relocation.
* Every function has the Win64 calling convention, like the x64 backend. The runtime functions are built by llvm_emit.c.
* On Windows, LLVM writes `build/out.obj`, which gcc links into `build/out.exe` with the libraries of the externs.
  With `--elf`, it writes `build/out.o` for the Linux harness, like the x64 backend.

The compiler has the LLVM backend when it is built with LLVM: `build.sh` finds it with `llvm-config` (e.g
`apt install llvm-18-dev`), and `make.bat` in `C:\Program Files\LLVM` or `%LLVM_PATH%` (its `include\llvm-c` and
`lib\LLVM-C.lib`). Without it, `--llvm` says so. LLVM takes longer: about 8 seconds for `examples/blog/server.sd` at
`-O2`, against 0.5 seconds for the x64 backend and 0.9 seconds at `-O0`.

## Debugging

`saad build main.sd --debug` makes the program the old way, with `nasm -fwin64 -g` and `gcc`, so it has debug information
for gdb (see gdb_guide.md). That needs nasm and MinGW's gcc.

## Testing

* `tools/linux/runtests.sh` runs the tests on Linux with `saad build --elf`, so every test runs the code of the assembler.
  `runtests.sh --llvm` and `dbtests.sh --llvm` run them with the LLVM backend.
* `tools/linux/asm_driver.c` assembles an out.asm with the compiler's assembler into a COFF, ELF or PE file. Its COFF
  output was compared with nasm's (`objdump -d -r`) for every test and example: the same instructions, except that the
  assembler makes some jumps short where nasm makes them long.
* With Wine, the programs that the PE writer makes were run against the ones that nasm and MinGW make, for every test: the
  same output and exit code. A `saad.exe` built with MinGW passes the Windows test runner (`tests/_test_runner.c`) under Wine.
