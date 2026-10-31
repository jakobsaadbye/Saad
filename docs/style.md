# Style guide

The standard library, the examples and the tests follow these conventions. The compiler doesn't enforce them.

| What | Case | Example |
| --- | --- | --- |
| Functions and methods | camelCase | `newRandom`, `xs.removeAt(0)` |
| Variables, parameters and struct members | camelCase | `playerScore`, `firstName` |
| Types: structs, enums and type aliases | PascalCase | `StringBuilder`, `HttpStatus` |
| Enum values and union variants | camelCase, starting lowercase | `Status :: enum { open, closed }` |
| Constants | PascalCase | `MaxIntValue :: 123456;` |
| Namespaces | lowercase, dotted | `game.physics` |
| Attribute structs | PascalCase | `@json.Field(name: "id")` |

* Functions that make a value are called `newX`, e.g `newStringBuilder(64)`.
* An enum value can be a keyword, e.g `TypeKind.int` or `.struct`. After a `.` every word is a name.
* Bindings to C keep the C names, e.g `malloc` or raylib's `DrawText`.
