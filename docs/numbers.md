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

## Overflow

`+`, `-` and `*` of integers, also `+=` and the others, and `-x`, throw a `Panic` when the result doesn't fit the type,
instead of wrapping around:

```odin
m: i32 = 2147483647;
m += 1;                     // Panic: 2147483647 + 1 doesn't fit in i32
```

* The smallest value divided by -1, e.g `-2147483648 / -1`, is an overflow too, which the processor traps on. The
  remainder of it, `x % -1`, is 0.
* A function whose arithmetic wraps around on purpose, e.g a hash, a random generator or SHA-256, is marked
  `@noOverflowCheck`, and all of its integer arithmetic wraps around:

  ```odin
  @noOverflowCheck
  fnv :: (text: string) -> u64 {
      h: u64 = 14695981039346656037;
      for c in text { h = (h ^ c as u64) * 1099511628211; }
      return h;
  }
  ```

* `saad build --unchecked` leaves the checks out, with those of indexes and null pointers. They are in every other build,
  also with `--llvm`.
* What the compiler adds itself, e.g the arithmetic of an index or of a loop over a range, isn't checked, and a constant
  without a type, e.g `-9223372036854775807 - 1`, is checked when its value is fitted to a type, when compiling.
* How: an `i8` to `i32` (or unsigned) is computed in 64 bits and compared with the range of its type. Of 64 bits, `+` and
  `-` look at the signs (or compare, unsigned), and `*` checks that the result divided by a is b
  (`emit_checked_arithmetic()` in `ir_lower.c`). Nothing changes in the backends.
* What it costs, measured with `tools/bench/overflow_bench.sd` against the compiler before the checks: with `--llvm`, 3% on
  a loop of arithmetic, which LLVM simplifies, and about 16% on sorting and on encoding JSON. With the x64 backend, 60% on
  the loop of arithmetic, and 10 to 20% on sorting and JSON.

## The precedence of as

`as` binds looser than the unary operators `* & - ! ~` and tighter than the binary ones, and casts are read from left to
right:

```odin
*p as i64           // (*p) as i64: the value at p, as an i64
*(p as *i8) as i64  // the byte at p, as an i64
-a as f64           // (-a) as f64
3 * a as i64        // 3 * (a as i64)
2.5 as int as f64   // (2.5 as int) as f64, which is 2.0
```
