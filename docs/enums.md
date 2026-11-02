# Enums

```odin
Status :: enum { open, closed, locked }          // 0, 1, 2
Level  :: enum { low = 10, mid, high = 50 }       // 10, 11, 50

s := Status.open;
if s == .closed { ... }                           // the enum comes from the other side
print("% of %", s, Status.count);                 // .open of 3. Status.names and Status.values too
```

* A value without its enum, e.g `.open`, takes the enum that is wanted where it is written: the other side of `==`,
  the type of a variable or a parameter, a `case`, a `return`.
* An enum value is cast to a number with `as`, e.g `s as int`. Arithmetic on enums is an error.

## Flags

An `enum flags` is a set of options in one value, instead of an `int` whose bits are only known by comment:

```odin
Options :: enum flags { verbose, color, quiet }   // 1, 2, 4

o := Options.verbose | .color;
o |= .quiet;                                       // set
o &= ~.verbose;                                    // clear
o ^= .color;                                       // toggle

o.has(.quiet)                                      // all of these are set, e.g o.has(.quiet | .color)
o.hasAny(.verbose | .color)                        // any of them is set
o.with(.verbose)                                   // new flags with these set
o.without(.quiet)                                  // new flags without these
o.isEmpty()                                        // none is set

print("%", o);                                     // .color | .quiet
```

* The values are the bits 1, 2, 4, 8, ... A value can be given, e.g `none = 0` or `all = 7`, and the next value is
  the next bit that no value before has.
* `|`, `&`, `^` and `~` work on two flags of the same enum flags, and give flags of it. On a plain enum they are an
  error. `|=`, `&=` and `^=` set, clear and toggle.
* `fmt` writes the flags that are set, e.g `.verbose | .quiet`, a value with a name of its own by its name, e.g
  `.all`, and no flags as `0`. `json` writes them as a list of names, e.g `["verbose", "quiet"]`, and reads such a list
  (or a single name, or the number).
* The methods are functions of the prelude (`flagsHas`, ... in `std/prelude/runtime.sd`) that the compiler calls for
  `o.has(x)`, ...; an enum can declare a method of the same name instead.
* The runtime's own `Type.flags` is an enum flags: `type.flags.has(.isOptional)`.

## Precedence

`|`, `^` and `&` bind tighter than the comparisons, like in Rust and Go and unlike C: `x & 1 == 0` is `(x & 1) == 0`,
and `o == .verbose | .color` compares `o` with both flags.
