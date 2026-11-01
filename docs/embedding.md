# Embedding

A struct member with `@embed` lends its members and methods to the struct it is in:

```odin
Ast :: struct {
    kind: int;
    line: int;
    describe :: method () -> string { ... }
}

Binary :: struct {
    @embed head: Ast;
    left:  *Ast;
    right: *Ast;
}

b: Binary;
b.line = 3;            // b.head.line
b.describe();          // b.head.describe()
check(&b);             // a *Binary is an *Ast where one is wanted

check :: (node: *Ast) { ... }
```

It is how a C compiler uses an `Ast *` for every kind of node, and how Go embeds structs. It is not inheritance:
nothing is overridden at runtime, there is no table of methods, and a `Binary` is not an `Ast` by value. It is
names and pointers only.

## The rules

* `x.name` that isn't a member or method of `x` is looked for in its embedded members, then in theirs, up to 8
  levels. The fewest levels win, so the struct's own members and methods always win over embedded ones.
* A name in two embedded members at the same level is an error when it is used, with a hint to say which one,
  e.g `n.a.line`.
* A method of an embedded member is called on the embedded member, e.g `b.moveTo(9)` is `b.head.moveTo(9)`, so a
  `method *` changes `b.head`.
* The embedded member is still a member by its name: `b.head`.
* A `*Outer` converts to a `*Inner` of a struct it embeds, also through more levels, e.g for an argument or a
  variable. The pointer is the address of the embedded member, so it doesn't have to be the first member. The
  other way, `*Inner` to `*Outer`, is an error, as not every `Inner` is in an `Outer`.
* A pointer to a struct can be embedded too, e.g `@embed counter: *Counter;`. Its members are reached through the
  pointer, but a `*Outer` doesn't convert to it.
* Only structs and pointers to structs can be embedded, not unions, and the variants of a union can't be.
* Struct literals don't see embedded members: write `Binary{head: Ast{line: 3}}`, not `Binary{line: 3}`.
* The editor completes the embedded members and methods after a `.`.

## How it works

* The parser sets `IDENTIFIER_IS_EMBEDDED` on the member.
* When `check_member_access()` finds no member or method of the name, `find_embedded_path()` searches the embedded
  members level by level, and the access is rewritten to go through them, e.g `b.line` becomes `b.head.line`. Then
  it is checked like any member access, so methods, places and read-only `self` work as they do without embedding.
* `types_are_equal(*Inner, *Outer)` is true when `embedded_offset(Outer, Inner)` finds the embedded member, and the
  backend adds that offset when it converts the pointer (`convert_scalar()` in `ir_lower.c`).
