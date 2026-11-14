# Operators of your own

```odin
Vec2 :: struct {
    x: f32;
    y: f32;
}

operator + :: (a: Vec2, b: Vec2) -> Vec2 { return Vec2{a.x + b.x, a.y + b.y}; }
operator - :: (v: Vec2) -> Vec2          { return Vec2{-v.x, -v.y}; }
operator * :: (v: Vec2, s: f32) -> Vec2  { return Vec2{v.x * s, v.y * s}; }
operator * :: (s: f32, v: Vec2) -> Vec2  { return v * s; }
operator < :: (a: Vec2, b: Vec2) -> bool { return a.x * a.x + a.y * a.y < b.x * b.x + b.y * b.y; }

position := Vec2{1, 2};
velocity := Vec2{3, 4};
position += velocity * 0.5;     // operator * of (Vec2, f32), and operator + for +=
back := -velocity;
if position < velocity { ... }
```

`operator <op> :: (parameters) -> Result { ... }` declares what an operator does for the types of its parameters. It is a
function: `a + b` of two `Vec2` is a call of `operator + (Vec2, Vec2)`.

* **Operators:** `+ - * / %`, `-v` (one parameter), `< <= > >=` and `==`, which give a `bool`. `!=` is the opposite of
  `==`, so `a != b` is `!(a == b)`, and it can't be declared on its own. A struct without an `operator ==` is compared member by
  member. `&& || !`, `=`, `.`, `as`, indexing and the bitwise operators can't be declared.
* **Compound assignments:** `a += b` uses `operator +`, and so on for `-= *= /= %=`. The place is evaluated once, e.g
  `cells[next()] += v` calls `next` once. The operator must give the type of `a`.
* **Which one is called:** the one whose parameters are the types of the operands, exactly: an `f32` isn't an `f64`, and
  a `Meters` isn't an `f64`. A number written in the program fits a parameter of a number type, e.g the `0.5` of
  `velocity * 0.5`. When two operators fit as well, e.g `(Vec2, f32)` and `(Vec2, f64)` for `v * 2`, it is an error, and a cast
  says which, e.g `v * (2 as f32)`.
* **Order:** an operator has the precedence of the operator of numbers, e.g `a + b * 2` is `a + (b * 2)`.
* **Types:** at least one parameter is a struct, an enum or a newtype. The operators of numbers, strings and pointers are those
  of the language. An operator of a newtype wins over the operators of the type it is made of.
* **One per operands:** two operators with the same parameter types are an error, also in different files.
* **Where:** at the top of a file, not inside a struct or a function. An operator isn't generic, but a generic function uses
  the operators of its type arguments, e.g `total += x` in a `sum<T>` called with `Vec2`.
* **Visible:** like a method: an operator of a namespace that the program uses is there without importing more. `@private`
  keeps it to its namespace, or its file if it has none. It can't be called by a name.

## In an editor

The language server colors an operator of the program differently from the operators of the language (like a function, in
VS Code), and go to definition on it opens its declaration, e.g from the `+` of `position + velocity` to
`operator + :: (a: Vector2, b: Vector2) -> Vector2`. Find references lists where it is used. See `docs/lsp.md`.

## raylib

`std/vendor/raylib.sd` has the operators of the math types, which call the functions of raymath (`std/vendor/raymath.sd`, in
the namespace `raylib`, from `raylib.dll`), like the C++ operators of `raymath.h`:

| | |
| --- | --- |
| `Vector2`, `Vector3`, `Vector4` | `a + b`, `a - b`, `-v`, `v * s`, `s * v`, `v / s`, `a * b` and `a / b` component by component, `a == b`, and `v * mat` for `Vector2` and `Vector3` |
| `Quaternion` | `a + b`, `a - b`, `q + s`, `q - s`, `a * b` (the rotation `b`, then `a`), `q * s`, `q * mat`, `q * v` (rotates a `Vector3`), `a / b`, `a == b` |
| `Matrix` | `a + b`, `a - b`, `a * b` (`MatrixMultiply`) |

`==` is `Vector2Equals` and the others, which are true for values that are almost equal. The vectors also have the methods
`length()`, `lengthSqr()`, `normalize()`, `dot(other)`, `distance(other)` and `lerp(to, amount)`, `Vector2` has `rotate(angle)`
and `Vector3` `cross(other)`. `Quaternion` is a newtype of `Vector4`, so its operators aren't those of `Vector4`.

```odin
import raylib;
use raylib;

ball.position += ball.speed * GetFrameTime();
if ball.position.distance(paddle.position) < 20 { ball.speed = -ball.speed; }
```

## How it works

* The lexer has the keyword `operator`. `parse_operator_declaration()` in `parser.c` reads the declaration as a constant
  function named `operator +`, with `AstFunctionDefn.operator_token`. It isn't put in a scope, as there are many operators `+`,
  but in `Parser.operators`.
* `check_binary()` and `check_unary()` in `typer.c` call `check_operator_call()` when an operand is a struct, an enum or a
  newtype. `find_operator()` checks the declarations of the operators with the token (the signature, then the body, like the
  declaration of a function that is called), and picks the one whose parameters match. The call of it becomes the
  `replacement` of the expression, so the backends lower a call. The operands were checked already, so they get
  `AST_FLAG_OPERAND_CHECKED`, and `check_expression()` doesn't check them again.
* `a += b` with an operator: `check_operator_assignment()` makes the call `operator +(current, b)`, where `current` is an
  `AstOptionalValue` that `lower_assignment()` in `ir_lower.c` sets to the place of `a`, so the place is evaluated once.
* `check_operator_signature()` checks what the declaration takes and gives, and that no other operator has its parameters. The
  symbol is named after the operator and its parameter types, e.g `operator_add$Vec2$Vec2`.
* `tests/operators.sd`, `tests/should_fail/operator_*.sd`, and `tests/raylib/raymath.sd`, which
  `tools/linux/check_raymath.sh` runs with `raymath.h` compiled for Linux with the calling convention of Windows.
