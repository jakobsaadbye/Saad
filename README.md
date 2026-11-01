# Saad Compiler

A just for fun compiler

## Quick start on Windows

Saad compiles programs on its own: once the compiler is built, it needs no assembler or linker. To build the compiler
you need Git and gcc (MinGW-w64).

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

This makes `bin\saad.exe`. It finds the standard library in `std\` next to it, so leave it in the repository (or set
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
saad run examples\fibonacci.sd
saad run examples\breakout\breakout.sd     # a game with raylib. raylib.dll is copied next to out.exe
```

**5. Editor support (optional).** The VS Code extension gives errors while typing, completion, hover and go to
definition. It needs Node.js (`winget install --id OpenJS.NodeJS.LTS -e`). See [lsp/README.md](lsp/README.md).

### Commands

```
saad run main.sd          # compile and run
saad build main.sd        # compile to build\out.exe
saad check main.sd        # only check for errors
```

`saad build main.sd --debug` makes the program with nasm and gcc instead, with debug information for gdb (see
`gdb_guide.md`). On Linux, `./build.sh` builds the compiler, and it makes Windows programs too. See `docs/backend.md`.

# Changelog

## Update v0.9.3 - 1. Nov 2026

* Type arguments can be given at a call. A type parameter that is only in the return type is given that way

```odin
x := max<f64>(1, 2.5);
users, err := conn.query<User>("SELECT * FROM users WHERE age > $1", 30);
person, err2 := json.decode<Person>(text);
names := users.map<string>(nameOf);   // a method gets its own type parameters, not those of the receiver
```

* Methods can have type parameters of their own, also inside a generic struct

```odin
Box :: struct <T> {
    value: T;
    convert :: method <U> (f: (T) -> U) -> Box<U> { ... }
}
```

* `map`, `filter`, `reduce`, `find`, `findIndex`, `some`, `every` and `countWhere` on slices, arrays and lists. The type
  of a function argument is inferred, e.g `users.map(nameOf)` is a `[..]string`, and `map` and `filter` take an allocator

```odin
adults := users.filter(isAdult).map((u: User) -> string { return u.name; });
total  := prices.reduce(0.0, (sum: f64, p: f64) -> f64 { return sum + p; });
```

* A function type can leave out the names of its parameters: `(T) -> U`, `(int, string) -> bool`
* Changed: `conn.query<User>(sql)` gives the rows and `conn.queryOne<T>(sql)` gives the row, a found flag and the error,
  instead of filling a pointer. `json.decode<T>(text)` gives the value; `json.decodeInto(text, &value)` is the old
  `json.decode(text, &value)`
* Fix: a function literal checked after a call to a function that wasn't checked yet was taken for a global variable
  ("The start value of a global variable must be known when compiling")

## Update v0.9.2 - 1. Nov 2026

* `std/db`: run SQL against Postgres and read the rows into plain structs. Columns are matched to members by name
  (`createdAt` reads `created_at`), `NULL` columns go into pointers, and the parameters `$1, $2, ...` are sent
  apart from the SQL. See `docs/db.md`, which also has the plan for a generator and a typed query builder

```odin
import db;
import db.postgres;

conn, err := postgres.connect("host=localhost dbname=shop user=postgres");
users: [..]User;
err = conn.query(&users, "SELECT * FROM users WHERE age > $1", 30);
changed, err2 := conn.exec("DELETE FROM users WHERE id = $1", 7);
```

* The memory of the rows comes from `conn.allocator`, e.g an arena that frees a whole request at once. `mem.Arena`
  is ready to use as a zero value and gives an `Allocator` with `arena.allocator()`
* `db` doesn't know Postgres: a database is a driver, a struct of functions, so `std/db` can move to a repository
  of its own later
* Fixes: an unterminated string at the end of a file crashed the compiler, errors after a string over several lines
  had the wrong line, and `&p[-8]` (a negative index that is a literal) read the wrong memory

## Update v0.9.1 - 1. Nov 2026

* Characters: `'a'`, `'\n'`, `'\''` and `'\x41'` are the number of the byte. Where a number type is wanted they become it,
  e.g `ch == ' '` for a `u8`, and otherwise they are a `char`, which prints as the character

```odin
isDigit :: (c: u8) -> bool { return c >= '0' && c <= '9'; }

value := (ch - '0') as int;
switch c {
    case 'a'...'z' { ... }
    case '\n'     { ... }
}
```

* The StringBuilder is ready to use without `newStringBuilder`, and has `addLine`, `addRepeated`, `removeLast`,
  `endsWith`, `length` and `isEmpty`. `%%` in a format is a `%`. `fmt.join(parts, ", ")` joins strings.
  `addAnyValue` is now `addValue`. See `docs/standard_library.md`
* Fixes: `sb.clear()` didn't empty the text, `sb.add()` leaked a list on every call, and a variadic parameter that got
  no arguments, as the last parameter, had a garbage count
* A char in a struct prints as `'a'`, so it can be copied back into a program
* The standard library and the language server use characters instead of tables of constants like `Quote :: 34;`

## Update v0.9.0 - 1. Nov 2026

* The compiler makes `out.exe` itself, with its own assembler and its own writer of Windows programs. nasm and gcc are no
  longer needed to compile a program, and they were the slow part: `lsp/main.sd` compiles in 0.18 seconds instead of
  2.6, where assembling and linking went from 2.5 seconds to 0.04. See `docs/backend.md`
* `saad build --debug` makes the program with nasm and gcc as before, with debug information for gdb
* An extern block names the DLL of its functions, e.g `extern "kernel32.dll" { ... }` and `extern "raylib.dll" { ... }`.
  `extern "C"` is the C runtime. A DLL of `packages/` is copied next to `out.exe`, e.g raylib.dll
* The runtime functions (comparing strings, failed asserts) are part of the program, so there is no runtime library
* The compiler can make a Windows program on Linux too

## Update v0.8.6 - 1. Nov 2026

* `fmt` prints values as they are written in Saad, so they can be copied back into a program:
  `Person{name: "Ada", age: 36}` instead of `Person{ name = Ada, age = 36 }`. Strings in structs, arrays and unions
  get quotes, and a union is printed like a literal of it, e.g `Shape{circle: Circle{radius: 2.000000}}`
* The language server works in the files of the standard library, e.g `std/json/encode.sd`. A file of the standard
  library is checked on its own, with its namespace
* The signature of a function shows a parameter with a default value as it is written, e.g
  `encode :: <T>(value: T, options := Options{})` instead of `options: ?`, and `newline: bool = true`
* A file's imports are checked before the other files of its namespace, which can use its declarations
* Fix: `std/mem.sd` used a variable in its own declaration

## Update v0.8.5 - 1. Nov 2026

* Completion of `.open` where the type is known: after `status == .`, `x = .`, `x: Status = .`, `case .`, `f(.` and
  `return .`, it gives the values of the enum or the variants of the union
* Completion of the fields of a struct literal that aren't written yet, e.g `Point{x: 1, |` gives `y`
* A block that is still open at the end of a file no longer makes the function it is in fail to check, and a line
  that is being written no longer takes the line below it into its error

## Update v0.8.4 - 1. Nov 2026

* The language server completes names: the fields and methods after `c.`, the declarations of a namespace after
  `fmt.`, the values of an enum after `Status.`, and the names in scope. It shows the signature of the call that is
  being written, and colors the names of the program by what they are (semantic tokens)
* `saad complete main.sd --at file:line:character` writes what can be written at a position, as JSON for an editor.
  See `docs/lsp.md`

## Update v0.8.3 - 1. Nov 2026

* The compiler goes on after an error. A statement with a syntax error is skipped, and so is one that fails to type
  check, and the next one is checked. So `saad build` reports the errors of the whole program at once, and the
  language server can still go to definition, find references and hover in code with errors
* One mistake gives one error: the names of a declaration with an error don't give more errors where they are used
* An unknown character is skipped, e.g `Unknown character 'ø'`, and a missing `}` is found at the next declaration,
  e.g `This '{' has no '}'`
* Fixes: `a := ;` is an error, a `for` or `return` at the top of a file is an error instead of a crash, returning too
  many values no longer crashes the compiler, and a long list of errors no longer overflows a buffer

## Update v0.8.2 - 1. Nov 2026

* A language server for Saad, written in Saad, in `lsp/`: errors while typing (also of unsaved text), go to
  definition, find references, hover, the outline of a file and workspace symbols. `lsp/vscode` is the extension for
  VS Code, with highlighting. Any editor with an LSP client can use it, see `lsp/README.md`
* `std/io`: bytes on stdin, stdout and stderr (`io.readLine`, `io.read`, `io.write`, `io.writeError`, `io.hasInput`)
* `os.output(command)` runs a command and returns what it wrote and its exit code. `os.args()` is the command line
* A function can use a constant that is declared below it, e.g `isLetter :: (c: u8) -> bool { return c >= A; }`
  above `A :: 65;`
* An untyped constant can be assigned to a smaller number type when it fits, e.g `bytes[0] = Quote;` for a `u8`, and
  one that doesn't fit is an error
* A type of a namespace can be a type argument, e.g `Map<string, fmt.StringBuilder>`
* A function of a program can be called like a C function the standard library uses, e.g `exit` or `malloc`. Before,
  the standard library called the program's function instead
* `saad index` gives a function the range of its body, and a variable at the top of a file is a `global`

## Update v0.8.1 - 1. Nov 2026

* `saad check main.sd` type checks a program without compiling it. With `--json` it writes the errors and warnings as
  JSON, for an editor. It is the first step to a language server, see `docs/lsp.md`
* The compiler no longer exits in the middle of a file on a syntax error
* `saad index main.sd` writes where every name is declared and used, with types and doc comments, as JSON for an
  editor. `--overlay path=file` reads the unsaved text of an editor. See `docs/lsp.md`

## Update v0.8.0 - 1. Nov 2026

* Global variables

```odin
count := 0;
names: [..]string;
origin: Point;           // with the default values of Point

@private
cache: [64]int;          // only for this namespace

bump :: () { count += 1; }
```

A global variable starts with a value that is known when compiling (literals, enum values and constants), or with
the default value of its type. Any function can use it, also one above it.

* The random library is named like Python's

```odin
import random;

dice := random.between(1, 6);   // both included
name := random.choice(names);
random.shuffle(cards);
picks := random.sample(names, 2);

rng := random.newRandom(42);    // repeatable, with the same methods
rng.between(1, 6);
```

* Generic methods of a struct that isn't generic, e.g `choice :: method <T>(r: *Random, xs: []T) -> T` called as `rng.choice(xs)`
* BREAKING: `rng.nextInt(min, max)` is `rng.between(min, max - 1)`, `nextFloat` is `random`, `next` is `bits`,
  `nextBool()` is `chance(0.5)`, and `random.shuffle(&rng, xs)` is `rng.shuffle(xs)` or `random.shuffle(xs)`

## Update v0.7.1 - 1. Nov 2026

* The standard library and breakout use switch where it reads better, e.g the kinds of types in `fmt` and
  `std/json`, the escapes and characters of the JSON parser, and the pickups of breakout
* A function doesn't need a return at its end when every way through it returns: an `if` with an `else`, or a
  switch with an `else` or one that handles every value of an enum or union, where every branch returns
* A case of a switch expression can give an untyped number to a switch of any number type, e.g `=> Minus;` for a u8

## Update v0.7.0 - 31. Oct 2026

* switch

```odin
switch status {
    case .open   { ... }
    case .closed
    case .locked { ... }
}

switch score {
    case 90...100 { ... }
    case 0        { ... }
    else          { ... }
}

choice := switch x {
    case 1 => "icecream";
    case 2 => "coffee";
    else   => "mango";
};
```

A switch on an enum or union without an `else` must handle every value. There is no fallthrough. A switch
always has a value to switch on, as `if` and `else` are for conditions.

* Tagged unions

```odin
Shape :: union {
    circle: Circle;
    rect:   struct { w, h: f32; };
    empty;
}

area :: (s: Shape) -> f32 {
    switch v in s {
        case .circle { return Pi * v.radius * v.radius; }   // v is the Circle
        case .rect   { return v.w * v.h; }
        case .empty  { return 0; }
    }
    return 0;
}

Option :: union <T> { none; some: T; }
```

`shape.kind` is which variant a union holds, as an enum: `if shape.kind == .circle { ... }`.
See `docs/switch_and_unions.md`. Unions are printed by `fmt` and read and written by `std/json`.

## Update v0.6.1 - 31. Oct 2026

* Options for `json.encode` and `json.decode`: indentation, the style of member names and enum values
  (camelCase, snake_case, kebab-case, ...), enums as numbers, omitEmpty for every member and strict decoding

```odin
json.encode(person, json.Pretty);
json.decode(text, &person, json.SnakeCase);
json.encode(person, json.Options{indent: 4, names: .snakeCase, omitEmpty: true});
```

* The default value of a struct member can use an enum that is declared after the struct, or in another file
* BREAKING: `json.encode(value, indent: 2)` is `json.encode(value, json.Pretty)`, and `v.toJson(indent: 2)` is `v.toJson(json.Pretty)`

## Update v0.6.0 - 31. Oct 2026

* Custom attributes and a JSON library

A struct marked with `@attribute` is an attribute. Its values go on struct members and are read through
reflection. `std/json` uses one to name and skip members. See `docs/reflection.md` and `docs/standard_library.md`.

```odin
import json;

Person :: struct {
    @json.Field(name: "first_name")
    firstName: string;
    age:       int = 18;

    @json.Field(skip: true)
    cache: *void;
}

main :: () {
    text := json.encode(Person{firstName: "Ada", age: 36});   // {"first_name":"Ada","age":36}

    p: Person;
    err := json.decode(text, &p);       // "" when it worked

    value, parseErr := json.parse("{\"langs\": [\"en\", \"da\"]}");
    first := value.get("langs").at(0).asString();
}
```

* Reflection descriptors are constants made by the compiler, one per type. Enums have all their values,
  struct members have their offset and attributes, and types that refer to themselves work.
* `sort`, `isSorted`, `reverse`, `indexOf`, `contains` and `swap` are methods on every slice, array and list,
  from the prelude: `xs.sort()`. Any generic method on `[]T` is one, e.g `sum :: method <T>(xs: []T) -> T`.
* `fmt.sprint` returns the formatted text, and `string.free()` frees it.
* Style: functions are camelCase (`newRandom`), enum values start lowercase (`Status.open`, `TypeKind.int`)
  and constants are PascalCase (`MaxIntValue`). Keywords can be enum values. See `docs/style.md`.
* Strings can have escaped quotes: `"say \"hi\""`. `_` can be used more than once in a declaration.
* BREAKING: The `slices` namespace is gone, its functions are methods now. `random.NewRandom` is
  `random.newRandom`, `fmt.NewStringBuilder` is `fmt.newStringBuilder` and `mem.NewArena` is `mem.newArena`.
  The enum values of `runtime.sd` start lowercase, e.g `TypeKind.struct`. `StructMember.value` and
  `TypeEnum.valueName` are replaced by `member.valueIn(data)` and `type.nameOf(value)`.

## Update v0.5.2 - 31. Oct 2026

* BREAKING: The builtin `print` is removed. Use `fmt.print`, which was called `fmt.Print`. It adds a newline,
  unless `newline: false` is given.

```odin
use fmt;

main :: () {
    print("% + % = %", 1, 2, 3);
    print("no newline", newline: false);
}
```

* The compound assignment operators `%=`, `^=`, `|=`, `&=`, `<<=` and `>>=`
* `fmt.print` prints the smallest `i64` correctly
* The tests of the standard library are in `tests/std/` and the tests of the backend in `tests/backend/`

## Update v0.5.1 - 31. Oct 2026

* The standard library is sorted into folders, so the top of `std/` stays clean. See `docs/standard_library.md`.

```
std/
  prelude/   runtime, List/Map and the string methods. Always there, without an import
  c/         namespace c: the C bindings
  fmt/       namespace fmt
  os/        namespace os, and os.windows for the Windows API
  vendor/    raylib
  math.sd  mem.sd  random.sd  slices.sd  time.sd
```

* New namespaces: `os` (files, environment variables, running commands), `time` (clocks, stopwatches,
  sleep), `random` (a seeded generator) and `slices` (generic sort, reverse, indexOf, contains)

```odin
import os;
import random;
import slices;

main :: () {
    text, ok := os.readFile("names.txt");
    names := text.split("\n");
    slices.sort(names);

    rng := random.NewRandom(42);
    random.shuffle(&rng, names);
    dice := rng.nextInt(1, 7);
}
```

* BREAKING: `printf` and `sprintf` moved from `fmt` to `c`, and the namespace `windows` is now `os.windows`.
  `import "fmt";` of a namespace of the standard library now means `use fmt;` (still with a warning).

## Update v0.5.0 - 31. Oct 2026

* Namespaces

A file can declare its namespace at the top. Any number of files, in any folders, can be in the same
namespace. See `docs/namespaces.md`.

```odin
// physics/body.sd
namespace game.physics;

Body :: struct { mass: f64; }
```

```odin
use game.physics;           // every public declaration
import fmt;                 // fmt.Print("%", x)
import math as m;           // m.sqrt(2.0)
import math.sin;            // a single declaration
import math.cos as cosine;

main :: () {
    b := Body{mass: 2.0};
    fmt.Print("% %", m.sqrt(b.mass), sin(0.0) + cosine(0.0));
}
```

The standard library is split into the top-level namespaces `fmt`, `math`, `mem`, `c` (the C bindings),
`raylib` and `windows`. `List`, `Map` and `string` are still always available.

* Attributes: `@private` and `@deprecated`

```odin
@private
helper :: () -> int { ... }   // Only visible inside its namespace

@deprecated("use add")
push :: method * (value: T) { ... }
```

* BREAKING: `import "fmt";` is deprecated in favour of `use fmt;` or `import fmt;`, and `std/arrays.sd`
  (the deprecated `appendInt`) is removed. `#` is reserved for compile-time features, so the old `#import`
  and `#extern` directives are gone. Use `extern "C" { ... }` blocks for C functions.

## Update v0.4.3 - 30. Oct 2026

* BREAKING: Struct literals and named arguments use `:` instead of `=`

```odin
Person :: struct {
    name: string;
    age:  int;
}

p := Person{name: "ada", age: 18};

greet :: (name: string, times := 1) {}
greet("ada", times: 3);
```

It matches the `key: value` of map literals, and a struct declaration can be copied into a struct literal by
replacing the types with values. The old `name = value` is an error that points to the new syntax.

## Update v0.4.2 - 30. Oct 2026

* Default values for struct members

```odin
Person :: struct {
    name:  string = "anon";
    age:   int    = 30;
    tags   := 3;        // The type comes from the default value
    pos:   Vec2;        // Gets the defaults of Vec2
}

p: Person;                 // name = "anon", age = 30, tags = 3
q := Person{name: "ada"};  // age = 30, tags = 3
```

Defaults are used when a struct is declared without a value, and for the members a struct literal leaves
out. They also work for anonymous structs, where the defaults are a part of the type, and for generic
structs, where `value: T = 1` takes the type of each instance. A default can be a literal, a constant, an
enum value or a struct literal, but can't call a function yet.

## Update v0.4.1 - 30. Oct 2026

* Anonymous structs and enums

A struct or enum type can be written inline without a name. In an expression, a struct literal or an
enumerator right after it makes a value:

```odin
person := struct {
    name: string;
    age:  int;
} {"ada", 36};

status := enum { open, closed }.closed;

older :: (p: struct { name: string; age: int; }) -> int {
    return p.age + 1;
}
```

Two anonymous structs or enums with the same members are the same type, so `person` can be passed to
`older`. Their name is their members, e.g `struct { name: string; age: int; }`. Anonymous structs can't
have constants or methods; give the struct a name for those.

## Update v0.4.0 - 30. Oct 2026

* Collections in the standard library

Dynamic arrays and maps are now written in Saad, in `std/collections.sd`. See `docs/collections.md`.

```odin
names: [..]string = ["ada", "bob"];
names.add("cy");
names.insertAt(0, "zed");
names.removeAt(0);
print("% %\n", names.first(), names.last());

ages := ["alice": 31, "bob": 27];   // Map<string, int>
ages["carol"] = 40;
counts: Map<string, int> = [:];
counts["word"] += 1;
for name, age in ages { print("% is %\n", name, age); }
```

`[..]T` is another way to write `List<T>`. The syntax comes from a collection protocol that any struct
can use: `c[k]` calls `index` or `indexRef`, `for x in c` and passing `c` as a slice call `elements`, and
literals call `add` or `set`. Maps keep their entries in the order they were added. `for key, value in m`
works for any collection with a `set` method whose elements have `key` and `value` members.

* BREAKING: `append(xs, v)` is replaced by `xs.add(v)`
* Methods on primitive types and enums, e.g `doubled :: method (x: int) -> int`
* Structs can be compared with `==` and `!=`, member by member
* Arguments and return values can now widen, e.g pass an `i32` where an `i64` is wanted

## Update v0.3.1 - 29. Oct 2026

* BREAKING: The signed integer types are renamed from `s8`, `s16`, `s32`, `s64` to `i8`, `i16`, `i32`, `i64`

```odin
a: i32 = -5;
b := 100 as i64;
```

`int` is still an alias for `i32`.

* Type aliases

A constant declaration whose value is a type declares a type alias. It works for every kind of type,
including instances of generic structs:

```odin
Byte     :: u8;
IntPtr   :: *int;
Ints     :: []int;
IntList  :: List<int>;
Hue      :: Color;
Callback :: (x: int) -> int;

main :: () {
    b: Byte = 255;
    numbers := IntList{};
    numbers.push(1);    // Methods of List<int> work through the alias
    h: Hue = .Red;
}
```

An alias is the same type as the type it names, so an `IntList` can be passed where a `List<int>` is wanted,
and `typeof` gives the name of the aliased type. Aliases can be used before they are declared.

## Update v0.3.0 - 29. Oct 2026

* Generic functions

Functions can take type parameters, written in angle brackets after `::`. The types are inferred from the
arguments of each call, and each distinct set of types gets its own copy of the function.

```odin
max :: <T>(elements: ...T) -> T {
    best := elements[0];
    for e in elements {
        if e > best { best = e; }
    }
    return best;
}

main :: () {
    a := max(3, 9, 4);   // T = int
    b := max(1, 2.5);    // T = float, a float literal wins over an integer literal
    x: i64 = 1;
    c := max(x, 10);     // T = i64, literals take the type of a typed argument
}
```

Typed arguments must agree: `max(a, b)` with an `i32` and an `i64` is an error, nothing is widened.
Every type parameter has to be used by a parameter, as there is no syntax to give the types explicitly yet.

* Methods on generic structs

Methods declared outside a struct declare the type parameters of the receiver:

```odin
set :: method <K, V>(m: *Map<K, V>, key: K, value: V) {
    append(m.keys, key);
    append(m.values, value);
}
```

The receiver must use each type parameter exactly once, e.g `Map<K, V>`. `Map<string, V>` is an error, as methods
cannot be specialized for some types.

* Methods declared inside structs

Methods can be declared inside a struct, where they get an implicit `self`. `method` gives a read-only `self`,
and `method *` a `self` that can be changed:

```odin
Map :: struct <K, V> {
    keys:   [..]K;
    values: [..]V;

    get :: method (key: K) -> V {
        for i in 0..self.keys.count {
            if self.keys[i] == key { return self.values[i]; }
        }
        empty: V;
        return empty;
    }

    set :: method * (key: K, value: V) {
        append(self.keys, key);
        append(self.values, value);
    }
}
```

Members are only reached through `self`, e.g `self.keys`. Methods don't change the size or members of the struct.

In a `method`, nothing in `self` can be changed: assigning to `self.count`, taking `&self.pos`, calling a
`method *` on `self` and `for *` over a fixed size array member are errors. Copy it into a local first if you
want to change a copy (`copy := self.pos`). Read-only is shallow: what the members point to can still be changed,
e.g the elements of a dynamic array member.

* Methods can now be called before they are declared, and on values that are not variables, e.g `make_map().set(k, v)`
  (the value is put in a temporary, so the change is lost)

Bug fixes:

* Fixed untyped literals passed as varargs not taking the type of the variadic parameter e.g `f(a, 100)` with `f :: (xs: ...i64)`
* Fixed a struct declared after a variable of its type not having its members resolved

## Update v0.2.9 - 29. Oct 2026

* Generic structs

Structs can now take type parameters, written in angle brackets after `struct`

```odin
List :: struct <T> {
    elements: [..]T;
    count: int;
}

Pair :: struct <A, B> {
    first:  A;
    second: B;
}

main :: () {
    numbers: List<int>;
    append(numbers.elements, 1);

    pair  := Pair<int, float>{1, 2.5};
    pairs := Pair<List<int>, *Pair<int, int>>{};
}
```

Every distinct set of type arguments creates its own struct (monomorphization), so `List<int>` and `List<float>` are two
separate types with their own sizes. Writing `List<int>` in two places refers to the same type.

Not supported yet: methods on generic structs (methods on a specific instance like `*List<int>` work), constants
inside generic structs, function types as members of generic structs and type aliases like `MyList :: List<int>`

Bug fixes:

* Fixed keywords directly followed by an operator e.g `int>` being lexed as identifiers

## Update v0.2.8 - 30. May 2026

Big changes to functions to make them first-class citizens

Function definitions can now appear as normal expressions to create annonymous functions.

For example:
```odin
MyButton :: struct {
    onClick: () -> void;
}

main :: () {
    // Here we assign a function to the onClick handler
    btn := MyButton{
        onClick: () {
            Print("You clicked!");
        }
    };

    btn.onClick(); // Prints You clicked!

    // Here we define a fixed array of functions
    myFuncs := [
        () { Print("0"); },
        () { Print("1"); },
        () { Print("2"); },
    ];

    for func in myFuncs {
        func(); // 0, 1, 2
    }
}
```

Bug fixes:

* Fixed assignment to pointer that would dereference the pointer on assigment making normal pointer assignment impossible
* Fixed Print not producing newlines correctly because of default arguments being wrong

## Update v0.2.7 - XX. Mar 2026

* Fixed break and continue in for-loops or while-loops not correctly being attached to the correct loop
* Fixed name collision when having two constants with the same name in different scopes
* Fixed crash due to .data array member not being resolved
* Fixed wrong precedence with casting using 'as'
* Fixed constant identifiers not transforming to float
* Fixed `append` being broken when appending structs in a variable

## Update v0.2.6 - 26. Mar 2026

* Constants within structs can now refer to other constants within the same struct

```odin
Color :: struct {
    r, g, b, a: u8;

    Red   :: Color{255, 0, 0, 255};
    Green :: Color{0, 255, 0, 255};
    Blue  :: Color{0, 0, 255, 255};

    Rainbow :: [Red, Green, Blue];
}
```

## Update v0.2.5 - 25. Mar 2026

* Structs can not have constant members that live on the struct instance

```odin
Color :: struct {
    r, g, b, a: u8;

    Red   :: Color{255, 0, 0, 255};
    Green :: Color{0, 255, 0, 255};
    Blue  :: Color{0, 0, 255, 255};
}

main :: () {
    Print("%", Color.Red);
    Print("%", Color.Green);
    Print("%", Color.Blue);
}
```

* Structs can now be defined and used in any order

Before, structs had to be declared before any using structs like in C.
Now there is no order to how structs can be defined

* Bug fixes

** Fixed structs being allowed to have recursive definitions

```odin
Foo :: struct {
    x: Foo; // <-- This is not allowed
}
```

** Fixed and correctly detect cycles in struct sizing
```odin
A :: struct {
    b: B;
}

B :: struct {
    a: A; // <-- This is not allowed, since it would do a cycle back to typechecking A
}

```

## Update v0.2.4 - 16. Mar 2026

* BREAKING: New syntax for casting

The keyword 'cast' is now replaced with the suffix 'as' keyword, like in Rust

```odin
myString := "New cast syntax" as *u8;

memory := malloc(1024) as *u8;

mySmallNumbers := [1 as u8, 2, 3, 4, 5];
```

Auto-casting with 'xx' is left unchanged.

The 'as' way of casting is often more natural to write as you append to the right of your expression when you need a cast,
opposed to cast(type) being prefix and requiring paranthesis.

## Update v0.2.3 - 13. Mar 2026

* Structs can now appear as constants

```odin
MyColor :: Color{255, 127, 0, 255};
```

### Minor fixes

* Fixed wrong casting behavior for integers. Now works correctly with the new fmt.Print
* Fixed wrong struct sizing leading to weird behavior

## Update v0.2.2 - 07. Mar 2026

### Better enums

Enums now have the members:

* ```count```: To get the number of enum values
* ```names```: To get the string representation of the enum values
* ```values```: To get an array of all the enum values of the enum

```odin
Fruit :: enum {
    mango,
    kiwi,
    watermelon,
    dragonfruit,
    pear,
    apple,
}

main :: () {

    print("%\n", Fruit.count);

    for name in Fruit.names {
        print("%\n", name);
    }

    for value in Fruit.values {
        print("%\n", cast(int) value);
    }

}
```

### Minor fixes
* Fix ```append``` not working with literals
* Fix lots of issues in runtime.sd after strings are now 16 bytes slices

## Update v0.2.1 - 18. Feb 2026

* Introduction of default parameters and positional arguments for functions

Function parameters can now have default values

```odin
Myprint :: (format: string, args: ...any, newline := true) {
    Print(format, ...args);
    if newline {
        Print("\n");
    }
}

main :: () {
    Myprint("Default parameters are %", "useful during refactoring or providing an extensible api", newline: false);
}


```

## Update v0.2 - 1. Feb 2026

* Introduction of runtime type reflection. 

Its now possible to inspect the type of variables at runtime using the new ```std/reflect.sd``` package

See ```tests/runtime_reflection.sd``` as an example of how to use it

* Introduced variadic function parameters

Its now possible to declare a function parameter as being variadic and passing variadic arguments to functions just like in  
any other language.

Example:
```odin
printArguments :: (args: ...any) {
    for arg, i in args {
        print("Arg % = %", i, arg);
    }
}

main :: () {
    printArguments("Hello! My name is Charles", 420);
}
```

* External C functions now has to be declared inside an extern block instead of being at the end of each function definition

Old syntax:
```odin
printf  :: (format: string, args: ...any) -> int #extern "C"
sprintf :: (buffer: *u8, format: string, args: ...any) -> int #extern "C"
```

New syntax:
```odin
extern "C" {
    printf  :: (format: string, args: ...any) -> int;
    sprintf :: (buffer: *u8, format: string, args: ...any) -> int;
}
```