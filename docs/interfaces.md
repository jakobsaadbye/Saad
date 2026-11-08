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
* Only a pointer becomes an interface: `describe(&c)`, not `describe(c)`. So making an interface value never copies
  the value and never allocates, and a `method *` changes the value itself. Where the value lives is up to you, e.g
  the stack, an arena or the heap.
* The zero value is no value: `s: Shape;` is null, `s == null` tells, and a call of a method of a null interface
  throws a `Panic`, like a null pointer.
* A call `s.area()` loads the method from the table and calls it with the value as `self`: one load more than a
  direct call.

### Getting the type back

* `s as *Circle` is the value, like taking a value out of an `any`, and it throws a `Panic` when it is another type.
* `switch v in s`, as for unions (`docs/switch_and_unions.md`), with a type in each case instead of a variant, and
  `v` of that type:

  ```odin
  switch v in s {
      case *Circle { v.radius *= 2; }
      case *Square { v.side *= 2; }
      else         { }                 // other Shapes, e.g of other libraries
  }
  ```
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

## Questions

* The name: `interface`, like Go, or `protocol`, which the docs already use for the collection protocol? Or should
  the collection protocol become interfaces?
* Should the value of an interface be allowed to be a non-pointer when it is small, e.g an `int` with methods, by
  copying it into the interface? This proposal says no, so it never allocates.
* `s as *Circle` throwing, or giving an optional like `?*Circle`?
* Generic interfaces, e.g `Iterator<T> :: interface { next :: method * () -> ?T; }`: now, or after the first version?
