# Reflection and attributes

## Type descriptors

A value converted to an `any` carries a pointer to the descriptor of its type. The compiler makes one constant
descriptor per type (`type_info_global()` in `ir_lower.c`), with the layouts of the structs in
`std/prelude/runtime.sd`. So the same type always has the same descriptor, and they cost nothing at runtime.

| Kind | Descriptor | What it has |
| --- | --- | --- |
| `.bool`, `.int`, `.float`, `.void` | `TypePrimitive` | `kind`: `.u8`, `.i64`, `.f32`, ... |
| `.string`, `.function`, ... | `Type` | `name`, `kind`, `size` |
| `.pointer` | `TypePointer` | `pointerTo` |
| `.array` | `TypeArray` | `kind` (`.fixed` or `.slice`), `elemType`, `count` of a fixed array |
| `.enum` | `TypeEnum` | `backingType`, `values: []EnumValue` (name and value), `nameOf(v)`, `readValue(data)` |
| `.struct` | `TypeStruct` | `members: []StructMember` (name, type, offset, index, attributes), `alignment` |
| `.struct` with `isUnion()` | `TypeStruct` | A union: the members are `$tag` (the kind, an enum) and the variants. `variantOf(data)` and `setVariant(data, variant)` |

```odin
x: any = person;
type := x.type as *TypeStruct;
for member in type.members {
    value: any;
    value.type = member.type;
    value.data = member.valueIn(x.data);   // the address of the member in the value
    fmt.print("% = %", member.name, value);
}
```

* `name` is the name of the type as `typeof` gives it, e.g `game.Vec2` or `List<int>`, so `a.type.name == typeof(Vec2)`
  checks the type of an `any`.
* A fixed array is stored in the `any`. A slice is `{data, count}`, so its count is in the value.
* A collection like `List<T>` becomes a slice of its elements when converted to an `any`. Convert a pointer to it
  to keep its type.

## Attributes

An attribute is a struct marked with `@attribute`. Its values can be given to struct members, and are read
through reflection, e.g for JSON, a database or a UI:

```odin
@attribute
Column :: struct {
    name:     string;
    nullable: bool = false;
}

Row :: struct {
    @Column(name: "row_id")
    id: int;

    @Column("label", true) @Ignore
    label: string;
}

for member in (rowAny.type as *TypeStruct).members {
    column: Column;
    if attributeOf(&member, &column) {
        // column.name, column.nullable
    }
    for attribute in member.attributes {   // []any
        if attribute.type.name == typeof(Ignore) { ... }
    }
}
```

* The arguments are written like a struct literal, with `()` instead of `{}`. Without arguments, e.g `@Ignore`,
  the parentheses can be left out. Members without an argument get their default value.
* The arguments must be known when compiling: literals, enum values and constants. The values are constants in
  the program, made by the compiler.
* The attributes of the language (`@private`, `@deprecated`, `@attribute`) go on declarations. Attribute values go
  on struct members, and on functions and methods (below).

### On functions

Attribute values on functions and methods are listed by `attributedFunctions()`, e.g to register the routes of a
server without listing them by hand:

```odin
@attribute
Route :: struct { verb: string = "GET"; path: string; }

@Route("POST", "/posts")
createPost :: (request: *Request) { ... }

for f in attributedFunctions() {     // []AttributedFunction: name, namespaceName, function and attributes
    route: Route;
    if f.attribute(&route) {
        handler := f.function as (request: *Request);   // The address, cast to the type the program knows
        handler(&request);
    }
}
```

* The compiler writes the list, and compiles the functions on it, when the program uses `attributedFunctions()`.
* A generic function can't have attribute values, as it is a function for each set of types.
* `*void` and a function type can be cast to each other, as a function value is its address. The program must give
  the right type.
* A function can also be called without knowing its type: `f.parameters` and `f.results` have the name, `*Type` and
  offset of each parameter and result, and for a parameter with a default, `hasDefault` and the `defaultValue` when it
  is a constant, and `f.invoke(arguments, results)` calls it with the arguments in a block of
  `f.argumentsSize` bytes, laid out like a struct of its parameters, and stores its results in a block of
  `f.resultsSize` bytes. It throws what the function throws. The compiler makes an `invoke` for each function on the
  list.
* `reflect.Call` (`std/reflect.sd`) does that with values of any type, which are checked against the parameters, so a
  program doesn't write to offsets in the blocks. `web` calls its endpoints like this:

```odin
import reflect;

call := f.newCall();               // the blocks, from the allocator of the thread, or f.newCall(&arena)
call.set("name", "ada");           // by the name of the parameter
call.setAt(1, 7);                  // or its index. A number is converted when it fits, e.g 7 to an i32
call.invoke();                     // throws what the function throws
text := call.result(0);            // an any that points into the block of the results
err := call.error();               // the error the function gave as a result, or null
```

  * `set` and `setAt` take a value of the type of the parameter, a number that fits a number parameter (also an int
    for a float), or a `T` for a `?T`. Anything else throws a `reflect.WrongArgument`, e.g
    `The parameter count of describe is a i32, which can't be given a string`. A parameter that isn't set is zero.
  * `setFrom(index, data)` copies the value at `data` without a check, for a value whose type the program knows only as
    the type of the parameter, e.g the session of `web`. `argument(index)` is an `any` of the place of an argument, to
    read a value into, e.g `json.assignAt(value, call.argument(i))`, and `result(index)` one of a result, e.g
    `json.encodeAt(call.result(0))`.
* `anyAt(data, type)` of the prelude makes an `any` of the value at `data`, which has the type, e.g of a member found by
  reflection. It is the one place where a `*void` and a `*Type` are put together: the functions of the standard library
  that work on a value whose type is only known when the program runs take an `any`, e.g `json.encodeAt(value: any)`,
  `json.assignAt(v, place: any)` and `StringBuilder.addInteger(value: any)`, so the pointer and its type can't disagree.
  * What it costs (`tools/bench/call_bench.sd`, `--llvm`, on Linux): a call with three arguments set by index takes about
    20 ns more than writing the blocks by hand, by name about 30 ns, and a new `Call` from an arena about 50 ns more. A
    value of the type of the parameter is copied, and its type compared by pointer: nothing is boxed or allocated.
    For a request of a web server, which takes microseconds, that is too small to measure.
