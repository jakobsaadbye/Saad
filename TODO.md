# TODO

What is still open, checked against the compiler on 14. Nov 2026. What is done is in `CHANGELOG.md` and `docs/`.

## Bugs

- `as` binds tighter than `*` and `&`: `*(p as *i8) as i64` is read as `*((p as *i8) as i64)`. The casts should bind
  looser than the unary operators
- `value()` of an empty optional stops at an assert in collections.sd, without a message of its own. A message needs a
  string literal there, which makes collections.sd import the string prelude while it is parsed, and that breaks the
  names it uses (e.g free)

## Language

- Structs
  - [ ] Structs declared inside a struct, e.g `Outer :: struct { Inner :: struct { ... } }`
  - [ ] Anonymous structs and function types as members of generic structs
  - [ ] Default values of members that call functions or are collection literals, e.g `xs: [..]int = [1, 2];`
- Enums
  - [ ] Backing integer types, e.g `Small :: enum u8 { a, b }`
  - [ ] Enums as keys of a `Map` without writing a `hash` method (pointers already work)
- Functions
  - [ ] Functions in constant arrays, e.g `handlers :: [() -> int { return 1; }];`
  - [ ] Generic functions as values, e.g `f := max<int>;` (calls can give type arguments, values can't yet)
  - [ ] Methods on fixed arrays and pointers to pointers
  - [ ] Methods on receivers like `*List<int>` that are declared after they are called
  - [ ] Taking one value of a function with more than one, e.g `ok := two();` (`_` is needed now: `ok, _ := two();`)
- Generics
  - [ ] Constraints on type parameters, e.g `<T: Hashable>`
  - [ ] Generic type aliases, e.g `Twin<T> :: Pair<T, T>;`
  - [ ] Generic operators, e.g `operator + :: <T>(a: Vec<T>, b: Vec<T>) -> Vec<T>`
- Globals
  - [ ] Globals whose value is computed when the program starts, e.g `rng := newRandom(clock());`
- Syntax
  - [ ] `use` with a list of names, e.g `use math.{sin, cos};`
  - [ ] A conditional expression, e.g `x := a ? b : c;` (a `switch` expression works today)
- Imports
  - [ ] A project file that lists or excludes folders

## Standard library

- [ ] `Map.remove` in O(1) that keeps the order (tombstones). It moves the entries after the key now
- [ ] `dbms.QueryError` as an error type with the SQL, instead of a kind

## Backend

- [ ] C functions that return structs of 3, 5, 6 or 7 bytes ("Not implemented in the IR backend yet")
- [ ] ARM64: a `TargetDesc` and `arm64_emit.c` (the IR and the register allocator are independent of the target)
- [ ] Optimizations: promote slots to vregs (mem2reg), constant folding, immediates and memory operands in `x64_emit.c`
- [ ] A fast path for struct literals whose values are all simple, which makes fewer instructions

## Tools

- [ ] Error messages with more than one highlight, where a single red underline is confusing, e.g at a dereference
- [ ] A quick fix that renames a use of a `@deprecated` name to the name its message gives

## Ideas, not decided

- Compile-time execution on top of the IR
- Non-null pointers, `@sealed` newtypes and a `parse` hook for newtypes and enums
- Lifetimes and more memory safety, e.g against use after free
- Iterators of your own, e.g a `next()` protocol for `for x in c`, besides `elements()`
