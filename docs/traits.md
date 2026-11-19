# Traits

A design, not implemented yet. Traits would replace interfaces (`docs/interfaces.md`) as the way to say what a type
can do. Two things are wanted of them:

* **A contract.** A type says which traits it implements, and the compiler checks it there: a missing method is an
  error at the implementation, not at a use far away.
* **Static dispatch.** Code that works for any type of a trait is made for each type that uses it, so its calls are
  direct and can be inlined, like the constraints of generics today (`<S: Shape>`).

Dynamic dispatch, today's interface values, is left open: see the end.

## Declaring a trait

```odin
Hash :: trait {
    hash :: method () -> u64;
}

Shape :: trait {
    area :: method () -> f64;                        // Required
    name :: method () -> string { return "shape"; }  // A default, which a type can replace

    unit :: () -> Self;                              // No self: called on the type, e.g Circle.unit()
}

Iterable :: trait <T> {                              // A generic trait
    next :: method * () -> ?T;
}

Comparable :: trait implements Equals {              // A Comparable must be an Equals too
    compare :: method (other: Self) -> int;
}
```

* A trait has methods, as in a struct: `name :: method (...) -> T;` without a body is required, and with a body it is
  a default. `method *` is a method that changes `self`.
* `Self` is the type that implements the trait.
* A function without `self` is called on the type: `Circle.unit()`, or `S.unit()` in generic code.
* `implements` after `trait` names the traits that a type of this one must implement too.
* Only methods and functions, no members: a trait says what a type can do, not what it has.

## Implementing a trait

A type implements traits in a block of its own, apart from its declaration:

```odin
Vector2 :: struct {
    x, y: f64;
    length :: method () -> f64 { ... }               // Its own methods stay in the struct
}

Vector2 implements Hash, Comparable {
    hash    :: method () -> u64 { ... }
    compare :: method (other: Vector2) -> int { ... }
    equals  :: method (other: Vector2) -> bool { ... }   // Comparable implements Equals
}

geometry.Circle implements Shape { ... }             // A type of another namespace
Status implements Display { ... }                    // Enums and unions too
Marker implements Serializable {}                    // A trait with only defaults, or none
```

* The block is `Type implements A, B { ... }`. Parentheses around the traits are allowed and not needed, like around
  return types: `Vector2 implements (Hash, Comparable) { ... }`.
* The methods have an implicit `self`, as inside a struct, which is a pointer to the value. `method *` changes it. A
  method looks the same in a struct and in a block, so it can be moved between them.
* A type can have many blocks, e.g one per trait or one per file. It implements a trait at most once.
* There is one way to implement a trait: the block. The declaration of a type doesn't change when it gets a trait,
  and each block shows what a trait costs.

### Generic types and traits

```odin
List<T: Hash> implements Hash { ... }                // A List<T> is Hash when its T is
List<T> implements Iterable<T> { ... }               // Every List<T>
Range implements Iterable<int> { ... }               // A generic trait for one type argument
Map<K, V> implements Display { ... }
```

* After a generic type, the angle brackets name its own type parameters, by position, with what they must implement.
  Each name is new, whatever types are in scope, and every parameter is listed (`Map<K, V>`, not `Map<K>`).
* A block for one instance only, e.g `List<int> implements Display`, can't be written. It can be added later with a
  syntax of its own.

### What is checked

At the block, with an error that points at it:

* every required method of each trait (and of the traits they implement) is there, with the parameters and results
  of the trait;
* every method of the block is in one of its traits. A method that isn't is likely a typo;
* the traits that a trait requires are implemented, in this block or another.

```
error: Vector2 doesn't implement Comparable: it has no method 'compare'
  note: Comparable requires compare :: method (other: Self) -> int
```

### Coherence

A block must be in the namespace of the trait or of the type. So a namespace can implement its own traits for any
type, e.g its `Display` for the prelude's `string`, and any trait for its own types, e.g the prelude's `Hash` for its
`Vector2`. Two namespaces that know nothing of each other can't both implement the same trait for the same type,
which would conflict in a program that uses both.

As a block is with the trait or the type, it is part of every program that uses either: there is nothing to import
to get it.

## Using traits

### The methods are the type's methods

The methods of a block, and the defaults of its traits, become methods of the type: `v.hash()` works anywhere, as a
method declared in the struct does. So a type has one set of method names:

* a method of the struct and a method of a block with the same name is an error;
* two traits of a type that both have a method of the same name is an error at the second block. A call that says
  which trait, like Rust's `Shape::name(&v)`, can come later if a case needs it.

Blocks are registered with the methods of their file before any function is checked (`register_methods_early()`). So
order doesn't matter: a method in `Vector2` can call `self.compare(other)` of a block further down, or in another
file, and a `Vector2` can be used where a `Comparable` is required.

### Bounds: static dispatch

```odin
largest :: <T: Comparable>(values: []T) -> T { ... }
dedupe  :: <T: Hash + Equals>(values: []T) -> [..]T { ... }
fresh   :: <S: Shape>() -> S { return S.unit(); }

Map :: struct <K: Hash + Equals, V> { ... }
```

* `<T: Trait>` requires that T implements the trait, and `+` requires more than one. The bound is checked where the
  type argument is given, e.g at a call, with the trait that isn't implemented in the error.
* Inside, the methods and functions of the trait can be called on T. The function is made for each type it is used
  with, so the calls are direct, like generics today.

## Traits of the standard library

What the prelude would declare, and implement for its numbers, strings and collections:

| Trait | For |
| --- | --- |
| `Equals` | `==` and `!=`, the keys of `Map` |
| `Hash` | the keys of `Map`, e.g `Map<K: Hash + Equals, V>`, with a clear error for a key that can't be hashed |
| `Comparable` | `<`, `sort`, `largest` |
| `Iterable<T>` | `for x in c`, for any type that implements it |
| `Display` | how `print` writes a value |

The collection protocol of the prelude (`Iterable<T>`, `Growable<T>`, `Indexable<K, V>`, `Keyed<K, V>`) becomes
traits. A struct that wants the defaults of `Equals` and `Hash` from its members could ask for them with an attribute
later, e.g `@derive(Equals, Hash)`, made with reflection.

## Compared with Rust and Mojo

| | Saad traits | Rust | Mojo | Saad interfaces today |
| --- | --- | --- | --- | --- |
| How a type gets one | `Type implements T { }` | `impl T for Type { }` | `struct Type(T):` | By having the methods |
| Checked | At the block | At the impl | At the struct | Where a value becomes one |
| For a type of someone else | Yes, with coherence | Yes, with the orphan rule | No (extensions are planned) | Yes, it just has the methods |
| Default methods | Yes | Yes | Yes | No |
| Static dispatch | `<T: Trait>` | `<T: Trait>`, `impl Trait` | `[T: Trait]` | `<S: Shape>` |
| Dynamic dispatch | Open (see below) | `dyn Trait` | No (`Variant`) | Interface values |

## Not decided yet

* **Dynamic dispatch.** A value of any type of a trait, chosen at run time, e.g a list of different shapes, the
  allocator of a list, or a `dbms.Driver`. Today these are interface values: a pointer to the value and a table of the
  methods of its type. Whether that stays, as `dyn Trait` or another way, is open. Until it is decided, interfaces
  stay as they are, and the prelude's `Allocator` and `dbms.Driver` and `dbms.Rows`, which are used as values, stay
  interfaces.
* **A shorthand for bounds**, e.g `describe :: (s: *Shape)` for `<S: Shape>(s: *S)`. It depends on what a trait name
  as a type means, which is part of the question of dynamic dispatch.
* **Constants and types of a trait**, e.g a `Sides :: int;` that each type gives, or the type of the items of
  `Iterable` as a member instead of a parameter.
* **A call that names its trait**, for two traits with the same method.
* **A block for one instance**, e.g `List<int> implements Display`.

## In the compiler

What exists and what is new, for when it is implemented:

* The check that a type has the methods of an interface (`interface_mismatch()` in `typer.c`) becomes the check of a
  block, and runs when the block is checked instead of where a value is converted.
* Constraints of type parameters are checked and monomorphized already (`<S: Shape>`), so bounds need the check
  against implementations, `+`, and calls of functions on T (`S.unit()`).
* New: the `trait` declaration, `Self`, the `Type implements ... { }` block at the top of a file (a name, maybe
  dotted, followed by the keyword), its registration in `register_methods_early()`, default methods (made for each
  type that implements the trait, like a generic method), the checks of a block, coherence, and the errors for names
  of methods that clash.
* `trait`, `implements` and `Self` become keywords. No code of the repository uses them as names.
