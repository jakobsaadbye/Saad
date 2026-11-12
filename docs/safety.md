# Safety

What Saad checks, so that a mistake is an error when compiling, or a `Panic` with its place when the program runs,
instead of a value that is silently wrong.

## When compiling

| Mistake | What happens |
| --- | --- |
| A number into a type it may not fit, e.g an `i64` into an `int`, or an `i32` into a `u64` | An error with a hint to write `as` (see `docs/numbers.md`) |
| A `*void` into a pointer to a type, e.g `post: *Post = memory;` | An error with a hint to write `memory as *Post`, which says what the memory holds |
| A format whose places don't match its values, e.g `print("% %", 1)` | An error that counts them (below) |
| Returning the address of a variable of the function, e.g `return &x;` | An error, as the variable is gone when the function returns (below) |
| A number cast to an enum that isn't one of its values, e.g `7 as Color` | An error (see `docs/enums.md`) |
| A switch on an enum or a union that doesn't handle every value | An error that names them |
| A newtype where its type is wanted, or the other way, e.g a `Meters` where an `f64` is wanted | An error (see `docs/newtypes.md`) |

## Warnings

A variable of a function whose value is never used is a warning, e.g a value that was computed for nothing, or a typo
in a name that is used instead. Giving it a value, e.g `x = 5;`, isn't using it. An `error` that is taken and never
looked at gets a warning of its own, as it is a failure that goes unnoticed:

```odin
value, err := load();      // warning: The error 'err' is never looked at, so a failure goes unnoticed
                           // hint: Check it, e.g 'if err != null { ... }', or don't take it, so it is thrown: 'v := f();'
_, err := load();          // _ takes a value that isn't used
```

Parameters aren't warned about, as a function often has to take one it doesn't use, e.g of an interface. A write
through a variable, e.g `p.x = 5` or `xs[i] = 1`, uses it. `warn_unused_locals()` in `typer.c`; `tests/warnings/` has
the cases, which `tools/linux/runtests.sh` checks.

## When the program runs

These throw a `Panic`, which names the place, and which a web server turns into a 500 (see `docs/errors.md`):

* an index outside of the elements of an array, a slice, a list or a string
* a null pointer that is dereferenced
* a number cast to an enum that isn't one of its values, and a switch on an enum that holds none of them
* integer arithmetic whose result doesn't fit its type, e.g `2147483647 + 1 doesn't fit in i32` (see `docs/numbers.md`)
* a division by zero, and an `assert` that fails
* with `--check-memory`, memory of an arena that was cleared or freed

`saad build --unchecked` leaves the checks of indexes, null pointers, enum casts and integer overflow out.

## Formats

A function with a parameter `format: string` followed by `...any` puts its values in the places of the format, each `%`:
`print`, `sprint`, `StringBuilder.add`, `errors.make`, `errors.wrap`, and functions of the program written the same way.
When the format is written in the call and values are given, the number of places must be the number of values:

```odin
print("% has % moons", "Mars");     // error: The format has 2 places for values, but 1 value is given
print("100%");                      // fine: without values the text is as written
print("100\% of %", name);          // \% is a % of the text
print(format, ...values);           // not counted: the format isn't written here, and the values are spread
```

`check_format_arguments()` in `typer.c`.

## Addresses that are returned

The variables of a function, also its parameters, are gone when it returns, so their addresses can't be returned:

```odin
counter :: () -> *int { x := 0; return &x; }                     // error
start   :: () -> Cursor { grid: Grid; return Cursor{at: &grid.cells[0]}; }   // error: also inside a literal
digits  :: () -> []int { d: [3]int = [1, 2, 3]; return d; }      // error: a slice of a fixed array of the function
```

The address of what a pointer, a slice or a list points to may be returned, e.g `&p.y` of a `p: *Point` or `&xs[0]` of
a `[]int`, and so may the address of a global or of memory from `new`. `check_not_dangling()` in `typer.c` looks at
`&x`, `&x.member` and `&x.array[i]` of a variable, in the value and in the struct and array literals it is made of. An
address that is kept in a variable first, e.g `p := &x; return p;`, isn't followed.
