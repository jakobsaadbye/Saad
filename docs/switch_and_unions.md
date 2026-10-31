# switch and unions

## switch

```odin
switch status {
    case .open            { ... }
    case .closed, .locked { ... }    // more values
}

switch score {
    case 90...100 { ... }            // a range, with 100
    case 80..90   { ... }            // a range, without 90
    case 0        { ... }
    else          { ... }            // nothing else matched
}

switch name {
    case "alice", "bob" { ... }      // anything that can be compared with ==
}

switch {                             // no value: each case is a condition, like a chain of if else
    case x < 0  { ... }
    case x == 0 { ... }
    else        { ... }
}
```

* The first case that matches runs, and nothing else. There is no fallthrough, so `break` and `continue`
  are for the loop around the switch.
* The value is evaluated once.
* `else` must be the last case.
* A switch on an enum without an `else` must handle every value of the enum. The compiler lists the ones
  that are missing.
* A value that is handled twice is an error.

The typer turns a switch into a hidden local with the value and a chain of ifs (`check_switch()`), which is
what is lowered.

## Unions

A union holds one of its variants. Each variant has a name and a type, or no type:

```odin
Shape :: union {
    circle: Circle;
    rect:   struct { w, h: f32; };
    empty;                            // no value
}

a := Shape{circle: Circle{radius: 2}};
b := Shape{rect: {w: 3, h: 4}};
c := Shape.empty;
d: Shape = .empty;                    // .name when a Shape is wanted, e.g return .none;
```

The value of a variant is read with a switch. `switch v in shape` gives the value a name, with the type of
the variant in each case:

```odin
area :: (s: Shape) -> f32 {
    switch v in s {
        case .circle        { return Pi * v.radius * v.radius; }  // v: Circle
        case .rect          { return v.w * v.h; }
        case .empty         { return 0; }                         // no v, as empty has no value
    }
    return 0;
}

grow :: (s: *Shape) {
    switch v in s {                   // on a pointer, v points to the value in the union
        case .circle { v.radius *= 2; }
        else         { }
    }
}
```

* A switch on a union without an `else` must handle every variant.
* In a case with more variants, e.g `case .rect, .circle`, `v` is the union itself.
* The zero value of a union holds its first variant, with a zeroed value. So `Option<T>` below is `.none`
  until it is given a value.
* Unions can be generic, have methods and be in lists and maps like structs. They can't be compared with
  `==`, and their variants can't be read as members, e.g `shape.circle`.
* `fmt` prints a union like an enum value with its value, e.g `.circle(Circle{ radius = 2 })`, and `std/json`
  writes it as `"empty"` or `{"circle": {"radius": 2}}`.

```odin
Option :: union <T> {
    none;
    some: T;
}

Result :: union <T, E> {
    ok:  T;
    err: E;
}

find :: (xs: []int, x: int) -> Option<int> {
    for v, i in xs {
        if v == x { return Option<int>{some: i}; }
    }
    return .none;
}
```

### How a union is stored

A union is a struct (`AstStruct.is_union`) with a hidden first member `$tag: u32`, the index of the variant
it holds, and the variants as the other members. The variants all start at the same offset, after the tag, so
a union is as big as the tag and its biggest variant. The descriptor of a union for reflection has
`UnionFlag` in `Type.flags`, and `type.variantOf(data)` gives the variant a value holds.

## Not done yet

* Reading the tag without a switch, e.g `shape.is(.circle)` or `shape.kind`
* Variants of a union in `case` without a switch on a union, e.g `if shape == .empty`
