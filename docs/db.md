# db: databases

`std/db` runs SQL and reads the rows into plain structs. `db` is the core and knows no database. A database
is a driver of its own namespace; `db.postgres` is the first one, through `libpq.dll`.

```odin
import db;
import db.postgres;
use fmt;

Role :: enum { guest, member, admin }

User :: struct {
    id:        i64;
    name:      string;
    email:     ?string;    // NULL is empty
    role:      Role;       // a Postgres enum, read by name
    createdAt: string;     // reads the column created_at
}

main :: () {
    conn, err := postgres.connect("host=localhost dbname=shop user=postgres password=secret");
    if err != "" { print("Can't connect: %", err); return; }

    users, err1 := conn.query<User>("SELECT * FROM users WHERE role = $1 ORDER BY name", Role.admin);
    for u in users { print("% %", u.id, u.name); }

    changed, err2 := conn.exec("UPDATE users SET role = $1 WHERE id = $2", Role.member, 7);

    total, found, err3 := conn.queryOne<i64>("SELECT count(*) FROM users");
    ada, isThere, err4 := conn.queryOne<User>("SELECT * FROM users WHERE name = $1", "Ada");

    conn.close();
}
```

Status: steps 1 to 3 of the plan below are done: `db`, `db.postgres`, rows into structs, transactions,
allocators, `db.Table<T>` and the generator that writes the tables of a database as Saad. The typed query
builder (step 4) is designed below, but not written yet.

With the generator, the same program is:

```odin
import db;
import db.postgres;
import shop;              // written by std/db/tools/generate.sd

main :: () {
    conn, err := postgres.connect("host=localhost dbname=shop user=postgres password=secret");
    app := shop.open(&conn);

    ada, err1 := app.users.insert(shop.User{name: "Ada", role: .admin});   // gives back the id the database made
    found, err2 := app.users.get(ada.id);                                   // a ?shop.User
    users, err3 := app.users.all();
}
```

## Running SQL

| | |
| --- | --- |
| `conn.exec(sql, args...) -> i64, string` | A statement without rows (INSERT, UPDATE, DELETE, CREATE, ...). Gives the number of rows it changed |
| `conn.query<T>(sql, args...) -> [..]T, string` | A T for every row |
| `conn.queryOne<T>(sql, args...) -> T, bool, string` | The first row as a struct, or the first column of it as a single value, e.g `count(*)`. `false` when there are no rows |
| `conn.begin()`, `conn.commit()`, `conn.rollback()` | Transactions. Each gives an error or `""` |
| `conn.close()` | Closes the connection. Using it afterwards gives the error "The connection is closed" |

* Every function gives an error as a string, `""` when it worked, like `json.decode`. Errors of Postgres are its
  own message, e.g `ERROR:  relation "userz" does not exist`. Errors of reading say the row and the column, e.g
  `Row 3: age: 'x' is not a whole number`.
* The arguments are the parameters `$1`, `$2`, ... of the SQL, sent apart from the SQL text, so a value can never
  change the SQL (no SQL injection). Numbers, `bool`, `string`, chars and enums (by name) can be parameters, and
  optionals and pointers of them, where empty and `null` are `NULL`.

## Rows into structs

Each column is read into the member with the same name, where case and `_` don't matter: `createdAt` reads
`created_at`. `@db.Column` gives a member another column, or leaves it out:

```odin
User :: struct {
    id: i64;
    @db.Column(name: "mail") email: string;
    @db.Column(skip: true)   cache: *Cache;
    visits: int = -1;      // keeps -1 when the query has no column 'visits'
}
```

| Column | Member |
| --- | --- |
| `bigint`, `int`, `smallint`, `serial` | any integer type. A value that doesn't fit is an error |
| `double precision`, `real`, `numeric` | `f64`, `f32` |
| `boolean` | `bool` (`t`/`f`) |
| `text`, `varchar`, and everything else | `string`, as Postgres writes it, e.g a `timestamptz` as `2026-11-01 10:00:00+01` |
| a Postgres enum, or a number | an enum: its name (case and `_` don't matter) or its number |
| a column that can be `NULL` | an optional, e.g `?string` or `?i64`, which is empty for `NULL` (see `docs/optionals.md`) |

* `NULL` into a pointer is `null`, and into a value that is neither keeps the value, so a default stays.
* Columns that the struct doesn't have are skipped, and members without a column keep their value. So a
  smaller struct is a way to read only some columns.

## Memory

All the memory of a result comes from `conn.allocator`: the list, the strings and what pointers point to.
`null`, the default, means the heap. A list that has an allocator already keeps it.

An arena is the easy way for a web request: everything of the request is freed at once.

```odin
import mem;

arena: mem.Arena;
alloc := arena.allocator();
conn.allocator = &alloc;

orders, err := conn.query<Order>("SELECT * FROM orders WHERE customer = $1", id);
// ... use the orders ...

arena.free();   // or arena.clear() to use the memory again for the next request
```

`mem.Arena` (in `std/mem.sd`) works without setting up: the zero value takes memory when it is first used.
`arena.allocate(size)` gives zeroed memory, `arena.allocator()` an `Allocator` for lists, maps and connections,
`arena.used()` the bytes in use, `arena.clear()` makes all of it free again (keeping the newest chunk) and
`arena.free()` gives it back.

The parameters and the result of libpq itself are freed by `db` when a call returns.

## Setup

### Windows

`libpq.dll` comes with PostgreSQL, in its `bin` folder, together with the DLLs it needs (`libssl`,
`libcrypto`, `libintl`, `libiconv`, ...). A program finds them when the folder is in `PATH`, e.g:

```
setx PATH "%PATH%;C:\Program Files\PostgreSQL\16\bin"
```

or when they are next to the program. Without them, Windows says it can't find `libpq.dll` when the program
starts.

### Tests

* `tests/db/generate.sd` checks that the generator writes `tests/db/shop/shop.sd` from the schema of
  `tests/db/shop/setup.sd`, and `tests/db/shop_tables.sd` uses that code on the database. After a change of the
  generator, `SAAD_UPDATE_GOLDEN=1 tools/linux/dbtests.sh` writes the file again. `dbtests.sh` also runs
  `std/db/tools/generate.sd` itself and compares what it writes.
* `tests/std/db.sd` tests `db` with a driver written in the test itself. It needs no database and runs with
  the other tests (`tools/linux/runtests.sh`).
* `tests/db/*.sd` need Postgres. `tools/linux/dbtests.sh` makes a throwaway server in a temporary folder (with
  `initdb`), runs them and removes it. With `SAAD_TEST_POSTGRES` set to a connection string, it uses that
  server instead. On Linux it needs `postgresql` and `libpq-dev`; `tools/linux/shim_pq.c` wraps libpq for the
  Win64 calls of Saad programs, like `shim.c` does for the C runtime.

## Drivers

`db` talks to a database through a `db.Driver`, a struct of functions. The values are text, as the database
writes them, which is what libpq gives and what SQLite can give too.

```odin
Driver :: struct {
    name: string;
    execute:     (conn: *void, sql: *u8, params: []*u8, err: *string) -> *void;  // a result, or null and err
    rowCount:    (result: *void) -> i64;
    columnCount: (result: *void) -> i64;
    columnName:  (result: *void, column: i64) -> *u8;
    value:       (result: *void, row: i64, column: i64) -> *u8;  // null is NULL
    affected:    (result: *void) -> i64;
    freeResult:  (result: *void);
    close:       (conn: *void);
}
```

A driver gives back a `db.Connection{driver: ..., handle: ...}` from its own `connect`. `tests/std/db.sd` has a
complete driver of 20 lines.

## Keeping it apart

`std/db` is written so it can move to its own repository:

* `db` uses only `fmt`, `c` and the prelude. It doesn't know Postgres; `db.postgres` knows `db`.
* Nothing in the compiler knows about databases. Everything goes through reflection, member attributes and
  generic methods.
* To move it: copy `std/db/` into a project, e.g `deps/db/`. The compiler finds namespaces in the folders of
  the project. The standard library doesn't allow other files in its namespaces, so it has to leave `std/`
  rather than be in both places.
* Not solved yet: DLLs are only copied next to a program from the compiler's `packages/*/lib/`. A library
  outside the compiler can't bring its own DLLs along yet.

## The plan

1. **Done** `db.postgres`: connect, run SQL with parameters, read rows, close.
2. **Done** Rows into structs with `query` and `queryOne`, transactions, `conn.allocator`, `mem.Arena`.
3. **Done** `db.Table<T>` and a generator that writes the tables as structs.
4. A typed query builder on the generated code: `app.users.where(...).select<User>()`.
5. Later: joins, a pool of connections, a SQLite driver, migrations, and Postgres
   spoken directly over a socket instead of libpq.

### 3. Tables and the generator

#### db.Table<T>

The rows of one table, as structs of `T`:

```odin
users := db.Table<User>{conn: &conn, name: "users"};      // schema: "sales" for another schema than public
all, err := users.all();
ada, err2 := users.get(1);                                // ?User, empty when no row has the key 1
saved, err3 := users.insert(User{name: "Ada"});           // with what the database made, e.g the id
changed, err4 := users.update(saved);                     // the row with its key. 1, or 0 when there is none
removed, err5 := users.delete(saved.id);
n, err6 := users.count();
```

* The columns are the members of `T`. A member's column is the name in `@db.Column(name: ...)`, or the
  member's name in snake case: `createdAt` is `created_at`.
* `@db.Column(key: true)` marks the key, which `get`, `update` and `delete` use.
* `@db.Column(generated: true)` marks a column the database makes, e.g a `serial` id or a default of `now()`.
  `insert` and `update` leave it out, and `insert` reads it back (`RETURNING *`).
* Names are quoted in the SQL, so columns like `order` or `for` work.

#### The generator

A Saad program reads the tables from `information_schema` and the enums from `pg_enum`, and writes plain Saad
into the project:

```
saad run std/db/tools/generate.sd -- "host=localhost dbname=shop user=postgres" models/shop.sd shop
saad run std/db/tools/generate.sd -- "host=localhost dbname=shop user=postgres" models/sales.sd sales sales
```

The arguments are the connection string, the file to write, its namespace, and the schema (public when it is
left out). `saad run` runs the program in the folder it is called from, so the file is where you are. The same
is a function too: `postgres.generate(&conn, postgres.GenerateOptions{ns: "shop"})` gives the code.

For a table `users` it writes (`tests/db/shop/shop.sd` is a whole file):

```odin
// The enum role
Role :: enum { guest, member, admin }

// A row of users
User :: struct {
    @db.Column(key: true, generated: true)
    id:        i64;
    name:      string;
    email:     ?string;
    age:       ?i32;
    role:      Role = .guest;
    active:    bool = true;
    @db.Column(generated: true)
    createdAt: string;        // timestamp with time zone
}

// The tables of the schema public, on a connection
Database :: struct {
    users: UsersTable;
}

open :: (conn: *db.Connection) -> Database { ... }

// The rows of users
UsersTable :: struct {
    table: db.Table<User>;

    all    :: method () -> ([..]User, string) { ... }
    get    :: method (key: i64) -> (?User, string) { ... }    // the key has its own type
    insert :: method (row: User) -> (User, string) { ... }
    update :: method (row: User) -> (i64, string) { ... }
    delete :: method (key: i64) -> (i64, string) { ... }
    count  :: method () -> (i64, string) { ... }
}
```

| Postgres | Saad |
| --- | --- |
| `smallint`, `integer`, `bigint` (and `serial`) | `i16`, `i32`, `i64` |
| `real`, `double precision` | `f32`, `f64` |
| `boolean` | `bool` |
| an enum of the schema | an enum, with the labels as they are when they are names |
| `text`, `varchar`, `char` | `string` |
| anything else, e.g `numeric`, `timestamptz`, `uuid`, `json` | `string`, with the type of the column as a comment |
| a column that can be `NULL` | `?T` |

* Names: the table `order_items` is the row `OrderItem` (most English plurals become singular), the table type
  `OrderItemsTable` and the member `orderItems` of `Database`. The column `created_at` is the member `createdAt`.
  A column that is a keyword gets a `_`, e.g `"for"` is `for_` with `@db.Column(name: "for")`.
* A default that is a constant becomes the default of the member, e.g `role: Role = .guest`, so a struct literal
  has the same values the database would give. A default that is computed (`nextval(...)`, `now()`, ...) and
  identity columns make the column `generated`.
* A table with a key of one column gets `get`, `update` and `delete`. Others get `all`, `insert` and `count`.
* The file is plain Saad: a change of the database is a diff in the project, and building a program needs no
  database. Make it again instead of changing it.
* Typed query columns (`db.Field<T>`) come with step 4, which adds them to the table types.

### 4. The typed query builder

C#'s LINQ turns `u => u.Age > 30` into a tree that a library can read. Saad has no closures, and no code that
reads code, so the generated `Field`s stand in for the members, and their methods build the conditions:

```odin
app := shop.open(&conn);

adults, err := app.users
    .where(app.users.age.atLeast(18).and(app.users.name.like("A%")))
    .orderBy(app.users.name)
    .limit(20)
    .select<shop.User>();

// Only the columns of a smaller struct: SELECT id, name
Name :: struct { id: i64; name: string; }
names, err2 := app.users.where(app.users.email.isNull()).select<Name>();

app.users.age.atLeast("18");   // A compile error: wants an i32
```

* `atLeast :: method <T>(f: db.Field<T>, value: T) -> db.Condition` is what makes `"18"` a type error.
  Likewise `equals`, `notEquals`, `lessThan`, `atMost`, `greaterThan`, `between`, `like`, `isNull`, `oneOf`,
  and `and`, `or`, `not` on conditions.
* The builder writes the SQL with parameters, `WHERE age >= $1 AND name LIKE $2`, and runs it with `query`.
  `select` reads the rows like `query` does, so the struct decides which columns are read.
* `first<T>()`, `count()`, `exists()`, and later `insert(row)`, `update().set(field, value)` and `delete()`.
* Plain SQL stays for everything the builder can't say.

## Gaps in the language

Found while writing this, in the order they would help:

* Assigning several results to existing variables, `count, err = conn.exec(...)`. Today each call needs new
  names (`err1`, `err2`, ...) or a function around it.
* `defer`, to free a result on every return.
* Operators on structs, so `app.users.age >= 18` could be a condition instead of `atLeast(18)`.
* Attributes on structs, e.g `@db.Table("users")`, for tables written by hand.
* Struct embedding (or `using`), so `UsersTable` could be a `db.Table<User>` with more methods, instead of
  forwarding each method to its member `table`.
* Types for dates, decimals and UUIDs. They are strings for now.
