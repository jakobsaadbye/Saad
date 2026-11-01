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
4. **Error recovery** (done): after an error, the parser and the typer go on with the next statement, so the index
   and the errors cover the whole program while the code is being typed. See "Error recovery" below.
5. **Completion, signature help and semantic tokens** (done): `saad complete` and the index. See below.
6. Later: rename, go to the definition of a type, code actions, formatting.

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
  or after an error in the same statement.

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

## Error recovery

One mistake gives one error, and the rest of the program is still checked and indexed.

* **Lexer.** An unknown character is reported and skipped, e.g `Unknown character 'ø'`.
* **Parser.** Before each statement of a file or a block, the parser saves its state. A syntax error jumps back to it
  (`stop_parsing()`), and `skip_failed_statement()` skips to the end of the statement: after its `;` (only braces are
  counted, so a missing `)` ends at the `;`), after the `}` of its block, or to the `}` of the block it is in. At the
  top of a file, it also stops at the next declaration at the start of a line. A declaration at the start of a line
  inside a block means its `}` is missing, e.g `This '{' has no '}'`, and the block ends there. Statements that only go
  in a function (`for`, `return`, ...) are an error at the top of a file. A block that is still open at the end of the
  file ends there (one error, "This '{' has no '}'"), so the function it is in is still checked. When the error is on a
  later line that starts like a statement (`p := ...`, `if ...`), the statement that failed ends at the line above, e.g
  `x := size(.` that is being written doesn't take the line below it.
* **Typer.** Before each statement, the typer saves its state (the scope, the function, the loop, ...). When the
  statement fails, the state is put back and the next statement is checked (`check_statement_and_recover()`). This is
  done in files and in blocks, but not in the instances of generics, which are checked once for each set of type
  arguments. A method whose signature has an error is skipped, and the other methods are still registered.
* **Poisoning.** The names of a statement that failed are *poisoned*, so using them fails without another error:
  * A declaration that failed gets `AST_FLAG_HAS_ERRORS`, and isn't checked again. Its names get
    `IDENTIFIER_IS_POISONED`, except a function that has a type, which can still be called (an error in its body
    doesn't stop its callers).
  * The names of a statement with a syntax error are taken out of the scope, as their declaration isn't whole, and put
    in `parser->poisoned_names`. "Undeclared identifier", "Unknown type", "is not a member of" and "is not declared in
    the namespace" are not reported for those names (`is_poisoned_name()`).
  * A struct that failed to check is reported once, and the structs that need it fail without an error. The uses of a
    member whose type has an error fail without an error.
* A file with errors still gives its declarations to the files that import it. The program fails when any error was
  reported (`error_count()` in compiler.c), so code is never generated from a program with errors.

`tools/linux/check_recovery.py` makes typical mistakes in `tests/index.sd` and checks that there is one error and the
names after it are still in the index. `tools/linux/fuzz_recovery.py` breaks real programs in random ways (deleting a
`}` or `;`, renaming names, cutting files off, junk) and checks that the compiler never crashes and always writes an
index. Both run in `tools/linux/runtests.sh`.

## saad complete

`saad complete main.sd --at file:line:character` writes what can be written at a position, and the signature of the
call it is in. The line and the character count from 0, and the character is a byte of the line. It takes `--overlay`
like `saad index`.

```json
{"version": 1,
 "items": [
  {"label": "radius", "kind": "field", "detail": "f32"},
  {"label": "area", "kind": "method", "detail": "method (self: *Circle) -> f32", "doc": "The area of the circle"}],
 "signature": {"label": "grow(by: f32)", "parameters": ["by: f32"], "active": 0}}
```

The program is checked first, and, as the compiler goes on after errors (phase 4), the code around the position is
checked even when the line that is being written isn't whole. Then the text before the position says what is wanted
(src/complete.c):

* After a `.`, the chain before it (`a.b[0].c().`) is followed from the scope at the position: a namespace gives its
  declarations (the private ones only in the same namespace), a type by its name its static members (the values of an
  enum, the variants of a union), and a value the members of its type: fields and methods, the methods of the generic
  struct that aren't made yet for an instance, the methods of strings, numbers and enums, and `count`, `data` and the
  methods of slices. A pointer is followed to what it points to.
* A `.` with nothing before it is a value of the type that is wanted there, so it gives the values of an enum or the
  variants of a union. The type comes from what is before it: `a == .` and `a != .` (the type of `a`), `a = .` and
  `a: Status = .` (the type of the place), `case .` (the type of what the switch is on), `f(x, .` (the type of the
  parameter), `return .` (the return type of the function) and `Point{kind: .` (the type of the field).
* In a struct literal, e.g `Point{x: 1, |`, the fields that aren't written yet (the variants for a union), with
  `insert` set to e.g `"y: "`. After `name:`, a value is written, so it is the names in scope.
* Otherwise, the names in scope: the locals declared above the position and the parameters, the declarations and
  imports of the file, and the keywords.
* In a call, `signature` is the function of the last `(` that isn't closed, and `active` the argument at the
  position. A method called on a value doesn't show its receiver.

The scope at a position is the innermost block that has it (`file->blocks`). Until its `}` is parsed, a block goes to
the end of the file, so a block that is being written still has a scope.

## Semantic tokens

The server colors the names of a file by what they are, from the index: every declaration and reference of the file is
a token, of the type of its symbol (`namespace`, `type`, `struct`, `enum`, `enumMember`, `function`, `method`,
`property`, `variable`, `parameter` and `typeParameter`). A declaration has the modifier `declaration`, and a constant
or an enum value `readonly`. Keywords, strings, numbers and comments are left to the grammar of the editor.
