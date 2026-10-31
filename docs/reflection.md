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
* The attributes of the language (`@private`, `@deprecated`, `@attribute`) go on declarations. Attribute values
  only go on struct members for now.
