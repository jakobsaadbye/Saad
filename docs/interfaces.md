# Interfaces (a proposal)

This is a design note: nothing of it is in the compiler yet. It is for deciding the syntax and the rules first.

An interface is a set of methods. Any type that has them is one, without saying so, like in Go. A value of an
interface is a pointer to the value and the methods of its type, so code can work with any of them, and a library can
add its own types that work with the code of others.

Today Saad does this by hand, with a `*void` and function pointers, e.g `Allocator`. The compiler can't check that the
`*void` is what the functions expect, and every implementation casts it back:

```odin
Allocator :: struct {
    context: *void;
    resize: (context: *void, memory: *void, size: i64) -> *void;
}

arenaResize :: (context: *void, memory: *void, size: i64) -> *void {
    arena := context as *Arena;    // Nothing checks that context is an Arena
    ...
}
```

## A small example

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
    total := 0.0;
    for s in shapes { total += s.area(); }
}
```

`Circle` and `Square` don't name `Shape`. They are Shapes because they have its methods, with the same parameters
and results. A struct of a library that doesn't know `Shape` is one too, if it has them.

## The rules

### Declaring one

* `Name :: interface { ... }` with the methods, as in a struct but without bodies: `name :: method (...) -> T;` and
  `name :: method * (...);` for one that changes `self`.
* Only methods. No members, and no bodies.
* `@embed` of another interface adds its methods, as for structs (`docs/embedding.md`):

  ```odin
  Reader :: interface { read :: method * (into: []u8) -> int, error; }
  Writer :: interface { write :: method * (bytes: []u8) -> int, error; }
  ReadWriter :: interface {
      @embed reader: Reader;
      @embed writer: Writer;
  }
  ```

### Being one

* A type is an interface when it has a method of each name, with the same parameters and results. The compiler checks
  it where a value becomes the interface, and the error says which method is missing or different:
  `*Circle isn't a Shape: it has no method 'name'`.
* A method of an embedded member counts, so a struct that embeds a `Circle` is a Shape too.
* Methods of primitive types and enums count, e.g `doubled :: method (x: int) -> int`.

### A value of an interface

* An interface value is two pointers: the value, and a table of the methods of its type for the interface. The
  compiler makes one table for each type and interface that are used together, in the program's read-only data.
* An interface value points at the value, so it never copies it and never allocates, and a `method *` changes the
  value itself. Where the value lives is up to you, e.g the stack, an arena or the heap.
* Only a pointer becomes an interface, and the `&` is written: `describe(&c)`. So the code shows where the value
  is shared. `describe(c)` is an error that says so: `c is a Circle, not a pointer: pass &c to share it as a Shape`.
* A value is never copied into an interface, also not a small one like an `int` with methods.
* The zero value is no value: `s: Shape;` is null, `s == null` tells, and a call of a method of a null interface
  throws a `Panic`, like a null pointer.
* A call `s.area()` loads the method from the table and calls it with the value as `self`: one load more than a
  direct call.

### Sizes

The methods aren't in the values. A `Circle` with its two methods is 8 bytes, as without them, also when it is used
as a `Shape`. The table of the methods is there once for each type and interface, in read-only data, and the
interface value points at it:

```
c: Circle                 s: Shape                     the table of Circle for Shape (once in the program)
┌──────────────┐          ┌──────────────┐             ┌────────────────────┐
│ radius: f64  │ ◄─────── │ value        │             │ area → Circle.area │
└──────────────┘          │ table ───────┼──────────►  │ name → Circle.name │
  8 bytes                 └──────────────┘             │ type → Circle      │
                            16 bytes                   └────────────────────┘
```

So only a value of an interface costs something, 16 bytes, and a list of a million circles is 8 MB. In C++ an object
of a class with virtual methods has the pointer to its table in it. Here, like in Go and in Rust's `dyn`, it is in
the interface value.

### Getting the type back

The table has the type of the value, so the value can be had with its own type again:

```odin
grow :: (s: Shape) {
    // When one type is expected: as. Another type throws a Panic
    circle := s as *Circle;
    circle.radius *= 2;
}

growIfCircle :: (s: Shape) {
    // With a second result, as takes the miss: ok is false and circle is null, and nothing is thrown
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
    grow(&q);                // Panic: Shape is a *Square, not a *Circle
}
```

* `s as *Circle` compares the type in the table with `Circle`, and gives the pointer. It is like taking a value out
  of an `any`.
* It is the same choice as for errors (`docs/errors.md`): `circle := s as *Circle` throws a `Panic` when it is
  another type, and `circle, ok := s as *Circle` takes it at the call, with `ok` false and `circle` null instead.
* `switch v in s` is the switch of unions (`docs/switch_and_unions.md`) with a type in each case instead of a
  variant.
* The table has the `*Type` of the value, so `print(s)`, `json.encode(s)` and the debugger show the value itself.

### As a constraint of a generic

The same interface constrains a type parameter. Then nothing is looked up at run time: the function is made for
each type, and the calls are direct:

```odin
totalArea :: <S: Shape>(shapes: []S) -> f64 {
    total := 0.0;
    for *s in shapes { total += s.area(); }    // a direct call of Circle.area for a []Circle
    return total;
}
```

* Use an interface value where different types are mixed or kept, e.g a list of Shapes or the allocator of a list.
  Use a constraint where one type is used and speed matters.
* This also gives generics the constraints that `docs/generics.md` lists as not supported yet: an error at the
  call for a type that doesn't fit, instead of an error inside the generic function.

## Allocators with interfaces

The `Allocator` of the standard library would be the first user. The interface also passes the old size and the
alignment, which the allocator plan needs:

```odin
Allocator :: interface {
    // A new block when memory is null, a bigger or smaller one, or none when newSize is 0. Like realloc
    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void;
}

Arena :: struct {
    ...
    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void { ... }
}

// An allocator of a library: it works with lists, maps, strings, json and dbms, as it has resize
Counting :: struct {
    inner: Allocator;
    bytes: i64;

    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void {
        self.bytes += newSize - oldSize;
        return self.inner.resize(memory, oldSize, newSize, align);
    }
}

arena: mem.Arena;
counting := Counting{inner: &arena};
list: [..]int;
list.allocator = &counting;      // an Allocator
```

`memory: *void` stays: it is raw memory, which has no type.

## What it isn't

* No `implements` and no list of interfaces on a type. A type of a library works with an interface the library
  doesn't know.
* No inheritance and no bodies in an interface. Code that many types share is a function that takes the interface.
* No values of an interface that own their value. It points at it, and the value must live as long as the interface
  value, as for any pointer. `--check-memory` finds one that points into an arena that was cleared.

## How it would be built

* The parser: `interface { ... }` as a type, with the method declarations.
* The typer: the check that a type has the methods, the conversion of a pointer to an interface, the calls of its
  methods, `as` and `switch`, and the constraints of type parameters.
* The IR lowering: a table for each type and interface, in read-only data, made the first time they meet. A call
  is a load from the table and a call through the pointer, so both backends get it from the IR.
* Reflection: a `TypeInterface`, with its methods.
* The language server: hover, completion of the methods of an interface value, and later "go to implementations".

## Decided

* The name is `interface`.
* The `&` is written where a value becomes an interface, and a value is never copied into one.
* `as` throws a `Panic` when the type isn't the one asked for, and `x, ok := s as *T` takes the miss instead.
* Generic interfaces, e.g `Iterator<T> :: interface { next :: method * () -> ?T; }`, aren't in the first version.

## After the first version

* Generic interfaces.
* The collection protocol (`docs/collections.md`) as interfaces: e.g `Collection<T> :: interface { elements :: method
  () -> []T; }` and `Growable<T> :: interface { add :: method * (value: T); }`, with `for`, `[a, b, c]` and `xs[i]`
  defined by them instead of by names the compiler knows. Its methods take and give the type of the elements, so it
  needs generic interfaces and comes with them. Until then, the protocol stays as it is.
* "Go to implementations" in the language server.
