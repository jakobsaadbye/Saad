# Editor support (LSP)

The goal is a language server written in Saad, in `lsp/` at the root of the repository, that any editor with an LSP
client can use (VS Code, Neovim, Helix, Zed, Emacs, Sublime). The compiler does the work: it knows where every name
is declared and used, so the server only runs it and looks things up in what it writes.

```
editor ── LSP (JSON-RPC over stdin/stdout) ──> lsp/ (Saad)
                                                 └─ runs: saad index main.sd --json --overlay <unsaved buffers>
                                                             → files, symbols, references, diagnostics
```

* LSP is JSON-RPC 2.0. Each message is `Content-Length: N\r\n\r\n` followed by N bytes of JSON. The editor starts
  the server as a child process and talks to it over its stdin and stdout. No HTTP or sockets are needed.
* VS Code needs a small extension that starts the server (`lsp/vscode/`). Other editors only need to be told the
  command.

## Phases

1. **Diagnostics as data** (done): `saad check main.sd --json`, and no `exit()` on errors.
2. **The index**: `saad index main.sd --json` with the declarations and uses of every name, for go to definition,
   find references, hover, the outline of a file and workspace symbols.
3. **The server** in `lsp/`, with `std/io` (bytes on stdin and stdout) and running a command and reading its output.
4. **Error recovery**: the typer goes on with the next declaration after an error, and the parser skips to the next
   top-level declaration after a syntax error, so the index stays useful while the code is being typed.
5. Later: completion, signature help, semantic tokens.

## saad check

`saad check main.sd` parses and type checks a program without compiling it. With `--json`, it writes the
diagnostics as JSON and nothing else:

```json
{"version": 1, "success": false, "diagnostics": [
  {"file": "/abs/main.sd",
   "range": {"start": {"line": 3, "character": 10}, "end": {"line": 3, "character": 11}},
   "severity": 1,
   "message": "'a' is used before its declared",
   "related": [{"file": "/abs/main.sd", "range": {...}, "message": "'a' is being declared here"}]}
]}
```

* The shape follows the `Diagnostic` of LSP. Lines and characters count from 0, and a character is a byte of the
  UTF-8 text (the server converts to UTF-16 for editors that want that).
* `severity` is 1 for an error and 2 for a warning. A note or a hint with a position is `related` information of the
  diagnostic before it. One without a position is added to its message.
* `success` is false when there are errors. The exit code is 0 without errors, otherwise 1.

In the compiler, every diagnostic goes through `report_error_helper()`, which adds it to `diagnostics` (lexer.h) and
prints it unless `diagnostics.print` is false. A syntax error that can't be returned from calls `stop_parsing()`,
which jumps back to `parse_file()`, so the compiler no longer exits in the middle of a file.
