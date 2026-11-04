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
IR (ir_lower.c) -> LLVM module (llvm_emit.c) -> optimized -> COFF in memory -> coff_read() -> pe_write() -> out.exe
```

`saad build main.sd --llvm` makes the program with LLVM, optimized like `-O2`. `-O0` to `-O3` give the level. The
compiler builds the module in memory with LLVM's C library, optimizes it with LLVM's passes (`default<O2>`) and makes
the object file in memory. Like the x64 backend, it needs nothing installed: LLVM is linked into saad, and the program
is made by the compiler's own PE writer, so no clang, linker or gcc is run.

* A vreg is an LLVM value. One used in another block than the one defining it lives in an alloca, which mem2reg makes a
  value again. A slot is an alloca, a data global is a packed struct of its bytes and a `ptr` for each relocation.
* Every function has the Win64 calling convention, like the x64 backend. The runtime functions are built by llvm_emit.c,
  and for Windows also `__chkstk` (touches each page of a big stack frame, so Windows grows the stack) and `_fltused`,
  which would come from the static library of the C runtime.
* `coff_read()` (object_writer.c) puts the code and data of LLVM's COFF object into the sections of an `AsmObject`, with
  its relocations, for `pe_write()`. The unwind tables (`.pdata`, `.xdata`) are left out, as the x64 backend has none.
* With `--elf`, LLVM writes `build/out.o` for the Linux harness instead.

The compiler has the LLVM backend when it is built with LLVM, which is then linked into it from LLVM's static libraries
(only the parts for x64, about 40 MB):

* `build.sh` finds LLVM with `llvm-config`, e.g after `apt install llvm-18-dev libzstd-dev`. When the static libraries
  can't be linked, it uses LLVM's shared library instead.
* `make.bat` uses `llvm-config` of a MinGW build of LLVM on the PATH, e.g of MSYS2 (`pacman -S mingw-w64-x86_64-llvm`).
* `SAAD_LLVM=0` builds without it. Then `--llvm` says that the compiler has no LLVM.

LLVM takes longer: about 8 seconds for `examples/blog/server.sd` at `-O2`, against 0.5 seconds for the x64 backend and
0.9 seconds at `-O0`.

## Debug information

`saad build main.sd --debug` makes the program with DWARF debug information, with both backends (with `--llvm` at `-O0`
unless an `-O` is given). A debugger (LLDB, e.g through CodeLLDB in VS Code, or GDB) can then stop at a line of a `.sd` file, step through the
lines, and show the calls with the names of the program, e.g `List<int>.add` or `json.Parser.parse`.

* Each IR function has its file and line (`IrFunction.source_file`, `source_line`) and its name in the program
  (`AstFunctionDefn.display_name`). Each IR instruction has the line of its statement.
* llvm_emit.c makes a DWARF subprogram for each function and gives each LLVM instruction its line. DWARF is used on
  Windows too (in the sections `.debug_*` of out.exe, like MinGW does), as LLDB and GDB read it there.
* Every program of the LLVM backend has unwind tables (`.pdata`, `.xdata`), so a debugger can walk the calls.
  `coff_read()` keeps them and the debug sections, and `pe_write()` writes them as sections of their own, with the
  exception directory pointing at `.pdata`, and a string table for the long names like `.debug_abbrev`.

The variables and parameters have debug information too:

* `declare_local()` (ir_lower.c) records each variable of the program (`IrFunction.locals`): its name, Saad type, slot,
  line and lexical scope. Each block, `for` and `catch` is a scope (`IrFunction.scopes`, and `IrInst.scope` for each
  instruction), so two loops can each have an `i`.
* llvm_emit.c makes a lexical block per scope and declares each variable on the alloca of its slot, with the DWARF type
  of its Saad type (`llvm_di_type`): structs with their members, pointers, fixed arrays, enums by name, and `string`,
  lists, slices, optionals, unions and `any` as the structs they are.
* `lsp/vscode/debug/saad_formatters.py` shows those like the program writes them: a string as `"hi"`, a list or slice as its
  elements, an optional as its value or `none`, a union as the variant it holds (`.label "hi"`), and an `any` with the
  type of its value (`int 7`). Load it with `command script import lsp/vscode/debug/saad_formatters.py`.
* The code after a block, e.g the step of a loop, is at the line of its `}`, so stepping goes through a loop like in C.
* `tools/linux/check_debug.sh` (part of `runtests.sh` when LLDB is installed) stops in `tests/debug/variables.sd` with
  LLDB and checks what it shows.

The x64 backend makes the same debug information itself, with dwarf.c:

* With `--debug`, x64_emit.c puts a label in the code where the line or the scope of the instructions changes, after the
  prologue and at the end of each function, and keeps the rbp offsets of the slots (`X64DebugFunction`).
* After x64_assemble(), the labels have their addresses, and `dwarf_write()` writes `.debug_abbrev`, `.debug_info`,
  `.debug_line` and `.debug_ranges` into the AsmObject: a subprogram per function, its variables at `rbp + offset`, a
  lexical block for each scope with variables, and the types like llvm_emit.c makes them. An address is a relocation to
  a label, and an offset into another debug section a section relative one, so `pe_write()` and `elf_write()` write them.
* For Windows it writes the unwind tables too. Every function starts with `push rbp; mov rbp, rsp`, so all of them have
  the same unwind information: rbp is the frame register.

The global variables don't have debug information yet.

### In VS Code

The VS Code extension (`lsp/vscode`) has the debug type `saad`. On F5 in a `.sd` file, `debug.js` builds the program
with `saad build --debug` (on Linux with `--elf`, linked with tools/linux/shim.c) and gives VS Code a configuration of
CodeLLDB (`vadimcn.vscode-lldb`, a dependency of the extension) to launch it, which loads
`lsp/vscode/debug/saad_formatters.py`. `tools/linux/check_vscode_debug.js` does the same without VS Code: it talks to
CodeLLDB's debug adapter over the Debug Adapter Protocol and checks the stops and the variables. `runtests.sh` runs it
when `SAAD_CODELLDB` is the folder of an unpacked CodeLLDB (the `extension` folder of `codelldb-linux-x64.vsix`).

## Testing

* `tools/linux/runtests.sh` runs the tests on Linux with `saad build --elf`, so every test runs the code of the assembler.
  `runtests.sh --llvm` and `dbtests.sh --llvm` run them with the LLVM backend.
* `tools/linux/asm_driver.c` assembles an out.asm with the compiler's assembler into a COFF, ELF or PE file. Its COFF
  output was compared with nasm's (`objdump -d -r`) for every test and example: the same instructions, except that the
  assembler makes some jumps short where nasm makes them long.
* With Wine, the programs that the PE writer makes were run against the ones that nasm and MinGW make, for every test: the
  same output and exit code. A `saad.exe` built with MinGW passes the Windows test runner (`tests/_test_runner.c`) under Wine.
