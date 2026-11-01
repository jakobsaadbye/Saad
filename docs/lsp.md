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
2. **The index** (done): `saad index main.sd` with the declarations and uses of every name, for go to definition,
   find references, hover, the outline of a file and workspace symbols.
3. **The server** (done) in `lsp/`, with `std/io` (bytes on stdin and stdout), `os.output` (running a command and
   reading its output) and `os.args`. See `lsp/README.md`.
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

## saad index

`saad index main.sd` type checks a program and writes, as JSON, where every name is declared and used, and the
diagnostics of `saad check`:

```json
{"version": 1, "success": true,
 "symbols": [
   {"id": 3, "name": "area", "kind": "method", "file": 0,
    "nameRange": {"start": {"line": 17, "character": 4}, "end": {"line": 17, "character": 8}},
    "range": {...}, "detail": "method (self: *Circle) -> f32", "container": 2, "doc": "The area of the circle"}],
 "references": [
   {"file": 0, "range": {"start": {"line": 40, "character": 12}, "end": {...}}, "symbol": 3}],
 "files": ["/abs/main.sd", "/abs/std/fmt/print.sd"],
 "diagnostics": [...]}
```

* A **symbol** is a declaration. `kind` is one of `namespace`, `function`, `method`, `struct`, `union`, `enum`,
  `enumMember`, `field`, `variant`, `constant`, `global`, `variable`, `parameter`, `typeAlias` and `typeParameter`.
  `nameRange` is its name and `range` the whole declaration, e.g a function with its body. `detail` is its signature
  or type. `container` is the symbol it is in (the struct of a field or method, the enum of a value), or -1. `doc` is
  the `//` lines right above it, when there are any.
* A **reference** is a use of a symbol: go to definition is the symbol of the reference under the cursor, and find
  references is every reference of a symbol. A declaration is not a reference of itself.
* By default the index has the program's files, and the symbols of the standard library that the program uses.
  `--std` also writes the rest of the standard library.
* `--overlay path=file` reads `file` instead of `path`, for the text an editor hasn't saved. It can be given more
  than once.
* A name in a function that is never type checked isn't in the index, e.g in a generic function that is never called,
  or after an error in the same declaration (until phase 4).

In the compiler, the typer calls `index_identifier_use()` and friends (src/index.h) where it finds out what a name is,
e.g for an identifier, a member, an enum value, a type name or a named argument. A symbol is known by the file and the
offset of its name, so the copies the compiler makes of a declaration (for an import, a namespace or an instance of a
generic) are the same symbol. `tools/linux/check_index.py` checks that `saad index` knows every name of `tests/index.sd`.

## The server

`lsp/` is a program in Saad. It reads JSON-RPC messages with `std/io` and `std/json`, and answers them from the
index:

```
lsp/
  main.sd      The messages: initialize, the documents (open, change, save, close) and the questions
  rpc.sd       namespace rpc: reading and writing JSON-RPC messages (Content-Length + JSON)
  project.sd   namespace project: running 'saad index' and looking up the name at a position
  source.sd    namespace source: file URIs, lines, and UTF-8 bytes <-> UTF-16 units
  vscode/      The VS Code extension that starts the server
```

* **Projects.** A file is answered with the program it is part of: a program the server already indexed that has the
  file, else the `main.sd` of its folder or a folder above it (up to the workspace), else the file on its own. A
  project is indexed again when a document has changed since, and when a document is saved.
* **Unsaved text.** Every open document is written to a file in the temporary folder, and given to the compiler with
  `--overlay`. After a change, the program is checked again when no more messages are waiting (`io.hasInput()`), so
  typing doesn't queue up a check for every key.
* **Diagnostics** are sent for every file of the project with errors, and for the open files, so old errors go away.
* **Positions.** The compiler counts bytes and the editor UTF-16 units, so `source` converts each position with the
  line it is on (`positionEncoding` is `utf-16`).
* The server writes its log to stderr. VS Code shows it in the Output panel (Saad).

`tools/linux/check_lsp.py` talks to the server like an editor, and checks every feature on `tests/index.sd`.
