# Reflection and attributes

## Type descriptors

A value converted to an `any` carries a pointer to the descriptor of its type. The compiler makes one constant
descriptor per type (`type_info_global()` in `ir_lower.c`), with the layouts of the structs in
`std/prelude/runtime.sd`. So the same type always has the same descriptor, and they cost nothing at runtime.
`typeInfo<T>()` gives the descriptor of a type, e.g `typeInfo<User>()`.

Every descriptor is a `Type`: `name`, `kind`, `size` and `flags`. The kinds with more to them have a struct of their own
that embeds it (`@embed base: Type`), so it has the members and methods of a `Type` and is one where a `*Type` is
wanted. `asPointer()`, `asArray()`, `asEnum()` and `asStruct()` give it, or null for another kind:

| Kind | Descriptor | What it has |
| --- | --- | --- |
| `.bool`, `.int`, `.float`, `.string`, `.void`, `.any` | `Type` | An integer is told apart by its `size`, `isSigned()` and `isChar()`, e.g a `u8` has size 1 and isn't signed |
| `.pointer` | `TypePointer` | `pointerTo` |
| `.array` | `TypeArray` | `arrayKind` (`.fixed` or `.slice`), `elemType`, `count` of a fixed array |
| `.enum` | `TypeEnum` | `backingType`, `values: []EnumValue` (name and value), `nameOf(n)`, `valueNamed(name)`, `flagNames(n)` |
| `.struct` | `TypeStruct` | `members: []StructMember` (name, type, offset, index, attributes), `alignment`, `attributes`, `member(name)` |

The questions a library asks about a type are methods of `Type`, so it doesn't look at names or layouts:

| | |
| --- | --- |
| `is<T>()` | the type is `T`, e.g `type.is<time.Time>()`. It compares descriptors, so a type of another namespace with the same name isn't `T`. `value.is<T>()` of an `any` is the same |
| `isInteger()`, `isFloat()`, `isNumber()`, `isSigned()`, `isChar()` | the numbers. `integerRange()` is the smallest and the largest value, e.g -128 and 127 |
| `isOptional()`, `isUnion()`, `isFlags()`, `isList()`, `isMap()`, `isSlice()` | from `flags`, which the compiler sets |
| `isSequence()` | a fixed array, a slice or a List, which have elements |
| `pointee()`, `elementType()`, `optionalType()`, `keyType()`, `valueType()` | the types inside, e.g `int` of `*int`, `[..]int`, `?int` and `Map<string, int>`. Null for other types |
| `TypeStruct.variant(name)` | the variant of a union with the name |

* `name` is the name of the type as `typeof` gives it, e.g `game.Vec2` or `List<int>`. It is for messages: a type is
  checked with `is<T>()`, as two types can have the same name.
* A fixed array is stored in the `any`. A slice is `{data, count}`, so its count is in the value.
* A collection like `List<T>` becomes a slice of its elements when converted to an `any`. Convert a pointer to it
  to keep its type, which `anyOf(&value)` and `typeInfo<T>()` do.

## Values

An `any` is the address of a value and its type, and its methods (`std/prelude/any.sd`) read and change the value
through the type, so a library doesn't cast `data` or write to offsets:

```odin
options: Options;
v := anyOf(&options);                       // options, as an any that changes it
v.member("verbose").set(true);
v.member("level").setInt(3);                // false when 3 doesn't fit the type of level
v.member("files").add().set("main.sd");     // a new element of a slice or a List
v.member("ages").put("bob").setInt(31);     // a key of a Map
v.member("mode").setEnum("fast");

for *member in v.type.asStruct().members {
    fmt.print("% = %", member.name, v.memberAt(member));
}
```

| | |
| --- | --- |
| `to<T>()`, `ref<T>()`, `is<T>()` | the value as a `T` and true, its address as a `*T`, or false and null when it isn't a `T` |
| `set(value)` | stores a value of the type, a number that fits, or the value of an optional. False when it can't |
| `toInt()`, `setInt(n)`, `setIntText(text)` | an integer or an enum. `setInt` is false when the number doesn't fit, e.g 300 in a `u8`. `setIntText` reads e.g "18446744073709551615" without an `f64` |
| `toFloat()`, `setFloat(f)` | a float, and `toFloat` an integer too |
| `enumName()`, `setEnum(name)`, `addFlag(name)` | an enum by the names of its values |
| `count()`, `element(i)`, `add()`, `resize(n)` | a fixed array, a slice or a List. `count` is also the entries of a Map and the bytes of a string |
| `entry(i)`, `put(key)` | the key and the value of an entry of a Map, and the value of a key, added when it isn't there. The keys are strings or integers |
| `member(name)`, `memberAt(member)` | a member of a struct |
| `deref()`, `allocate()` | the value a pointer points to, and a new value of zero for it |
| `optional()`, `setOptional()`, `clearOptional()` | the value of a `?T`, a value of zero to set, and none |
| `variant()`, `setVariant(name)` | the variant of a union and its value, and a variant to set |
| `zero()`, `isEmpty()` | |

* `x as any` boxes a value where no `any` is wanted, e.g `(5 as any).type.name` or `(user as any).member("name")`. Like
  an `x` given for an `any`, it is an `any` of a copy, so changing it doesn't change `x`: `anyOf(&x)` is one that does.
  A collection keeps its type, e.g `List<int>`, where an `x` given for an `any` becomes a slice.
* A method that gives an `any` gives an empty one when there is no such value, e.g `member` of a name the struct doesn't
  have, `element` out of range or `deref` of null. The methods of an empty `any` do nothing and give false or another
  empty one, so `v.member("a").member("b").setInt(1)` is safe, and false when a part is missing.
* A List grows with its own allocator. A slice doesn't own its memory, e.g it can be a fixed array, so `add` and
  `resize` give it a new block of the allocator of the thread, and leave the old one. To add many elements to a slice,
  `resize` it once.
* `to<T>` is for the exact type, so `to<int>` of an `i64` is false. `toInt` reads any integer.
* Methods can be declared on `any` like on other types, e.g `describe :: method (v: any) -> string`.

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

for *member in typeInfo<Row>().asStruct().members {
    column: Column;
    if attributeOf(member, &column) {
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
  on struct members, on structs, and on functions and methods (below).

### On structs

Attribute values on a struct are in `TypeStruct.attributes`, and `type.attribute(&value)` gets one of them, e.g what
`cli.parseArguments` says about the program:

```odin
@cli.Options(about: "Runs games of bots")
Options :: struct { ... }

about: cli.Options;
if typeInfo<Options>().asStruct().attribute(&about) { ... about.about ... }
```

An instance of a generic struct has the attribute values of the generic struct.

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
