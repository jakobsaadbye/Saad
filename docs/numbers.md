# Numbers

## Conversions

A number converts to another type by itself only when every value of its type fits in the other:

| From | Converts by itself to |
| --- | --- |
| a signed integer, e.g `i32` | a signed integer at least as big, e.g `i32`, `i64` |
| an unsigned integer, e.g `u32` | an unsigned integer at least as big, or a signed one that is bigger, e.g `u64`, `i64` |
| a float | a float at least as big |
| a number written in the program, e.g `200` | any number type its value fits in, e.g `u8` (checked when compiling) |

Anything else is written with `as`, which says that it is meant:

```odin
big: i64 = 5000000000;
takes(big);              // error: Wanted type int, got type i64
                         // hint: A value of type i64 doesn't always fit in int, so it isn't converted by itself.
                         //       Write 'x as int' when it fits, or use i64
takes(big as int);       // 705032704: the low 32 bits, as written
```

* This holds everywhere a value goes: a variable, an assignment, an argument, a return value and a member of a literal.
  Before, arguments and return values could narrow by themselves, e.g an `i64` length into an `int` parameter.
* `int` is an `i32`. Lengths, counts and indexes are `i64`, e.g `s.len`, `xs.count`, so the functions of the standard
  library that take sizes or indexes take an `i64`: `mem.allocate`, `mem.copy`, `string.slice`, `slice.swap`, and the
  C functions that take a `size_t`, e.g `malloc`, `memcpy` (`int` was 32 bits there, so a size over 2 GB was cut).
* The rule is in `can_cast_implicitly()`, and `types_fit_either_way()` takes it for numbers (`typer.c`).
