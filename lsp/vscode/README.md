# Saad

The [Saad language](https://github.com/jakobsaadbye/Saad) in VS Code: highlighting, errors as you type, go to
definition, find references, hover, outline, completion, signature help, rename and quick fixes for `.sd` files. And
debugging of Saad programs: breakpoints, stepping, the calls and the variables.

## The compiler and the language server

The extension starts the language server of Saad, which uses the compiler. Both come from the Saad repository:

```
saad build lsp/main.sd          # lsp/build/out.exe, the language server
```

* When the Saad repository is open, the compiler is `bin/saad.exe` and the server is `lsp/build/out.exe`. When the
  server isn't built yet, the extension asks to build it.
* For another project, set `saad.compilerPath` to the compiler, e.g `C:/dev/Saad/bin/saad.exe`. The server is found in
  the repository the compiler is in.

## Debugging

Open a `.sd` file and press F5, or use the Debug button at the top of the editor. The extension builds the program
with debug information (`saad build --debug`) and debugs it with [CodeLLDB](https://github.com/vadimcn/codelldb),
which VS Code installs with this extension. Breakpoints go in `.sd` files, and the variables are shown like Saad writes
them: a string as `"hi"`, a list as its elements, an optional as its value or `none`, a union as the variant it holds.

For arguments or the LLVM backend, add a configuration to `.vscode/launch.json`:

```json
{
    "type": "saad",
    "request": "launch",
    "name": "Debug the Saad program",
    "program": "${workspaceFolder}/main.sd",
    "args": ["input.txt"],
    "backend": "llvm"
}
```

* `program`: the `.sd` file. `${file}` (the default) is the open file.
* `args`, `cwd`, `env`: of the program. It runs in the workspace folder by default.
* `backend`: `x64` (the default) or `llvm`, when the compiler is built with LLVM. `optimization`: the `-O` of LLVM.
* `stopOnEntry`: stop at the start of the program.

## Settings

* `saad.compilerPath`: the compiler. By default `bin/saad.exe` of the workspace, or `saad` on the PATH.
* `saad.serverPath`: the server. By default `lsp/build/out.exe` of the repository the compiler is in.
* `saad.trace.server`: `messages` shows what VS Code and the server send each other, in the Output panel.

## Commands

* `Saad: Restart the language server`
* `Saad: Build the language server`
* `Saad: Debug the file`
