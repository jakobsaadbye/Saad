<p align="center">
  <img src="docs/images/logo.svg" width="128" height="128" alt="The Saad logo: a blue S on a dark tile">
</p>

<h1 align="center">Saad</h1>

<p align="center">
  A small, fast, compiled language for fun: structs and generics, errors you can throw or take, lambdas, and a standard
  library with JSON, HTTP, a web framework and SQL.
</p>

<p align="center">
  <a href="#quick-start-on-windows">Quick start</a> ·
  <a href="examples/README.md">Lessons</a> ·
  <a href="docs/standard_library.md">Standard library</a> ·
  <a href="lsp/README.md">Editor support</a> ·
  <a href="CHANGELOG.md">Changelog</a>
</p>

```odin
import fmt;

Planet :: struct {
    name:  string;
    moons: int;
}

describe :: method (planet: Planet) {
    if planet.moons == 0 {
        fmt.print("% has no moons", planet.name);
    } else {
        fmt.print("% has % moons", planet.name, planet.moons);
    }
}

main :: () {
    planets := [Planet{"Mars", 2}, Planet{"Jupiter", 95}, Planet{"Venus", 0}];

    for planet in planets {
        planet.describe();
    }
}
```

```
$ saad run planets.sd
Mars has 2 moons
Jupiter has 95 moons
Venus has no moons
```

The compiler is written in C and makes programs on its own, with no assembler or linker to install: its own x64 backend,
or LLVM for optimized builds.

## Quick start on Windows

To build the compiler you need Git and gcc (MinGW-w64).

**1. Install Git and gcc.** In a terminal (PowerShell or cmd):

```
winget install --id Git.Git -e
winget install --id BrechtSanders.WinLibs.POSIX.UCRT -e
```

Open a new terminal afterwards, so they are on the PATH. `gcc --version` should print a version. (Any MinGW-w64 gcc
works, e.g from MSYS2, as long as `gcc` is on the PATH.)

**2. Get Saad and build the compiler:**

```
git clone https://github.com/jakobsaadbye/Saad.git
cd Saad
make.bat
```

This makes `bin\saad.exe`, optimized (`set SAAD_DEBUG=1` before `make.bat` builds it without optimizations, for
debugging the compiler). It finds the standard library in `std\` next to it, so leave it in the repository (or set
`SAAD_ROOT` to the folder of the repository).

**3. Put `saad` on the PATH**, so it works from any folder. In PowerShell, in the Saad folder:

```powershell
$path = [Environment]::GetEnvironmentVariable("Path", "User")
[Environment]::SetEnvironmentVariable("Path", "$path;$PWD\bin", "User")
```

Then open a new terminal.

**4. Write a program.** Save this as `hello.sd` in any folder:

```odin
import fmt;

main :: () {
    name := "Saad";
    fmt.print("Hello from %!", name);
}
```

and run it:

```
saad run hello.sd
```

It prints `Hello from Saad!`. The program is in `build\out.exe` next to `hello.sd`. More to try:

```
saad run examples\01_hello.sd                # the first of 28 small lessons, see examples\README.md
saad run examples\breakout\breakout.sd     # a game with raylib. raylib.dll is copied next to out.exe
```

**5. Editor support (optional).** The VS Code extension gives errors while typing, completion, hover and go to
definition. It needs Node.js (`winget install --id OpenJS.NodeJS.LTS -e`). See [lsp/README.md](lsp/README.md).

### Commands

```
saad run main.sd          # compile and run, in the current folder
saad run tool.sd -- a b   # arguments after -- are for the program (os.args())
saad build main.sd        # compile to build\out.exe
saad check main.sd        # only check for errors
saad build help           # the options of build, e.g --debug and --llvm
```

`saad build main.sd --debug` makes the program with debug information, so a debugger (LLDB, e.g CodeLLDB in VS Code,
or GDB) can stop at the lines of the program and show its variables (see `gdb_guide.md`). In VS Code, the extension
debugs the open `.sd` file on F5. `saad build main.sd --llvm` makes an optimized program with LLVM (`-O0` to `-O3`, `-O2` when not
given), when the compiler is built with LLVM. On Linux, `./build.sh` builds the compiler, and it makes Windows programs too. See `docs/backend.md`.

## Changelog

What changed in each version is in [CHANGELOG.md](CHANGELOG.md).
