# Generics in the front-end

How generic structs, functions and methods are implemented. Everything happens in the parser and the
typer. By the time the program reaches `ir_lower.c`, every generic has been replaced by ordinary
structs and functions, so the IR, `regalloc.c` and `x64_emit.c` know nothing about generics.

## Generic structs (monomorphized by copying)

```odin
List :: struct <T> { elements: [..]T; count: int; }
numbers: List<int>;
```

* The parser stores the type parameters on `AstStruct.type_params`. `List<int>` in a type position becomes a
  `TYPE_GENERIC_INSTANCE` (a name plus type arguments). `scan_type_arguments()` looks ahead to tell `List<int>`
  apart from a less-than comparison.
* The generic struct itself is never checked or sized (`check_struct_defn` returns early).
* `resolve_type()` on a `TYPE_GENERIC_INSTANCE` calls `instantiate_generic_struct()`. The struct is copied
  with its member types deep copied by `copy_type()`, because `resolve_type()` resolves types in place. The copy
  sits below a scope that binds each type parameter to its type argument. It is then sized by the normal
  `check_struct_defn()`.
* Instances are cached on the generic struct by name, e.g `List<int>`. Structs are compared by name, so that
  name is the identity of the type.

## Generic functions (monomorphized by parsing again)

```odin
max :: <T>(elements: ...T) -> T {...}
max(3, 9, 4);   // instance max<int>
```

* The parser stores the type parameters on `AstFunctionDefn.type_params`, and remembers
  `first_token_index`, the first token of the function.
* The generic function is not checked (`check_generic_function_defn()` only checks what is known without
  type arguments). Every type parameter must be used by a parameter, as there is no syntax to give type
  arguments explicitly.
* At a call, `instantiate_generic_function_call()` infers the type arguments: `unify_types()` matches each
  parameter type as written (e.g `*Map<K, V>`) against the type of the argument (e.g `*Map<int, string>`)
  through pointers, arrays, variadics and generic struct instances.
  * Typed arguments must agree exactly (aliases like `int` and `i32` are the same type). Nothing is widened.
  * Untyped literals give way to typed arguments. Between untyped literals a float wins over an integer.
    Only untyped literals give the same type as `x := 1` and `x := 2.5`, which is `int` and `float`.
* `instantiate_generic_function()` makes an instance by **parsing the function again** from its tokens
  (`reparse_generic_function()`), below a scope that binds the type parameters. That gives each instance its
  own fresh AST without a deep copy of every kind of node. The instance is then checked like any other
  function, so all the state the typer puts on a function is per instance.
* Instances are cached on the generic function by name, e.g `max<int>`. They are added to the cache before
  they are checked, so recursion finds them. Nested instantiation stops at `GENERIC_DEPTH_LIMIT` (64).
* `call->func_defn` points to the instance, so the `lower_program()` worklist only lowers instances that
  are called. Their symbols are mangled from the type arguments, e.g `max$int`.
* A `>>` that closes two lists of type arguments is split without changing the token
  (`parser->split_shift_right_token`), as the tokens are parsed more than once.

## Methods

```odin
// Declared outside the struct
set :: method <K, V>(m: *Map<K, V>, key: K, value: V) {...}

// Declared inside the struct
Map :: struct <K, V> {
    get :: method (key: K) -> V {...}            // read-only self
    put :: method * (key: K, value: V) {...}     // mutable self
}
```

* A generic method is added to the methods of the generic struct (`register_generic_method()`). Its receiver
  must be the generic struct with exactly the method's type parameters as type arguments, each used once, so
  there is no partial specialization. Looking up a method on an instance falls back to its generic struct.
  Each instance of the method is added to the methods of the instance of the struct it is called on.
* `register_methods_early()` adds the methods of a file to their structs before anything is checked, so
  methods can be called before they are declared.
* A method declared inside a struct is desugared by the parser (`add_implicit_self()`) into the outside form:
  it gets the struct's type parameters and an implicit first parameter `self: *Map<K, V>`. Its body is parsed
  in the scope around the struct, so members are only reached through `self`. The struct keeps these methods
  in `AstStruct.inside_methods`, not as members, so they don't change its layout.
* `self` is always a pointer. In a `method` (not `method *`) the typer rejects anything that could change
  `self`: assigning to it or to a place inside it, taking an address inside it, calling a method that can change
  its receiver on it, `append` to one of its members, `for *` over a fixed size array member, and storing,
  passing or returning the `self` pointer. See `readonly_self_place()`.
  Read-only is shallow: what members point to, like the elements of a dynamic array, can still be changed.
* At a call `x.name(...)` the receiver is adjusted to what the method wants (`rewrite_as_method_call()`): `&x`
  for a pointer receiver, `*x` for a value receiver called through a pointer. A non-addressable receiver like
  `make_map().set(k, v)` is put in a temporary whose address is passed, so changes to it are lost.

## Not supported yet

Generic methods with type parameters of their own, explicit type arguments at call sites (`max<f64>(...)`),
generic functions as values, constraints, constants and function type members in generic structs, and
generic type aliases like `Pair2<T> :: Pair<T, T>`.

## Type aliases

`IntList :: List<int>;` is a type alias (`DECLARATION_TYPE_ALIAS`, `IDENTIFIER_IS_TYPE_ALIAS`). The parser
recognises a right-hand side that can only be a type (`starts_type_alias()`). A single name like
`Alias :: Point;` could also be a constant, so `check_declaration()` turns it into an alias when the name is a
type. An alias resolves to exactly the type it names. The type parameters bound in an instance of a generic
are aliases too, which is how `T` resolves inside an instance.
