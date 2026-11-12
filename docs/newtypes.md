# newtype

```odin
Meters :: newtype f64;
Feet   :: newtype f64;
UserId :: newtype i64;

toFeet :: method (m: Meters) -> Feet {
    return (m as f64 * 3.28084) as Feet;
}

walk :: (distance: Meters) { ... }

d: Meters = 2;          // a number written in the program is a number of the newtype
d = d * 2 + 0.5;        // the operators of f64, which give a Meters
walk(d);
walk(2.5);
f := d.toFeet();

raw: f64 = 3;
walk(raw);              // error: Wanted type Meters, got type f64
walk(raw as Meters);    // a cast says it is meant
d + f;                  // error: Cannot use a Meters with a Feet
```

`Name :: newtype Type;` declares a new type made of another. It has the values, the size and the operators of the type it is
made of, but it is only equal to itself, so a `Meters` can't be given where an `f64` or a `Feet` is wanted, or the other way.
Unlike an alias (`Name :: Type;`), which is another name for the same type.

* **Made of:** a number, a bool, a string, a pointer, an array, an enum or a struct (not an interface or a union), or another
  newtype.
* **Values:** a number written in the program is one of the newtype, e.g `d: Meters = 2` or `d == -4.0`, and `null` is a
  value of a newtype of a pointer. A newtype of an enum takes its values, e.g `p: Paint = .green`. A newtype of a struct is
  made with a literal of its name, e.g `Position{1, 2}`.
* **Operators:** the ones of the type it is made of, between two values of the newtype, or a value and a number written in
  the program. They give the newtype. The amount of a shift, e.g `id << 2`, can be any integer.
* **Casts:** `x as Meters` and `m as f64` cast between a newtype and the type it is made of, and between two newtypes of the
  same type, e.g `f as Meters`.
* **Methods:** a newtype has methods of its own, declared like any method, e.g `toFeet :: method (m: Meters) -> Feet`. It has
  none of the methods of the type it is made of: a newtype of a string has the members `len` and `data`, but not `hash`, and a
  newtype of a struct has the members of the struct, but not its methods or constants. To use one, cast, e.g
  `(n as string).hash()`, or give the newtype a method that does, e.g for the key of a `Map`:

```odin
hash :: method (id: UserId) -> u64 { return (id as i64).hash(); }
```

* **Generics:** a newtype is a type like any other, e.g `List<UserId>` or `biggest(a, b)` with `T = Meters`.
* **Printing and reflection:** `typeof(d)` is `"Meters"`, and a value prints like the type it is made of, e.g `2.000000`. A
  newtype of a struct prints with its own name, e.g `Position{x: 1.000000, y: 2.000000}`. JSON reads and writes it like the
  type it is made of.

## How it works

* The parser reads `Name :: newtype Type;` as a type alias with `DECLARATION_NEWTYPE`. `check_type_alias()` resolves the type
  and `make_newtype()` copies it: the copy has the kind and the size of the type, so the IR and the backends see an `f64`, and
  `Type.newtype` is the identifier that declares it, which is its identity. `Type.newtype_base` is the type it is made of.
* `types_are_equal()` finds two types with different `newtype`s unequal, except an untyped number or `null` for a newtype of
  a number or a pointer. `check_binary()` gives a number literal next to a newtype the newtype as its type, and rejects the
  rest. `can_cast_explicitly()` allows a cast between a newtype and its base.
* A newtype of a struct gets its own `AstStruct`, a copy that shares the members, with empty methods and constants. Another
  newtype keeps its methods in `Type.newtype_methods`, which `method_holder_of()` gives.
* `type_to_str()` and `type_identity()` give its name, so errors and `typeof` say `Meters`.
