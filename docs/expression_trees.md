# Expression trees

A parameter of type `Expr<F>` takes a short lambda as the tree of what it does, instead of as a function. A library
can then read the tree, e.g to make SQL of it (see `docs/dbms.md`). `F` is the type of the lambda, e.g
`Expr<(User) -> bool>`.

```odin
where :: method <T>(q: *Query<T>, f: Expr<(T) -> bool>) -> *Query<T> { ... }

minAge := 18;
adults := db.users.where(u => u.age >= minAge && u.name.startsWith("A"));
```

The lambda above is the tree

```
binary &&
├── binary >=
│   ├── member age
│   │   └── parameter 0 (u)
│   └── value 18          <- minAge, when the tree was made
└── call startsWith
    ├── member name
    │   └── parameter 0 (u)
    └── value "A"
```

## The rules

* Only a short lambda (`x => expr`) can be an `Expr`. It is checked like a function of type `F`, so a wrong member or
  a result of the wrong type is an error where it is written.
* A part of the lambda that doesn't use its parameters becomes a value, computed when the tree is made. So unlike other
  lambdas, it can use the variables around it, and calls like `now()`. The value is copied, so the tree can outlive
  them, e.g be returned from the function.
* A part that uses the parameters can be:

  | | Kind of node |
  | --- | --- |
  | a parameter `x` | `parameter`, with `index` and `name` |
  | a member `x.age`, `x.address.city` | `member`, with `left` and `name` |
  | `!a`, `-a` | `unary`, with `op` and `left` |
  | `a == b`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `\|\|`, `+`, `-`, `*`, `/`, `%`, `??` | `binary`, with `op`, `left` and `right` |
  | a call `x.name.startsWith(p)` or `f(x.age)` | `call`, with `left` (null for a function), `name` and `args` |
  | a struct literal with names `{name: x.name}` | `object`, with `names` and `args` |

  `a?.b` and `a?.m()` are the same nodes as `a.b` and `a.m()`: a reader like SQL gives NULL when `a` is. Anything
  else, e.g a cast, indexing or another lambda, is an error.
* `null` is a `value` node whose `value.type` is null. An enum value like `.admin` is a value of the enum it is
  compared with.
* The tree types are in `std/prelude/collections.sd`: `Expr<F>{root, parameters}` and `ExprNode`.

## How it works

* `check_lambda_expression()` sees that an `Expr<F>` of the prelude is wanted (`expression_tree_type()`). A short
  lambda given to a generic function for an `Expr<...>` is checked like one given for the function type inside it, so
  `T` and `U` are inferred, e.g the `U` of `select(f: Expr<(T) -> U>)`.
* `check_expression_tree()` checks the lambda as a function of type `F`, marked `is_expression_tree`. Such a lambda
  can use the variables of the functions around it (`is_non_constant_identifier_found_inside_current_function()`),
  as it is never compiled.
* The tree is made from a second parse of the lambda (`reparse_short_lambda()`), as checking changes the AST.
  `tree_of()` turns it into calls of the builder functions in the prelude, e.g
  `exprBinary(">=", exprMember(exprParameter(0, "u"), "age"), exprValue(minAge))`. A part without the parameters is
  the argument of `exprValue(value: any)`, checked in the function around the lambda. The first parse gives the types
  it needs, e.g the enum of `.admin`.
* The calls go in the struct literal `Expr<F>{root: ..., parameters: ["u"]}`, which is the `replacement` of the
  lambda: the backend lowers it instead of the function.
