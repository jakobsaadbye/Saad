# The standard library

```
std/
  prelude/      Always there, without an import
    runtime.sd      Types for reflection (Type, TypeStruct, ...), attributeOf and the runtime functions
    collections.sd  List<T> ([..]T) and Map<K, V>
    slices.sd       Methods on slices, arrays and lists: sort, reverse, indexOf, contains, ...
    string.sd       The methods of string (slice, split, indexOf, free, ...)
  c/            namespace c: bindings for the C runtime (stdio, stdlib, string)
  db/           namespace db: run SQL and read rows into structs (see docs/db.md)
    postgres/       namespace db.postgres: the Postgres driver, through libpq.dll
  fmt/          namespace fmt: print, sprint and StringBuilder
  io/           namespace io: bytes on stdin, stdout and stderr
  json/         namespace json: encode, decode, parse and json.Value
  os/           namespace os: files, environment variables, processes
    windows.sd      namespace os.windows: the Windows API (threads, Sleep, ...)
  vendor/       Bindings for libraries the standard library doesn't own
    raylib.sd       namespace raylib
  math.sd       namespace math
  mem.sd        namespace mem: arenas
  random.sd     namespace random
  time.sd       namespace time
```

The folders only keep things tidy: a namespace is what a file says it is (see `docs/namespaces.md`). The prelude
is imported by the compiler, the rest by `import` or `use`. The files in `c/` and `prelude/runtime.sd` don't
get the prelude, as the prelude is built on them.

## fmt

```odin
fmt.print("% is % years old", name, age);          // with a newline
fmt.print("no newline", newline: false);
fmt.print("100%% done");                            // %% is a %
text := fmt.sprint("Score: %", score);              // like print, but returns the text
text.free();
joined := fmt.join(["a", "b", "c"], ", ");          // "a, b, c"
```

### StringBuilder

```odin
sb: fmt.StringBuilder;              // ready to use. fmt.newStringBuilder(1024) starts with room for 1024 bytes
sb.add("% + % = %", 1, 2, 3);       // formatted like fmt.print
sb.addLine("total: %", total);      // with a newline after it
sb.addString(name);
sb.addChar(',');
sb.addRepeated(' ', 4);             // e.g indentation
sb.addValue(person);                // a value like fmt.print writes it
sb.addQuoted(name);                 // "Ada", with escapes. addQuotedChar('a') is 'a'

for n in numbers { sb.add("%, ", n); }
sb.removeLast(2);                   // the ", " after the last number

sb.length(); sb.isEmpty(); sb.endsWith("!");
text := sb.toString();              // the text, not a copy: it changes with the builder. .copy() to keep it
sb.clear();                         // empty, and keeps its memory for more
sb.free();
```

The text of a StringBuilder is always followed by a 0 byte, so `sb.buffer` can be given to C.

Values are written as they are written in Saad, so printed values can be copied back into a program:

```odin
fmt.print("%", Person{name: "Ada", age: 36});   // Person{name: "Ada", age: 36}
fmt.print("%", ["a", "b"]);                     // ["a", "b"]
fmt.print("%", Shape{circle: Circle{2.0}});     // Shape{circle: Circle{radius: 2.000000}}
fmt.print("%", "Ada");                          // Ada. A string on its own has no quotes
```

Strings in a struct, an array or a union get quotes, with `"`, `\` and newlines escaped, and a char gets single quotes
(`'a'`). An enum value is `.open`,
an anonymous struct `{w: 1.000000, h: 2.000000}`, and the variant without a value of a generic union `.none`.

## os

```odin
text, ok := os.readFile("config.txt");      // the whole file, allocated with malloc
os.writeFile("out.txt", "Hello");           // creates or replaces, false on failure
os.appendFile("log.txt", "line\n");
os.fileExists("out.txt");
os.renameFile("out.txt", "new.txt");
os.removeFile("new.txt");
home, found := os.getEnv("HOME");
code := os.run("echo hi");                  // runs a shell command, returns its exit code
text, code := os.output("git status");      // runs a shell command, returns what it wrote and its exit code
args := os.args();                          // the command line, args[0] is the program
os.exit(1);
```

`os.output` runs the command with cmd.exe and puts it in quotes first, so a command that starts with a quoted path
works, e.g `os.output("\"C:/Program Files/tool.exe\" \"my file.txt\"")`.

`os.windows` has the bindings for the Windows API, e.g `CreateThread`, critical sections, condition variables,
`Sleep`, `GetStdHandle`, `ReadFile` and `WriteFile`.

## io

Bytes on the standard streams, for a program that another program talks to, e.g the language server (`lsp/`):

```odin
line, ok := io.readLine();      // without "\n" or "\r\n". ok is false at the end of the input
body, ok := io.read(52);        // exactly 52 bytes
waiting := io.hasInput();       // bytes have arrived, so a read won't wait
io.write("hello\n");            // stdout, nothing added
io.writeError("a log line\n");  // stderr
```

The input is binary, so `\r\n` stays as it is. `fmt.print` goes through the C runtime, which has its own buffer, so
don't mix it with `io.write`.

## time

```odin
start := time.now();                // seconds, high resolution, for measuring
seconds := time.unix();             // seconds since 1970 (UTC)
time.sleep(100);                    // milliseconds

watch := time.startStopwatch();
...
fmt.print("took % ms", watch.milliseconds());   // also seconds() and restart()
```

## random

Named like Python's random module. A xorshift64* generator: fast and good for games and simulations, not for
security.

```odin
dice  := random.between(1, 6);      // 1 to 6, both included
f     := random.random();           // from 0 up to but not including 1
x     := random.uniform(-1.0, 1.0); // from -1 up to but not including 1
name  := random.choice(names);      // one of them
picks := random.choices(names, 3);  // 3 of them, maybe the same one more than once
some  := random.sample(names, 2);   // 2 different ones
random.shuffle(cards);              // any slice, list or fixed array
if random.chance(0.1) { ... }       // true one in ten times
height := random.gauss(170.0, 10.0); // normal distribution with a mean and a standard deviation
bits  := random.bits();             // 64 random bits
```

* The functions use a generator that is seeded from the clock the first time it is used. `random.seed(42)` makes
  them repeatable.
* `rng := random.newRandom(42);` is a generator of your own, with the same methods: `rng.between(1, 6)`,
  `rng.choice(names)`, `rng.shuffle(cards)`, ...

## Methods on slices (prelude)

Slices, fixed arrays and lists have these methods without an import. They are generic methods on `[]T` in
`std/prelude/slices.sd`, so any library can add methods like them.

```odin
xs := [3, 1, 2];
xs.sort();                          // with <, quicksort with insertion sort for small ranges
xs.isSorted();
xs.reverse();
xs.indexOf(3);                      // -1 when not found, compares with ==
names.contains("anna");
xs.swap(0, 1);
```

With a function, which can be a function literal or the name of a function:

```odin
doubled := xs.map((x: int) -> int { return x * 2; });   // a new [..]int
names   := users.map(nameOf);                           // [..]string, U is inferred from nameOf
ids     := users.map<i64>(idOf);                        // or given
adults  := users.filter(isAdult);                       // the elements that it is true for, in order
total   := prices.reduce(0.0, (sum: f64, p: f64) -> f64 { return sum + p; });
user, found := users.find(isAdult);                     // the first one, found is false when there is none
users.findIndex(isAdult);                               // -1 when there is none
users.some(isAdult);  users.every(isAdult);  users.countWhere(isAdult);
```

`map` and `filter` take an allocator for the new list as their last argument, e.g `xs.filter(isEven, &alloc)`. Null,
the default, is the heap. A function type can leave out the names of its parameters: `(T) -> U`.

A type's own method of the same name wins, e.g `List` has its own `indexOf`, `contains` and `reverse`.

## json

```odin
Person :: struct {
    @json.Field(name: "first_name")
    firstName: string;
    age:       int = 18;
    tags:      [..]string;
    address:   *Address;
    extra:     json.Value;            // any JSON

    @json.Field(skip: true)
    cache: *void;

    @json.Field(omitEmpty: true)
    nickname: string;
}

text := json.encode(person);              // {"first_name":"Ada","age":36,...}
text := json.encode(person, json.Pretty); // over more lines

p, err := json.decode<Person>(text);      // err is "" when it worked, otherwise e.g "address.zip: Expected a number, but got a string"
err2 := json.decodeInto(text, &settings); // into a value that exists, which keeps what the JSON doesn't have

value, parseErr := json.parse(text);      // a json.Value of any shape
name := value.get("first_name").asString();
first := value.get("tags").at(0).asString("none");
value.toJson();
```

* `encode` and `decode` work through reflection: bool, numbers, strings, enums (by name), structs, unions,
  fixed arrays, slices, `List<T>`, `Map<string, V>`, pointers and `json.Value`. A union is the name of a variant
  without a value, e.g `"empty"`, or an object with the name and the value, e.g `{"circle": {"radius": 2}}`.
* `json.Field` is an attribute (see `docs/reflection.md`): `name` renames a member, `skip` leaves it out, and
  `omitEmpty` leaves out 0, false, "", null and things without elements.
* Decoding keeps members that are not in the JSON, so default values stay, and skips members it doesn't know.
  `decodeInto` keeps the values a value already has.
  Integers are read from the text, so an `i64` or `u64` doesn't lose digits, and they must fit their type.
* Both take options as the last argument:

  ```odin
  json.encode(person, json.Pretty);         // indent: 2
  person, err := json.decode<Person>(text, json.SnakeCase);
  json.encode(person, json.Options{indent: 4, names: .snakeCase, enumsAsNumbers: true, omitEmpty: true});
  ```

  | Option | Default | |
  | --- | --- | --- |
  | `indent` | 0 | Spaces per level. 0 writes everything on one line |
  | `names` | `.asWritten` | How member names are written and read: `.camelCase`, `.pascalCase`, `.snakeCase`, `.kebabCase`, `.upperSnake`. A name from `json.Field` is used as it is |
  | `enumValues` | `.asWritten` | The same for the values of enums |
  | `enumsAsNumbers` | false | Writes enums as numbers. Both are read |
  | `omitEmpty` | false | `omitEmpty` for every member |
  | `strict` | false | Decoding fails on members the struct doesn't have |

  The standard options are `json.Compact`, `json.Pretty`, `json.CamelCase`, `json.SnakeCase` and `json.KebabCase`.
  `json.styleName("userID", .snakeCase)` gives `user_id`: words are split where the case changes and at `_` and `-`.
* `json.Value` has `get`, `at`, `has`, `length`, `keys`, `asString`, `asNumber`, `asInt`, `asBool`, `isNull`,
  `toJson`, `clone` and `free`, and is made with `newObject`, `newArray`, `newString`, `newNumber`, `newBool`,
  `newNull`, `put` and `push`.

## db

`users, err := conn.query<User>("SELECT * FROM users WHERE id > $1", 10)` reads rows into structs, `conn.exec` runs
statements and `conn.queryOne` reads one row or value. `db.postgres` connects to Postgres. See `docs/db.md`.

## math, mem, c

* `math`: the functions of C's `math.h` (`sin`, `sqrt`, `pow`, ...).
* `mem`: `Arena`, memory that is freed all at once, and the sizes `Kilobyte` and `Megabyte`. The zero value is
  ready to use: `arena.allocate(size)` gives zeroed memory, `arena.allocator()` an `Allocator` for lists, maps and
  `db.Connection`, and `arena.clear()` and `arena.free()` free all of it.

```odin
arena: mem.Arena;
alloc := arena.allocator();
names: [..]string;
names.allocator = &alloc;      // the list grows inside the arena
arena.free();
```
* `c`: `printf`, `fopen`, `malloc`, `memcpy`, `strlen`, ... Use it when nothing else fits. Sizes that are
  `size_t` in C are `i64` here.

## Ideas for later

* `strings` with `contains`, `trim`, `replace`, `join` and a `parseInt`/`parseFloat`.
* `os.listDir` and `os.makeDir`, and a `path` namespace (`join`, `extension`, `directory`).
* `math` functions of its own: `Pi`, `clamp`, `lerp`, `min`/`max`, vectors.
* `hash` (e.g fnv1a, crc32), `base64` (e.g fnv1a, crc32) and `threads` (a portable layer over `os.windows`).
