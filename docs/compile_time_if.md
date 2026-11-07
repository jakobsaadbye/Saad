# #if

`#if` keeps code for some targets only, e.g the bindings of an operating system:

```odin
#if OS == .windows {
    extern "ws2_32.dll" {
        closesocket :: (s: u64) -> i32;
    }
    closeSocket :: (s: u64) { closesocket(s); }
} else #if OS == .linux {
    extern "C" {
        close :: (fd: i32) -> i32;
    }
    closeSocket :: (s: u64) { close(s as i32); }
}
```

## The rules

* A condition compares `OS` (`.windows` or `.linux`) or `ARCH` (`.x64`) with `==` or `!=`, and can use `true`,
  `false`, `!`, `&&`, `||` and parentheses.
* `CHECK_MEMORY` is a condition of its own, true when the program is built with `--check-memory` (see
  `docs/check_memory.md`), e.g `#if CHECK_MEMORY { ... }` and `#if !CHECK_MEMORY { ... }`.
* `else { ... }` and `else #if ... { ... }` follow an `#if`.
* It can be at the top of a file and in a block. What a branch declares is in the scope around the `#if`, as if the
  braces weren't there: `closeSocket` above is a function of the file.
* A branch that doesn't hold isn't checked, so it can use names that only exist on its target. Its braces must still
  match.
* The target is `windows`, or `linux` with `saad build --elf`. `--os linux` or `--os windows` picks it for any
  command, e.g `saad index main.sd --os linux` for the language server.

## In the editor

Completion in a condition offers `OS` and `ARCH`, and their values after `OS == .` or `ARCH != .`. The VS Code
extension highlights `#if`, `OS` and `ARCH`. A branch that doesn't hold has no names for the language server, as it
isn't parsed: `--os` picks the target of `saad index`.

## How it works

The compiler works on the tokens of a file before it parses it (`apply_compile_time_ifs()` in parser.c): the tokens of
a branch that holds are kept, without the `#if`, its condition and its braces, and the other tokens are left out. A
condition with an error is false, so the rest of the file is still parsed.
