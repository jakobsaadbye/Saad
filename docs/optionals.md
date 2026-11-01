# Optionals

`?T` is a `T` or nothing. It is for values that may be missing: a column that can be `NULL`, a member that JSON
may leave out, the result of a search.

```odin
User :: struct {
    name:    string;
    age:     ?int;          // empty until it is set
    address: ?Address;
    manager: *User;
}

age: ?int;                 // empty
age = 36;                  // a T becomes a ?T where one is wanted
age = null;                // empty again

if age.hasValue() {
    print("%", age.value());
}
years := age ?? 0;                          // the value, or 0 when it is empty
zip   := user?.address?.zip;                // a ?int, empty when user or its address is
text  := user?.manager?.name ?? "nobody";   // ?. also works on pointers, where null is empty
if age == null { ... }                      // the same as !age.hasValue()
if age == 36 { ... }                        // true when it has the value 36
```

## The rules

* `?T` is `Optional<T>` from `std/prelude/collections.sd`: the value and a bool. It is stored in place, so a `?int`
  needs no allocation, unlike a `*int`.
* A `T` or `null` can be given wherever a `?T` is wanted: assignments, declarations, arguments, returns, struct
  literals and the elements of collections. The other way around is an error, with a hint how to get the value.
* An optional is only read through its methods and operators, so it is never used by accident while it is empty:

  | | |
  | --- | --- |
  | `x.hasValue()` | true when it has a value |
  | `x.value()` | the value. The program stops (an `assert` in `collections.sd`) when it is empty |
  | `x.valueOr(fallback)` | the value, or the fallback. The fallback is always computed |
  | `x ?? fallback` | the value, or the fallback, which is only computed when `x` is empty |
  | `x?.member`, `x?.method()` | the member of the value, as an optional, or empty when `x` is |
  | `x == null`, `x != null` | if it is empty |
  | `x == value`, `x == y` | equal when both have the same value, or both are empty |

* `??` binds tighter than comparisons and looser than arithmetic, like in Swift: `age ?? 0 > 18` is
  `(age ?? 0) > 18`, and `age ?? x + 1` is `age ?? (x + 1)`. `a ?? b ?? c` tries `a`, then `b`. When the right
  side is an optional too, the result is one.
* `x?.b` is a `?B`, unless `b` is an optional already, then it stays one optional. Each step of a chain needs
  its own `?.`: `user?.address?.zip`. A method that gives nothing can be called with `?.` too, e.g `file?.close()`.
* `?.` works on pointers as well, where `null` is empty: `node?.next?.value`.
* You can't assign through `?.`, and `??int` (an optional of an optional) is not allowed.
* A type parameter is inferred through an optional: `unwrapOr :: <T>(x: ?T, fallback: T) -> T` called with
  `unwrapOr(5, 0)` has `T = int`.

## In the standard library

* `fmt` prints the value, or `null`.
* `json` writes the value or `null`, and reads `null` as empty. `omitEmpty` leaves out empty optionals.
* `dbms` reads `NULL` as empty, e.g `email: ?string`, and an empty optional parameter is `NULL`.
* `list.tryFirst()`, `list.tryLast()`, `map.tryGet(key)` and `xs.find(f)` give optionals, e.g
  `ages.tryGet("bob") ?? 0`.
* Through reflection: `type.isOptional()`, `optionalValue(data)` and `setOptional(data, present)` on a
  `TypeStruct`.

## How it works

* The parser reads `?T` as the standard generic instance `Optional<T>`, like `[..]T` is `List<T>`. `?.` is a
  member access with `MEMBER_ACCESS_FLAGS_OPTIONAL_CHAIN`, and `??` a binary operator.
* `types_are_equal(?T, T)` and `types_are_equal(?T, null)` are true, so a `T` is accepted where a `?T` is
  wanted. The backend wraps it where it is stored (`convert_aggregate()` in `ir_lower.c`): the value, and
  `present = true`, or all zeros for `null`. An empty optional is all zeros, so `==` compares the memory.
  Arguments and returns may also narrow (`types_fit_either_way()`), but never from `?T` to `T`.
* `a?.b` and `a?.m(x)` are replaced by an `AST_OPTIONAL_CHAIN` (`check_optional_chain()` in `typer.c`). Its
  `inner` is the same access or call on an `AST_OPTIONAL_VALUE`, which stands for the value inside `a`. The
  backend branches on `present` (or the pointer), lowers `inner` only when there is a value, and wraps it.
* `a ?? b` branches the same way, so `b` is only computed when `a` is empty.
* Reflection marks optionals with `OptionalFlag` in `Type.flags`, like unions with `UnionFlag`.
