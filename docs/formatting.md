# Formatting: saad fmt

`saad fmt` formats Saad code, so that all code looks alike, whoever wrote it.

```
saad fmt .                 # every .sd file in this folder and the folders in it, but not build or hidden folders
saad fmt main.sd std/json  # files and folders
saad fmt --check .         # only list the files that aren't formatted, and fail if there are any, e.g in CI
saad fmt --stdout main.sd  # write the formatted file instead of changing it, e.g for an editor
```

It formats the whole repository (500 files) in a fifth of a second, and a file of 1500 lines in 7 ms, including the
start of the process, so an editor can format a file every time it is saved.

## In the editor

The language server formats a document when the editor asks (`textDocument/formatting`): it runs
`saad fmt --stdout` on the text the editor has, also when it isn't saved, and answers with one edit of the lines that
change, so the cursor stays where it is. The VS Code extension turns on `editor.formatOnSave` for Saad, so a file is
formatted every time it is saved, and Shift+Alt+F formats it any time. To turn it off:

```json
"[saad]": { "editor.formatOnSave": false }
```

Only the saved file is formatted. `saad fmt .` formats all of them, e.g once for a project that wasn't formatted.

## What it does

```odin
// Before
Point :: struct {
  x:int;
     y : int = -1;
}
scale :: method * (p: *Point,factor:int) {
p.x*=factor;
    if p.y>0&&!ok { p.y = p.y*factor+ -1; }
}

// After
Point :: struct {
    x: int;
    y: int = -1;
}
scale :: method * (p: *Point, factor: int) {
    p.x *= factor;
    if p.y > 0 && !ok { p.y = p.y * factor + -1; }
}
```

* **Lines stay where they are.** It never joins or breaks lines. Two blank lines or more become one, and blank lines
  after a `{` at the end of a line, before a `}` at the start of a line, and at the start and the end of the file go
  away. The file ends with a newline, and lines end without spaces.
* **Indentation** is 4 spaces for each level of brackets that were opened on the lines before (brackets opened on
  the same line count once, so `xs.map((u: User) -> string {` indents its body once). A line that goes on with the
  statement of the line before, as that one doesn't end with `;`, `{`, `}` or `,`, is one level deeper:

  ```odin
  total := price * count +
      shipping;
  ```

* **Spaces between tokens:** one around binary operators (`a + b`, `x := 1`, `a ?? b`, `-> int`, `=> 1`), one after
  `,`, `;` and the `:` of a declaration (`x: int`), none inside `()` and `[]`, none around `.`, `..` and `?.`, none
  after a unary operator (`-x`, `!ok`, `*p`, `&x`) or `@` and `#`.
* **Where the tokens alone don't say what it is, a space is kept if there was one**, and only one. These are `<` and
  `>`, which can be a comparison or type arguments (`a < b`, `List<int>`), unless they are next to a value like `0` or
  after `)` or `]`, which makes them a comparison. And `{` after a name, which can start a block or a struct literal
  (`if ok {`, `Point{1, 2}`), and the spaces just inside `{}` (`{ return x; }`, `{1, 2}`).
* **The members of a struct, and the fields of a struct literal on lines of their own, are always lined up**: in the
  lines one after the other at the same indentation, the types (or values) start in the same column, one space after
  the longest name. Comment lines don't end such a group, and lines that start with an attribute, e.g
  `@embed base: Base;`, are left as they are. Variables declared in a block, e.g `x: int;`, follow the next rule:

  ```odin
  User :: struct {
      id:        i64;
      name:      string;
      createdAt: time.Time;
  }

  ada := User{
      id:   1,
      name: "Ada",
  };
  ```

* **Other columns that were lined up stay lined up.** Lines one after the other that had the same separator in the same
  column are lined up with the longest of them, plus one space. Every other separator gets one space. The separators
  are the `:` of a declaration (lined up after it, at the type), `=`, `:=`, `+=` and the others, `::`, and the `=>` or
  `{` of the arms of a switch and of `if` and `else` on one line. Comment and attribute lines, the values of an arm with
  more values (`case 1,`) and lines whose separator wasn't lined up don't end such a group:

  ```odin
  Request :: struct {
      method:  string;
      // The path, e.g /posts
      path:    string;
      version: string;
  }
  ```

  A comment at the end of a line stays in its column, unless the code before it got too long. Then it moves right,
  with the comments of the lines around it that were in the same column.

## What it doesn't do

* It doesn't break long lines or join short ones, and it doesn't move comments to other lines.
* It doesn't line up what wasn't lined up, e.g arguments of calls on lines one after the other: those get one space.
* It doesn't format a file that can't be lexed, e.g with a string that doesn't end. It says so, and leaves the file as
  it is (`--stdout` writes the file as it was). Errors of names or types don't matter: it doesn't read the program as
  the compiler does.

## How it works

`src/format.c`. It works on the tokens of the lexer and the comments and blank lines between them, not on the AST:

1. The file is lexed, and the text between each two tokens is read for its comments, newlines and spaces
   (`format_scan()`), with the column each token and comment had.
2. The tokens are written into lines (`format_lines()`): a new line where the source had one, its indentation from a
   stack of the open brackets, and the space between two tokens from `spacing_between()`, which looks at the two
   tokens and, for `+ - * &`, whether the token before ends an operand (then it is binary).
3. `format_align()` lines up the separators and comments that were lined up.
4. The formatted text is lexed again, and its tokens are compared with those of the source. Only spaces and newlines
   change, so they must be the same. If they aren't, it is a bug of the formatter, and the file is left as it is.

As only the space between tokens changes, formatting can't change what a program does. `tools/linux/runtests.sh`
checks `tests/format/messy.input` against `messy.formatted`, and that formatting all the code of the repository twice
gives what formatting it once gives.
