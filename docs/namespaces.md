# Namespaces, imports and attributes

## Namespaces

A file says which namespace it is in with `namespace` as its first statement:

```odin
// mygame/physics/body.sd
namespace game.physics;

Body :: struct { mass: f64; }
```

* Any number of files, in any folders, can be in the same namespace. They see each other's declarations,
  also the private ones, without importing anything.
* Namespaces can use each other, also in a cycle.
* Files without a `namespace` are standalone, like the main file of a program.
* The standard library namespaces are top-level: `fmt`, `json`, `math`, `mem`, `os`, `time`, `random`, `c`,
  `db` and `raylib`, plus `os.windows` and `db.postgres`. A project cannot add files to them. See `docs/standard_library.md`.
* The prelude (`List`, `Map`, the methods of strings and slices, and the runtime) is always there, without an import.

Before parsing, the compiler scans the folder of the main file (and every folder below it, except hidden
folders and `build`) and `std/` for files that start with `namespace`. Only the namespaces a program
imports or names are parsed.

## Imports

```odin
import fmt;                 // fmt.print("%", x)
import math as m;           // m.sin(x), m.Vec2
import math.sin;            // a single declaration: sin(x)
import math.cos as cosine;  // a single declaration with another name
use os;                     // every public declaration of os
std_name := math.sqrt(2.0); // a fully qualified name works without an import
```

* `import a.b;` makes the last part of the name, `b`, usable as a qualified name. `as` renames it.
* `import a.b.c;` is a single declaration `c` of the namespace `a.b`, when there is no namespace `a.b.c`.
* `use a.b;` brings every public declaration of the namespace into the file.
* A file's own declarations win over imported names, explicit imports win over `use`, and `use` wins over
  the prelude.
* When two `use`s bring in the same name, using it is an error that lists where it is declared and how to
  write it with its namespace. It is not an error as long as the name isn't used.
* Methods belong to their type, so `xs.add(1)` needs no import.
* `import "fmt";` (a string) is the old way to import a file. It still works, with a warning for files in
  a namespace. For a namespace of the standard library it means `use fmt;`.

Structs and enums are told apart by their namespace: `game.render.Vec2` and `geometry.Vec2` are different
types, also inside generic instances and anonymous structs. Their names in error messages and `typeof`
include the namespace. Functions of a namespace get it as a prefix of their symbol, e.g.
`game$physics$collide`.

## Attributes

An attribute describes the declaration that follows it. It is written with `@`, as `#` is reserved for
compile-time features.

```odin
@private
grow :: method * () { ... }

@deprecated("use add")
push :: method * (value: T) { ... }

@private @deprecated
Slot :: struct { index: int; }
```

* Attributes go before a function, method (also inside a struct), struct, enum, constant, type alias or global
  variable, at the top of a file. Not inside functions.
* Unknown, repeated and misplaced attributes are errors.
* Structs marked with `@attribute` are attributes of your own, which go on struct members and are read through
  reflection, e.g `@json.Field(name: "id")`. See `docs/reflection.md`.

| Attribute | |
| --- | --- |
| `@private` | Only visible inside its namespace, in any of its files. In a file without a namespace, only inside the file. `use` and imports leave it out. A public function cannot take or return a private type |
| `@deprecated("message")` | Using it gives a warning with the message. The message is optional |
| `@attribute` | On a struct: its values can be given to struct members as attributes |

## Not done yet

* A project file that lists or excludes folders, for big projects.
* `use` with a list of names, e.g. `use math.{sin, cos}`. Import each name on its own line for now.
* The attributes of the language on struct members, and attribute values on other declarations than members.
