# Errors

An error is a value that says what went wrong, and where. A function that can fail gives it as its last result:

```odin
import errors;

readConfig :: (path: string) -> (Config, error) {
    if !os.fileExists(path) { return Config{}, errors.make("% isn't there", path); }
    ...
    return config, null;                          // null: no error
}
```

The caller decides what a failure is:

```odin
config, err := readConfig(path);                  // taken: a value. Look at it, e.g for a game that must go on
if err != null { ... }

config := readConfig(path);                       // not taken: if there is an error, it is thrown
```

## The rules

1. **An error that is taken is a value, and one that isn't is thrown.** `v, err := f()` takes it, `v := f()` doesn't.
   A function that gives only an error, e.g `save :: () -> error`, throws it when it is called as a statement,
   `save();`, and gives it with `err := save();`.
2. **Any function can throw**, whatever its results are. There is no `throws` to write: an error thrown in a function
   leaves every function it is thrown through, up to a `try`, or ends the program. So a function without an `error`
   in its results still throws what its calls throw, e.g `total :: () -> int { return readCount(); }`.
3. **`return f()` gives the error along**, as it gives all of the results of `f`, when the function has the same
   results, e.g `(Config, error)`.
4. **All results but the error can be taken, and the error is thrown**: `post, found := conn.queryOne<Post>(sql)` of
   `(Post, bool, error)` throws, like `v := f()` does for `(V, error)`.
5. **Only a call whose last result is an `error` can be taken.** `v, err := f()` of a `f :: () -> int` is an error of
   the compiler, as there is one value. To get a value from a function that throws, put it in a `try`.
6. **An error isn't a condition**: `if err { }`, `!err` and `assert(err)` are compile errors, as they read the other way
   round. Compare it with `null`: `if err != null { }`. The same goes for any pointer, as a condition is a `bool`.

### When a function gives an error, and when it throws

* **Give an `error` as the last result** when failing is one of the things that happen when the function does its job:
  a file that isn't there, input that isn't valid, a network that is down, a row that isn't found. The caller knows
  best what that means, so it gets to choose: take it and look at it, or let it be thrown. It costs the caller nothing
  when it doesn't care, as `v := f()` throws for it. The standard library does it like that, e.g `os.readFile`.
* **Throw**, without an error in the results, when no caller can do anything better than let it go up: a mistake of
  the program (`panic("unreachable")`, a failed `assert`), or an error deep in a function that has no error of its own
  to give, e.g in the handler of a request, where the server turns it into a 500.
* What only asks a question and can't fail stays a `bool`, e.g `os.fileExists` and the `found` of `os.getEnv`.

The standard library gives errors: `dbms` (`dbms.DatabaseError`, `dbms.QueryError`, `dbms.NoLink`), `json.decode`,
`json.decodeInto` and `json.parse` (`json.DecodeError`), and the file functions of `os`, `readFile`, `writeFile`,
`appendFile`, `makeDir`, `removeFile` and `renameFile` (`os.NotFound`, `os.IOError`).

## Making errors

The package `errors` makes them:

```odin
import errors;

errors.make("no port")                                     // a message
errors.make("no user with the id %", id)                   // in a format, like fmt.sprint
errors.make("no user with the id %", id, kind: NotFound)   // of a kind, for err.is(NotFound)
errors.make("loading % failed", path, cause: err)          // because of another error
errors.wrap(err, "loading %", path)                        // the same, and of the kind of err
```

* Without values, the message is as it is written, so `errors.make(text)` takes a text from elsewhere as it is, also
  with a `%` in it.
* The message is on the heap, like the error and its trace, so it outlives the arena it was made in, e.g of a
  request.
* A kind is declared as a constant: `NotFound :: ErrorKind{"NotFound"};`. Two kinds are the same when their names are.

## Error types of their own

An error can carry more than a message: what a catch needs to decide or to report, e.g the SQL of a query that failed.
Its type is a struct that embeds `Error` (see `docs/embedding.md`). Functions still give and take `error`: the type of
its own is only seen where it is made, and where a catch asks for it.

```odin
QueryError :: struct {
    @embed base: Error;          // message, kind, cause and trace
    sql:  string;
    code: int;
}

run :: (sql: string) -> (Rows, error) {          // an error, not a *QueryError
    ...
    return Rows{}, errors.typed(QueryError{sql: sql, code: code}, "the query failed: %", reason);
}

rows, err := run(sql);
if err.is(QueryError) { ... }                     // the type is a kind

q, ok := err as *QueryError;                      // the QueryError, in the error or in its causes
if ok { log("% failed with %", q.sql, q.code); }

q := err as *QueryError;                          // a Panic when there is none
```

* `errors.typed(value, format, ...values, cause: err)` makes one on the heap and gives a `*QueryError`, so its members
  can be set. Where an `error` is wanted it is one, as a `*QueryError` converts to the `*Error` it embeds: it is
  returned as one, and thrown, `throw errors.typed(...)`.
* **The type is its kind.** Its kind is the name of its type, e.g `QueryError`, or `dbms.QueryError` in a namespace, so
  `err.is(QueryError)` tells, like for any kind, also from the causes. An error that nothing catches is reported as
  `QueryError: the query failed: ...`.
* **`as` gives it back**, like the cast of an interface (see `docs/interfaces.md`), and looks at the causes like `is`:
  * `q, ok := err as *QueryError`: the `*QueryError` and true, or null and false, also for a null error
  * `q := err as *QueryError`: throws a `Panic` when there is none, e.g `The error is a NotFound, not a QueryError`
  * So after `err.is(QueryError)` is true, `err as *QueryError` always gives one.
* `as` finds what `errors.typed` made, of exactly that type.

### A switch on an error

For more than one error type, a `switch` has a case for each, like the switch on an interface (`docs/interfaces.md`):

```odin
describe :: (err: error) -> string {
    switch e in err {
        case *QueryError { return fmt.sprint("the query % failed", e.sql); }    // e: *QueryError
        case *Timeout    { return fmt.sprint("no answer in % s", e.seconds); } // e: *Timeout
        case *NotAllowed
        case *LoggedOut  { return e.message; }                                 // more types: e is the error
        else {
            if err == null         { return "it worked"; }
            if err.is(NotFound)    { return "not there"; }                     // a kind is asked in the else
            return err.message;
        }
    }
    return "";
}
```

* A case asks what `err as *QueryError` asks: if the error, or one of its causes, is one. The first case that matches
  is taken, so a wrapped `QueryError` still goes to its case.
* A null error, and one of no case, go to the `else`. A switch expression on an error needs one, as an error can be of
  any type, or null.
* A case is an error type. A kind, e.g `case NotFound`, is an error of the compiler, with the hint to ask
  `err.is(NotFound)` in the `else`, and so is a type that doesn't embed `Error`.
* Without a name, `switch err { case *Timeout { ... } }`, the cases only choose.
* What isn't an error type is an error of the compiler, with a hint:
  * `err as *Plain`: `An error is never a *Plain`, and `hint: Plain isn't an error. An error type of its own embeds
    Error: give it a member '@embed base: Error;'`
  * `errors.make("...", kind: QueryError)`: the kind of an error type comes with its type, so the hint says to make
    one with `errors.typed`. `err.is(QueryError)` takes the type, as it only asks.

A kind is enough to tell errors apart. An error type of its own is for when a catch needs more than the message.

## try, catch and throw

A thrown error leaves the functions it is thrown through, running their `defer`s on the way, up to the `catch` of a
`try` around it:

```odin
handlePost :: (req: Request) -> Response {
    try {
        post := db.posts.where(p => p.slug == req.slug).first();
        return render(post);
    } catch (err) {
        if err.is(dbms.NotFound) { return Response{status: 404}; }
        log(err);
        return Response{status: 500};
    }
    return Response{status: 500};
}
```

* `throw err;` throws an error the program made itself, e.g `throw errors.make("no port");`. A `catch` can throw
  again, e.g with the error as the cause: `throw errors.wrap(err, "loading failed");`.
* An error that nothing catches ends the program, with its kind, message and trace:

  ```
  NotFound: no one is called cy
      at age (main.sd:11)
      at main (main.sd:37)
  ```

  and its causes after it, each with `caused by`, e.g `caused by QueryError: the query failed: ...`.

* These throw a `Panic`, so a web server can turn them into a 500 instead of crashing:
  * a division by zero
  * an index outside of the elements of a fixed array, a slice, a list or a string, e.g `Index 5 is out of bounds, as
    there are 3 elements`. An index into a pointer, e.g `p[i]` of a `*u8`, isn't checked for its bounds
  * a null pointer that is dereferenced: a member of it (`user.manager.name`, also a write), `*p`, and an index into it
    (`p[i]`). The message names the pointer, e.g `Null pointer dereference: user.manager is null`. A method called on a
    null pointer throws where it uses `self`, e.g `self is null`
    Taking an address, e.g `&p[i]` or `&p.x`, only computes it, so it doesn't throw for a null `p`; a read on the way
    does, e.g `p.next` of `&p.next.value`
  * `saad build --unchecked` leaves the bounds checks and the null checks out
  * an `assert` whose condition is false, with the place of the assert first in the trace
  * `mem.allocate` and `mem.resize` when there is no memory left
  * `panic("message")`

## The error type

`error` is a `*Error` of the prelude (`std/prelude/errors.sd`), null for no error:

| | |
| --- | --- |
| `err.message` | What went wrong |
| `err.kind` | An `ErrorKind`, for a catch to tell errors apart: `err.is(NotFound)`. Two kinds are the same when their names are |
| `err.cause` | The error it was made because of, or null. `is` and `as` look at the causes too |
| `err.trace` | Where it was made or thrown, and every function it was thrown through: `function`, `file` and `line` |
| `err.type` | The error type of its own it was made as, e.g `QueryError`, or null. `err as *QueryError` gives it back |

* The prelude has the type, `err.is` and `newError(message, kind, cause)`, as the compiler throws and
  catches errors. The package `errors` makes them with a format, as it uses `fmt`, which is above the prelude.
  `newError` is what `errors.make` and `panic` are built on, and takes the message as it is.
* `Panic` is the kind of the panics.

## How it works

* The error that is being thrown is in `thrownError` of the prelude, a `@threadLocal` global, so each thread has its
  own. A Saad function returns its values through out-pointers, so it says in `eax` if an error left it: 0 when it
  returns, 1 when it was thrown out of it. After every call of a Saad function, the compiler checks `eax`. When it is
  set, the call adds a frame to the trace (`traceThrow`) and goes to the `catch` of the innermost `try` of the
  function, or runs the `defer`s and returns with 1. A call through a function value, which can be a function of C,
  checks `thrownError` too when `eax` isn't 0. There is no stack unwinding of the system: the code of the function
  does it, so it works the same on Windows and Linux.
* A call whose error isn't taken (`AST_FLAG_THROWS_ON_ERROR`) throws the error when it isn't null.
* `throw` takes an `error`, or a pointer to a struct that embeds `Error` (`is_embedding_error_type()`), which is
  converted to the `*Error` in it (`AstThrow.error_type`).
* `errors.typed<T>` sets `type` to the descriptor of `T`, `value` to the whole error and the kind to the name of `T`.
* An error type where an `ErrorKind` is wanted, e.g `err.is(QueryError)`, is `ErrorKind{"QueryError"}`
  (`error_kind_of_type_name()`).
* `err as *QueryError` (`AstCast.is_error_cast`) walks the error and its causes and compares their `type` with the
  descriptor of `QueryError` (`error_as()` in `ir_lower.c`), so it doesn't need to know where in the struct the `Error`
  is: `value` points to the whole of it. Without `ok`, none is `panicErrorCast` of the prelude.
* `switch e in err` is `check_error_switch()`: a chain of ifs, each on a type test of the cases (`is_type_test` and
  `is_error_cast`), with `e` declared as the cast of its case, like `check_interface_switch()`.
