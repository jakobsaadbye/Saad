# Saad

The [Saad language](https://github.com/jakobsaadbye/Saad) in VS Code: highlighting, errors as you type, go to
definition, find references, hover, outline, completion, signature help, rename and quick fixes for `.sd` files.

## The compiler and the language server

The extension starts the language server of Saad, which uses the compiler. Both come from the Saad repository:

```
saad build lsp/main.sd          # lsp/build/out.exe, the language server
```

* When the Saad repository is open, the compiler is `bin/saad.exe` and the server is `lsp/build/out.exe`. When the
  server isn't built yet, the extension asks to build it.
* For another project, set `saad.compilerPath` to the compiler, e.g `C:/dev/Saad/bin/saad.exe`. The server is found in
  the repository the compiler is in.

## Settings

* `saad.compilerPath`: the compiler. By default `bin/saad.exe` of the workspace, or `saad` on the PATH.
* `saad.serverPath`: the server. By default `lsp/build/out.exe` of the repository the compiler is in.
* `saad.trace.server`: `messages` shows what VS Code and the server send each other, in the Output panel.

## Commands

* `Saad: Restart the language server`
* `Saad: Build the language server`
