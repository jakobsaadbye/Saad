# Errors

An error is a value that says what went wrong, and where. A function that can fail gives it as its last result:

```odin
readConfig :: (path: string) -> (Config, error) {
    if !os.fileExists(path) { return Config{}, fmt.errorf("% isn't there", path); }
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

The rule: **an error that is taken is a value, and one that isn't is thrown.** A function that gives only an error,
e.g `save :: () -> error`, throws it when it is called as a statement, `save();`, and gives it with `err := save();`.

An error isn't a condition: `if err { }`, `!err` and `assert(err)` are compile errors, as they read the other way
round. Compare it with `null`: `if err != null { }`. The same goes for any pointer, as a condition is a `bool`.

The standard library gives errors: `dbms` (`dbms.DatabaseError`, `dbms.QueryError`, `dbms.NoLink`), `json.decode`,
`json.decodeInto` and `json.parse` (`json.DecodeError`), and the file functions of `os`, `readFile`, `writeFile`,
`appendFile`, `makeDir`, `removeFile` and `renameFile` (`os.NotFound`, `os.IOError`). What asks a question and can't fail
stays a `bool`, e.g `os.fileExists` and the `found` of `os.getEnv`.

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

* `throw err;` throws an error the program made itself, e.g `throw newError("no port");`. A `catch` can throw again,
  e.g with the error as the cause: `throw newError("loading failed", cause: err);`.
* An error that nothing catches ends the program, with its kind, message and trace:

  ```
  NotFound: no one is called cy
      at age (main.sd:11)
      at main (main.sd:37)
  ```

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
| `err.cause` | The error it was made because of, or null. `is` looks at the causes too |
| `err.trace` | Where it was made or thrown, and every function it was thrown through: `function`, `file` and `line` |

* `newError(message, kind, cause)` makes one, and `fmt.errorf("no user %", id)` with a format.
* A kind is declared as a constant: `NotFound :: ErrorKind{"NotFound"};`. `Panic` is the kind of the panics.

## How it works

* The error that is being thrown is in `thrownError` of the prelude, a `@threadLocal` global, so each thread has its
  own. A Saad function returns its values through out-pointers, so it says in `eax` if an error left it: 0 when it
  returns, 1 when it was thrown out of it. After every call of a Saad function, the compiler checks `eax`. When it is
  set, the call adds a frame to the trace (`traceThrow`) and goes to the `catch` of the innermost `try` of the
  function, or runs the `defer`s and returns with 1. A call through a function value, which can be a function of C,
  checks `thrownError` too when `eax` isn't 0. There is no stack unwinding of the system: the code of the function
  does it, so it works the same on Windows and Linux.
* A call whose error isn't taken (`AST_FLAG_THROWS_ON_ERROR`) throws the error when it isn't null.
* While the `defer`s run during a throw, the error is put aside, so their calls run as usual.
* `main` reports an error that nothing caught (`reportUncaught`), and the program ends with the exit code 1.
* A call of C, e.g `printf`, has no check: C doesn't throw.
