# The standard library

```
std/
  prelude/      Always there, without an import
    runtime.sd      Types for reflection (Type, TypeStruct, ...), attributeOf and the runtime functions
    collections.sd  List<T> ([..]T) and Map<K, V>
    slices.sd       Methods on slices, arrays and lists: sort, reverse, indexOf, contains, ...
    string.sd       The methods of string (slice, split, indexOf, free, ...)
  c/            namespace c: bindings for the C runtime (stdio, stdlib, string)
  fmt/          namespace fmt: print, sprint and StringBuilder
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
text := fmt.sprint("Score: %", score);              // like print, but returns the text
text.free();
sb := fmt.newStringBuilder(64);
sb.add("% + % = %", 1, 2, 3);
text := sb.toString();
```

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
os.exit(1);
```

`os.windows` has the bindings for the Windows API, e.g `CreateThread`, critical sections, condition variables
and `Sleep`.

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

A seeded xorshift64* generator. Fast and good for games and simulations, not for security.

```odin
rng := random.newRandom(time.unix() as u64);   // the same seed gives the same numbers
dice := rng.nextInt(1, 7);          // 1 to 6
f := rng.nextFloat();               // from 0 up to but not including 1
coin := rng.nextBool();
if rng.chance(0.1) { ... }          // true one in ten times
random.shuffle(&rng, cards);        // any slice, list or fixed array
bits := rng.next();                 // 64 random bits
```

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
text := json.encode(person, indent: 2);   // over more lines

p: Person;
err := json.decode(text, &p);             // "" when it worked, otherwise e.g "address.zip: Expected a number, but got a string"

value, parseErr := json.parse(text);      // a json.Value of any shape
name := value.get("first_name").asString();
first := value.get("tags").at(0).asString("none");
value.toJson();
```

* `encode` and `decode` work through reflection: bool, numbers, strings, enums (by name), structs, fixed arrays,
  slices, `List<T>`, `Map<string, V>`, pointers and `json.Value`.
* `json.Field` is an attribute (see `docs/reflection.md`): `name` renames a member, `skip` leaves it out, and
  `omitEmpty` leaves out 0, false, "", null and things without elements.
* Decoding keeps members that are not in the JSON, so default values stay, and skips members it doesn't know.
  Integers are read from the text, so an `i64` or `u64` doesn't lose digits, and they must fit their type.
* `json.Value` has `get`, `at`, `has`, `length`, `keys`, `asString`, `asNumber`, `asInt`, `asBool`, `isNull`,
  `toJson`, `clone` and `free`, and is made with `newObject`, `newArray`, `newString`, `newNumber`, `newBool`,
  `newNull`, `put` and `push`.

## math, mem, c

* `math`: the functions of C's `math.h` (`sin`, `sqrt`, `pow`, ...).
* `mem`: `Arena`, a bump allocator, and the sizes `Kilobyte` and `Megabyte`.
* `c`: `printf`, `fopen`, `malloc`, `memcpy`, `strlen`, ... Use it when nothing else fits. Sizes that are
  `size_t` in C are `i64` here.

## Ideas for later

* `os.args()` for the command line arguments (needs support in the runtime).
* `strings` with `contains`, `trim`, `replace`, `join` and a `parseInt`/`parseFloat`.
* `os.listDir` and `os.makeDir`, and a `path` namespace (`join`, `extension`, `directory`).
* `math` functions of its own: `Pi`, `clamp`, `lerp`, `min`/`max`, vectors.
* `hash` (e.g fnv1a, crc32), `base64` (e.g fnv1a, crc32) and `threads` (a portable layer over `os.windows`).
