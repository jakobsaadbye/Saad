# Interfaces

An interface is a set of methods. Any type that has them is one, without saying so, like in Go. A value of an
interface is a pointer to the value and the methods of its type, so code can work with any of them, and a library can
add its own types that work with the code of others.

```odin
Shape :: interface {
    area :: method () -> f64;
    name :: method () -> string;
}

Circle :: struct {
    radius: f64;

    area :: method () -> f64 { return 3.14159 * self.radius * self.radius; }
    name :: method () -> string { return "circle"; }
}

Square :: struct {
    side: f64;

    area :: method () -> f64 { return self.side * self.side; }
    name :: method () -> string { return "square"; }
}

// Any Shape: a Circle, a Square, or a type of another library that has area and name
describe :: (s: Shape) {
    print("a % of %", s.name(), s.area());
}

main :: () {
    c := Circle{radius: 2};
    q := Square{side: 3};
    describe(&c);                         // a *Circle is a Shape, as Circle has area and name
    describe(&q);

    shapes: [..]Shape = [&c, &q];
    total: f64 = 0.0;
    for s in shapes { total += s.area(); }
}
```

`Circle` and `Square` don't name `Shape`. They are Shapes because they have its methods, with the same parameters
and results. A struct of a library that doesn't know `Shape` is one too, if it has them.

## Declaring one

* `Name :: interface { ... }` with the methods, as in a struct but without bodies: `name :: method (...) -> T;`, and
  `name :: method * (...);` for one that changes `self`. A body is an error.
* Only methods, no members.
* `Name :: interface <T> { ... }` is a generic interface, like a generic struct (see below).

## Being one

* A type is an interface when it has a method of each name, with the same parameters and results. The compiler checks
  it where a value becomes the interface, and the hint says why one isn't:

  ```
  error: Type mismatch. Wanted type Shape, got type *Circle
  hint: Circle isn't a Shape: it has no method 'name'
  ```

* The methods declared inside a struct count. So do methods declared outside it that take a pointer, e.g
  `area :: method (t: *Triangle) -> f64`. One that takes its value, e.g `area :: method (t: Triangle)`, doesn't, as
  the interface calls it with a pointer, and the hint says so.

## A value of an interface

* An interface value is two pointers, 16 bytes: the value, and a table of the methods of its type for the interface.
  The compiler makes one table for each type and interface that meet, in read-only data.
* Only a pointer becomes an interface, and the `&` is written: `describe(&c)`. So the code shows where a value is
  shared, nothing is copied, and nothing is allocated. `describe(c)` is an error:

  ```
  hint: c is a Circle, not a pointer: pass &c to share it as a Shape
  ```

* A `method *` called through the interface changes the value itself.
* The zero value is no value: `s: Shape;` is null and `s == null` tells. A null pointer becomes a null interface, so
  `s == null` tells for it too. A call of a method of a null interface throws a `Panic`, like a null pointer:
  `Null pointer dereference: s is null`.
* A call `s.area()` loads the method from the table and calls it with the value as `self`: one load more than a
  direct call.
* Interface values can be stored, e.g in a list or a struct member, passed and returned, like any other value. The
  value they point at must live as long, as for any pointer. `--check-memory` finds one that points into an arena that
  was cleared (`docs/check_memory.md`).

### Sizes

The methods aren't in the values. A `Circle` with its two methods is 8 bytes, as without them, also when it is used
as a `Shape`. The table is there once for each type and interface, and the interface value points at it:

```
c: Circle                 s: Shape                     the table of Circle for Shape (once in the program)
┌──────────────┐          ┌──────────────┐             ┌────────────────────┐
│ radius: f64  │ ◄─────── │ value        │             │ type → Circle      │
└──────────────┘          │ table ───────┼──────────►  │ area → Circle.area │
  8 bytes                 └──────────────┘             │ name → Circle.name │
                            16 bytes                   └────────────────────┘
```

So only a value of an interface costs something, and a list of a million circles is 8 MB. In C++ an object of a class
with virtual methods has the pointer to its table in it. Here, like in Go and in Rust's `dyn`, it is in the interface
value.

## Getting the type back

The table has the type of the value, so the value can be had with its own type again:

```odin
grow :: (s: Shape) {
    // When one type is expected: as. Another type throws a Panic
    circle := s as *Circle;
    circle.radius *= 2;
}

growIfCircle :: (s: Shape) {
    // With a second name, as takes the miss: ok is false and circle is null, and nothing is thrown
    circle, ok := s as *Circle;
    if !ok { return; }
    circle.radius *= 2;
}

describeExact :: (s: Shape) {
    // When it can be one of many: switch, as for unions. In each case v has the type of the case
    switch v in s {
        case *Circle { print("a circle of radius %", v.radius); }   // v: *Circle
        case *Square { print("a square of side %", v.side); }       // v: *Square
        else         { print("another shape: %", s.name()); }       // e.g a type of another library
    }
}

main :: () {
    c := Circle{radius: 2};
    q := Square{side: 3};
    describeExact(&c);       // a circle of radius 2
    describeExact(&q);       // a square of side 3
    growIfCircle(&q);        // nothing: it is a Square
    grow(&c);                // c.radius is 4 now: s points at c itself
    grow(&q);                // Panic: The Shape is a *Square, not a *Circle
}
```

* It is the same choice as for errors (`docs/errors.md`): `circle := s as *Circle` throws a `Panic` when it is
  another type or null, and `circle, ok := s as *Circle` takes it at the call, with `ok` false and `circle` null.
* `as` gives a pointer, and a type that can never be the interface is an error: `A Shape is never a *Plain`.
* `switch v in s` is the switch of unions (`docs/switch_and_unions.md`) with a pointer type in each case. A case with
  more than one type gives `v` as the interface. A switch expression on an interface needs an `else`, as any type can
  be one.
* `print(s)`, and an `any` of an interface, show the value itself, e.g `Circle{radius: 2.000000}`.

## As a constraint of a generic

An interface also constrains a type parameter. Then nothing is looked up at run time: the function is made for each
type, and the calls are direct:

```odin
totalArea :: <S: Shape>(shapes: []S) -> f64 {
    total: f64 = 0.0;
    for *s in shapes { total += s.area(); }    // a direct call of Circle.area for a []Circle
    return total;
}
```

* `S` is the type of the values, e.g `Circle`, and a pointer to it must be the interface.
* A type that isn't is an error at the call, instead of an error inside the generic function:

  ```
  error: 'S' of 'totalArea' must be a Shape, but it is Plain
  hint: Plain isn't a Shape: it has no method 'area'
  ```

* Use an interface value where different types are mixed or kept, e.g a list of Shapes or the allocator of a list.
  Use a constraint where one type is used and speed matters.

## Generic interfaces

An interface can have type parameters, like a struct. Its methods use them:

```odin
Source :: interface <T> {
    next :: method * () -> ?T;
}

Countdown :: struct {
    from: int;
    next :: method * () -> ?int {
        if self.from == 0 { return null; }
        self.from -= 1;
        return self.from + 1;
    }
}

drain :: (source: Source<int>) -> [..]int { ... }

down := Countdown{from: 3};
drain(&down);                        // [3, 2, 1]
```

* `Source<int>` is an interface whose `next` gives a `?int`. A generic struct is one too, with its own type
  arguments, e.g a `List<int>` is an `Iterable<int>`.
* A generic interface is a constraint too: `largest :: <C: Iterable<int>>(c: *C) -> int`.
* A method of an interface can't have type parameters of its own.

## The collection protocol

The collection protocol (`docs/collections.md`) is a set of generic interfaces of the prelude
(`std/prelude/collections.sd`). A collection doesn't name them: it is each one whose methods it has.

| Interface | Method | The syntax it gives |
| --- | --- | --- |
| `Iterable<T>` | `elements :: method () -> []T` | `for x in c`, `c` where a `[]T` is wanted |
| `Growable<T>` | `add :: method * (value: T)` | `[a, b, c]` where the collection is wanted |
| `Indexable<K, V>` | `indexRef :: method (key: K) -> *V` | `c[k]`, `c[k] = v` |
| `Keyed<K, V>` | `set :: method * (key: K, value: V)` | `[k1: v1, k2: v2]` where the collection is wanted |

So a `[..]int` is an `Iterable<int>`, a `Growable<int>` and an `Indexable<i64, int>`, and a `Map<string, int>` is an
`Iterable<Entry<string, int>>` and a `Keyed<string, int>`. Code can take any collection, and the syntax works on the
interface values too:

```odin
total :: (xs: Iterable<int>) -> int {
    sum := 0;
    for x in xs { sum += x; }        // xs.elements() through the table
    return sum;
}

bumpFirst :: (xs: Indexable<i64, int>) {
    xs[0] += 100;                    // *xs.indexRef(0)
}

list: [..]int = [1, 2, 3];
total(&list);                        // 6
bumpFirst(&list);                    // list[0] is 101
```

A collection of a library works with them as well, as long as it has the methods.

## Not supported yet

* Embedding an interface in another, e.g a `ReadWriter` of a `Reader` and a `Writer`. List the methods instead.
* The methods of an embedded struct member (`docs/embedding.md`) don't count yet for the outer struct.
* One interface as another, e.g a `ReadWriter` as a `Reader`. Make the value of the other from the pointer instead.
* An interface as a constraint of a generic struct, e.g `Box :: struct <T: Shape>`. Constraints are checked for
  generic functions.
* "Go to implementations" in the language server.

## How it works

* The parser makes an interface an `AstStruct` with `is_interface`, like a union is one with `is_union`. Its members
  are the hidden `$value` and `$table`, and its methods are `AstFunctionDefn`s with `is_interface_method` and no body.
  So a generic interface is a generic struct, and copying, storing and passing one is as for any struct.
* The typer checks that a type is an interface in `types_are_equal()` (`interface_mismatch()`), so it is checked
  wherever a value is wanted. A method of a generic interface or of a generic struct is the instance of the generic
  method for the type arguments (`method_of_instance()`). `x, ok := s as *T` has the type `(*T, bool)`, and a switch
  on an interface is a chain of ifs of `is_type_test` casts (`check_interface_switch()`).
* The IR lowering makes an interface value of a pointer (`make_interface_value()`), and the table of a type for an
  interface (`interface_table_global()`): the descriptor of the type, then the methods in the order of the interface.
  A call of a method of an interface loads it from the table and calls it through the pointer, so both backends get
  it from the IR.
