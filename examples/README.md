# Learn Saad in 28 small steps

Each file is one short program about one idea, with comments that explain it. Read them in order: each builds on
the ones before. Run one with

```
saad run examples/01_hello.sd
```

## The basics

| | |
| --- | --- |
| [01_hello.sd](01_hello.sd) | `main`, `print` and comments |
| [02_variables.sd](02_variables.sd) | `x := 1`, `x: int = 1` and constants `X :: 1` |
| [03_numbers.sd](03_numbers.sd) | Arithmetic, number types and `as` |
| [04_strings.sd](04_strings.sd) | Strings, characters and their methods |
| [05_if_else.sd](05_if_else.sd) | `if`, `else` and conditions |
| [06_loops.sd](06_loops.sd) | `for`, ranges, `while`, `break` and `continue` |
| [07_functions.sd](07_functions.sd) | Parameters, results, defaults and named arguments |

## Data

| | |
| --- | --- |
| [08_arrays.sd](08_arrays.sd) | Fixed arrays, lists `[..]T` and slices `[]T` |
| [09_structs.sd](09_structs.sd) | Structs, literals and defaults |
| [10_methods.sd](10_methods.sd) | Methods and `self` |
| [11_pointers.sd](11_pointers.sd) | `&`, `*`, `null` and `new` |
| [12_enums.sd](12_enums.sd) | Enums and `enum flags` |
| [13_switch.sd](13_switch.sd) | `switch` statements and expressions |
| [14_maps.sd](14_maps.sd) | `Map<K, V>` and `[key: value]` |
| [15_optionals.sd](15_optionals.sd) | `?T`, `??` and `?.` |
| [16_unions.sd](16_unions.sd) | Unions and `switch v in` |

## Going further

| | |
| --- | --- |
| [17_lambdas.sd](17_lambdas.sd) | Functions as values, `x => x * 2`, `map` and `filter` |
| [18_defer.sd](18_defer.sd) | `defer` |
| [19_errors.sd](19_errors.sd) | `error`, `try`, `catch`, `throw` and error types |
| [20_generics.sd](20_generics.sd) | `<T>` functions and structs |
| [21_interfaces.sd](21_interfaces.sd) | Interfaces |
| [22_embedding.sd](22_embedding.sd) | `@embed` |
| [23_namespaces.sd](23_namespaces.sd) | `namespace`, `import`, `use` and `@private` (with [geometry/](geometry/geometry.sd)) |
| [24_attributes.sd](24_attributes.sd) | Attributes of your own |
| [25_reflection.sd](25_reflection.sd) | `any`, `typeof` and types while the program runs |
| [26_memory.sd](26_memory.sd) | `new`, `mem.delete` and arenas |
| [27_compile_time_if.sd](27_compile_time_if.sd) | `#if` for one system or another |
| [28_standard_library.sd](28_standard_library.sd) | `json`, `time`, `os` and `random` |

## Bigger programs

| | |
| --- | --- |
| [breakout/](breakout) | A game with raylib |
| [blog/](blog) | A blog backend with `web` and a database (`dbms`) |
| [feed/](feed) | A feed API on SQLite: the $12 server challenge |

The [docs](../docs) go deeper into each part, e.g [errors](../docs/errors.md) or the
[standard library](../docs/standard_library.md).
