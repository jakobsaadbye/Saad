# dbms: databases

`std/dbms` runs SQL and reads the rows into plain structs. `dbms` is the core and knows no database. A database
is a driver of its own namespace; `dbms.postgres` is the first one, through `libpq.dll`.

```odin
import dbms;
import dbms.postgres;
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

Status: steps 1 to 3 of the plan below are done: `dbms`, `dbms.postgres`, rows into structs, transactions,
allocators, `dbms.Table<T>` and the generator that writes the tables of a database as Saad. The typed query
builder (step 4) is designed below, but not written yet.

With the generator, the same program is:

```odin
import dbms;
import dbms.postgres;
import database;          // written by std/dbms/tools/generate.sd
use entities;             // the rows, e.g User

main :: () {
    conn, err := postgres.connect("host=localhost dbname=shop user=postgres password=secret");
    db := database.open(&conn);

    ada := User{name: "Ada", role: .admin};
    db.users.create(&ada);                       // kept until saveChanges
    err1 := db.saveChanges();                    // writes the changes in one transaction, and the id into ada

    found, err2 := db.users.get(ada.id);         // a *User, null when there is none
    users, err3 := db.users.all();
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
`created_at`. `@dbms.Column` gives a member another column, or leaves it out:

```odin
User :: struct {
    id: i64;
    @dbms.Column(name: "mail") email: string;
    @dbms.Column(skip: true)   cache: *Cache;
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

The parameters and the result of libpq itself are freed by `dbms` when a call returns.

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

* `tests/db/generate.sd` checks that the generator writes the files of `tests/db/model` from the schema of
  `tests/db/shop/setup.sd`, and that a later run removes the files of tables that are gone. `tests/db/shop_tables.sd`
  uses those files on the database. After a change of the generator, `SAAD_UPDATE_GOLDEN=1 tools/linux/dbtests.sh`
  writes them again. `dbtests.sh` also runs `std/dbms/tools/generate.sd` itself and compares what it writes.
* `tests/std/dbms.sd` tests `dbms` with a driver written in the test itself. It needs no database and runs with
  the other tests (`tools/linux/runtests.sh`).
* `tests/db/*.sd` need Postgres. `tools/linux/dbtests.sh` makes a throwaway server in a temporary folder (with
  `initdb`), runs them and removes it. With `SAAD_TEST_POSTGRES` set to a connection string, it uses that
  server instead. On Linux it needs `postgresql` and `libpq-dev`; `tools/linux/shim_pq.c` wraps libpq for the
  Win64 calls of Saad programs, like `shim.c` does for the C runtime.

## Drivers

`dbms` talks to a database through a `dbms.Driver`, a struct of functions. The values are text, as the database
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

A driver gives back a `dbms.Connection{driver: ..., handle: ...}` from its own `connect`. `tests/std/dbms.sd` has a
complete driver of 20 lines.

## Keeping it apart

`std/dbms` is written so it can move to its own repository:

* `dbms` uses only `fmt`, `c` and the prelude. It doesn't know Postgres; `dbms.postgres` knows `dbms`.
* Nothing in the compiler knows about databases. Everything goes through reflection, member attributes and
  generic methods.
* To move it: copy `std/dbms/` into a project, e.g `deps/db/`. The compiler finds namespaces in the folders of
  the project. The standard library doesn't allow other files in its namespaces, so it has to leave `std/`
  rather than be in both places.
* Not solved yet: DLLs are only copied next to a program from the compiler's `packages/*/lib/`. A library
  outside the compiler can't bring its own DLLs along yet.

## The plan

1. **Done** `dbms.postgres`: connect, run SQL with parameters, read rows, close.
2. **Done** Rows into structs with `query` and `queryOne`, transactions, `conn.allocator`, `mem.Arena`.
3. **Done** `dbms.Table<T>`, `dbms.KeyedTable<T, K>` and a generator that writes the tables as structs.
4. A typed query builder on the generated code: `db.users.where(...).select<User>()`.
5. **Done** A unit of work: `create`, `update` and `delete` keep the changes, and `db.saveChanges()` writes them all
   in one transaction.
6. Later: joins, a pool of connections, a SQLite driver, migrations, and Postgres
   spoken directly over a socket instead of libpq.

### 3. Tables and the generator

#### dbms.Table<T> and dbms.KeyedTable<T, K>

The rows of one table, as structs of `T`:

```odin
context := dbms.newContext(&conn);                              // keeps the changes until saveChanges
users := dbms.keyedTable<User, i64>(context, "users", "id");    // the column of the key, and its type
users.generated = ["id", "created_at"];                         // the columns the database makes

all, err := users.all();                    // reads go to the database at once
ada, err2 := users.get(1);                  // a *User, null when no row has the key 1
n, err3 := users.count();

row := User{name: "Ada"};
users.create(&row);                         // changes are kept ...
ada.email = "ada@example.com";
users.update(ada);                          // the row with its key
users.delete(7);                            // by the key
err4 := context.saveChanges();              // ... and written in one transaction. row.id is set now

items := dbms.table<OrderItem>(context, "order_items");    // no key of one column: all, count and create
```

* `dbms.Table<T>` has `all`, `count` and `create`. `dbms.KeyedTable<T, K>` embeds a `dbms.Table<T>` (see
  `docs/embedding.md`) and adds `get`, `update` and `delete`, which take a key of type `K`, so `users.get("1")` is a
  compile error. A `*dbms.KeyedTable<T, K>` is a `*dbms.Table<T>` where one is wanted.
* `get` gives a pointer to the row, made with the allocator of the connection, or `null` when no row has the key.
  The error is only for what failed, e.g a lost connection.
* `schema:` gives another schema than the one of the connection, e.g `dbms.table<Sale>(context, "sales", schema: "shop")`.
* The columns are the members of `T`, in snake case: `createdAt` is `created_at`. A `_` at the end of a name is left
  out, so the member `for_` is the column `for`.
* The `generated` columns are left out of `create` and `update`, and `saveChanges` reads them back into the created
  rows (`RETURNING *`).
* For a struct written by hand, `@dbms.Column(name: "mail", key: true, generated: true, skip: true)` says the same on a
  member.
* Names are quoted in the SQL, so columns like `order` or `for` work.

#### The generator

A Saad program reads the tables from `information_schema` and the enums from `pg_enum`, and writes plain Saad files
into a folder of the project, e.g `db`:

```
saad run std/dbms/tools/generate.sd -- "host=localhost dbname=shop user=postgres" db
saad run std/dbms/tools/generate.sd -- "host=localhost dbname=shop user=postgres" sales/db sales
```

```
db/entity/user.sd      the struct of a row of users                     namespace entities
db/entity/role.sd      an enum                                           namespace entities
db/database.sd         the types of the tables, Database and open       namespace database
```

The arguments are the connection string, the folder, the schema (`public` when it is left out), and the namespaces of
the rows (`entities`) and of `database.sd` (`database`). `saad run` runs the program in the folder it is called from,
so the folder is where you are. The same is a function too: `postgres.generate(&conn, postgres.GenerateOptions{})`
gives the files, and `postgres.writeFiles(folder, files)` writes them. `tests/db/model` has all the files of a schema.

The rows and enums are all that is in the namespace `entities`, so `use entities;` brings in only them:

```odin
// db/entity/user.sd
namespace entities;

// A row of users
User :: struct {
    id:        i64;
    name:      string;
    email:     ?string;
    age:       ?i32;
    role:      Role = .guest;
    active:    bool = true;
    createdAt: string;        // timestamp with time zone
}
```

`database.sd` has a type for each table, which embeds a `dbms.Table` or `dbms.KeyedTable`, and the `Database` with
all of them:

```odin
// db/database.sd
namespace database;

import dbms;
use entities;

Database :: struct {
    @embed context: *dbms.Context;    // so db.saveChanges() is db.context.saveChanges()

    orderItems: OrderItemsTable;
    users:      UsersTable;
}

open :: (conn: *dbms.Connection) -> Database {
    tables: Database;
    tables.context = dbms.newContext(conn);
    tables.orderItems.table = dbms.table<OrderItem>(tables.context, "order_items");
    tables.users.table      = dbms.keyedTable<User, i64>(tables.context, "users", "id");
    tables.users.generated = ["id", "created_at"];
    return tables;
}

// The rows of users: all, get, count, and create, update and delete of dbms.KeyedTable
UsersTable :: struct {
    @embed table: dbms.KeyedTable<User, i64>;
}
```

A program then has:

```odin
import dbms.postgres;
import database;
use entities;

conn, err := postgres.connect("host=localhost dbname=shop user=postgres");
db := database.open(&conn);
ada := User{name: "Ada", role: .admin};
db.users.create(&ada);
err2 := db.saveChanges();
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

* The rows are plain structs: no attributes. What the tables need to know (the key, and the columns the database
  makes) is in `open`.
* The column `created_at` is the member `createdAt`. A column that is a keyword gets a `_`, e.g `"for"` is `for_`.
  Only a column that the snake case of its member isn't, e.g `"Type"`, gets `@dbms.Column(name: "Type")`.
* A default that is a constant becomes the default of the member, e.g `role: Role = .guest`, so a struct literal
  has the values the database would give. A default that is computed (`nextval(...)`, `now()`, ...) and identity
  columns are `generated`.
* A table with a key of one column is a `dbms.KeyedTable`, typed by its key. Others are a `dbms.Table`.
* Names: the table `order_items` is the row `OrderItem` in `entity/order_item.sd` (most English plurals become
  singular), the table type `OrderItemsTable` and the member `orderItems` of `Database`.
* Every file starts with `// Made by std/dbms/tools/generate.sd`, and `database.sd` lists the files. Run it again when
  the database changes: the files of tables and enums that are gone are removed, but files you wrote yourself in the
  folder are left alone. Make the files again instead of changing them.
* Building a program needs no database: a change of the database is a diff of these files.
* Typed query columns (`dbms.Field<T>`) come with step 4, as members of the table types next to the embedded table.

### 5. Saving changes

`create`, `update` and `delete` don't write anything. They add a change to the context of the tables, a
`dbms.Context`, which the generated `Database` embeds. `saveChanges` writes all of them, like `SaveChanges` of Entity
Framework:

```odin
db := database.open(&conn);

ada := User{name: "Ada"};
db.users.create(&ada);
bo, err := db.users.get(2);
bo.active = false;
db.users.update(bo);
db.categories.delete("books");

err2 := db.saveChanges();     // BEGIN, the changes in the order they were made, COMMIT
print("%", ada.id);           // what the database made, e.g the serial id and created_at, is in ada now
```

* `saveChanges` writes the changes in the order they were made, in one transaction. When one fails, it rolls back, so
  none of them are written, and the error says which one, e.g
  `create in orders (change 2 of 2): ERROR:  insert or update on table "orders" violates foreign key constraint ...`.
  The changes are kept then, so they can be fixed and saved again, or forgotten with `db.discardChanges()`.
* An `update` or `delete` of a key that no row has is an error too: `delete in users (change 1 of 1): No row has the
  key 12345`.
* `create` and `update` take a pointer to the row, which is read when `saveChanges` runs, so changes made to it after
  `create` are written too, and it must still be there then. After the commit, the row as the database stored it
  (with its generated columns) is written into the created row. Nothing is written into it when the save fails.
* `delete` takes the key, which is copied.
* `db.pendingChanges()` is the number of changes that aren't saved.
* The context and its list of changes are made with the allocator of the connection.

Not yet:

* Rows that refer to rows made in the same save: `order.userId = ada.id` is 0 before `ada` is saved. Save the user
  first. Later, the generator can read the foreign keys and fill in `userId` when the insert of `ada` gives its id.
* Batching the creates of a table into one `INSERT ... VALUES (...), (...)`.
* Tracking: `db.users.track(&row)` could keep a copy, and save the changed columns without an `update`.

### 4. The typed query builder

C#'s LINQ turns `u => u.Age > 30` into a tree that a library can read. Saad has no closures, and no code that
reads code, so the generated `Field`s stand in for the members, and their methods build the conditions:

```odin
db := database.open(&conn);

adults, err := db.users
    .where(db.users.age.atLeast(18).and(db.users.name.like("A%")))
    .orderBy(db.users.name)
    .limit(20)
    .select<User>();

// Only the columns of a smaller struct: SELECT id, name
Name :: struct { id: i64; name: string; }
names, err2 := db.users.where(db.users.email.isNull()).select<Name>();

db.users.age.atLeast("18");   // A compile error: wants an i32
```

* `atLeast :: method <T>(f: dbms.Field<T>, value: T) -> dbms.Condition` is what makes `"18"` a type error.
  Likewise `equals`, `notEquals`, `lessThan`, `atMost`, `greaterThan`, `between`, `like`, `isNull`, `oneOf`,
  and `and`, `or`, `not` on conditions.
* The builder writes the SQL with parameters, `WHERE age >= $1 AND name LIKE $2`, and runs it with `query`.
  `select` reads the rows like `query` does, so the struct decides which columns are read.
* `first<T>()`, `count()`, `exists()`, and later `update().set(field, value)` and `delete()` of the rows a query finds.
* Plain SQL stays for everything the builder can't say.

## Gaps in the language

Found while writing this, in the order they would help:

* Assigning several results to existing variables, `count, err = conn.exec(...)`. Today each call needs new
  names (`err1`, `err2`, ...) or a function around it.
* `defer`, to free a result on every return.
* Operators on structs, so `db.users.age >= 18` could be a condition instead of `atLeast(18)`.
* Attributes on structs, e.g `@dbms.Table("users")`, for tables written by hand.
* Types for dates, decimals and UUIDs. They are strings for now.
