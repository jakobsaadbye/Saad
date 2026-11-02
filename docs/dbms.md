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
    createdAt: time.Time;  // reads the column created_at
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

Status: steps 1 to 5 of the plan below are done: `dbms`, `dbms.postgres`, rows into structs, transactions,
allocators, `dbms.Table<T>`, the generator that writes the tables of a database as Saad, queries with lambdas, and
saving changes.

`examples/blog` is a whole program: a schema, the code the generator writes of it, and an app that creates,
queries, changes and removes rows. With the generator, the program above is:

```odin
import dbms;
import dbms.postgres;
import database;          // written by std/dbms/tools/generate.sd
use entities;             // the rows, e.g User

main :: () {
    conn, err := postgres.connect("host=localhost dbname=shop user=postgres password=secret");
    db := database.open(&conn);

    ada := db.users.create();                    // a new *User with the defaults, kept until saveChanges
    ada.name = "Ada";
    ada.role = .admin;
    err1 := db.saveChanges();                    // writes the changes in one transaction, and the id into ada

    found, err2 := db.users.get(ada.id);         // a *User, null when there is none
    users, err3 := db.users.all();

    minAge := 18;
    adults, err4 := db.users.where(u => u.age ?? 0 >= minAge).orderBy(u => u.name).all();
    names, err5  := db.users.where(u => u.role == .admin).select(u => {name: u.name, email: u.email}).all();
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
| `timestamptz`, `timestamp`, `date` | `time.Time`. A `timestamp` without a zone is UTC, and a `date` is midnight UTC. It is sent as RFC 3339 text |
| `text`, `varchar`, and everything else | `string`, as Postgres writes it, e.g a `numeric` as `12.50` |
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
4. **Done** Queries with lambdas, like LINQ: `db.users.where(u => u.age >= 18).select(u => {name: u.name}).all()`.
5. **Done** A unit of work: `create`, `update` and `delete` keep the changes, and `db.saveChanges()` writes them all
   in one transaction.
6. **Done** Relations from the foreign keys: `post.author()` and `author.posts()`.
7. Later: loading relations with a join (like `Include` of Entity Framework), a pool of connections, a SQLite driver,
   migrations, and Postgres spoken directly over a socket instead of libpq.

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

row := users.create();                      // a new *User. Changes are kept ...
row.name = "Bo";
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
    createdAt: time.Time;
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
ada := db.users.create();
ada.name = "Ada";
ada.role = .admin;
err2 := db.saveChanges();
```

| Postgres | Saad |
| --- | --- |
| `smallint`, `integer`, `bigint` (and `serial`) | `i16`, `i32`, `i64` |
| `real`, `double precision` | `f32`, `f64` |
| `boolean` | `bool` |
| an enum of the schema | an enum, with the labels as they are when they are names |
| `text`, `varchar`, `char` | `string` |
| `timestamptz`, `timestamp`, `date` | `time.Time`, and the file imports `time` |
| anything else, e.g `numeric`, `uuid`, `json` | `string`, with the type of the column as a comment |
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
`dbms.Context`, which the generated `Database` embeds. The rows that `get` and `all` give are tracked. `saveChanges`
writes all of it, like `SaveChanges` of Entity Framework:

```odin
db := database.open(&conn);

ada := db.users.create();     // a new *User with the defaults of User
ada.name = "Ada";
bo, err := db.users.get(2);   // tracked
bo.active = false;            // so this is saved without an update
db.categories.delete("books");

err2 := db.saveChanges();     // BEGIN, the changes in the order they were made, the tracked rows, COMMIT
print("%", ada.id);           // what the database made, e.g the serial id and created_at, is in ada now
```

* `saveChanges` writes the changes in the order they were made, and then the tracked rows that changed, in one
  transaction. When one fails, it rolls back, so none of them are written, and the error says which one, e.g
  `create in orders (change 2 of 2): ERROR:  insert or update on table "orders" violates foreign key constraint ...`.
  The changes are kept then, so they can be fixed and saved again, or forgotten with `db.discardChanges()`.
* An `update` or `delete` of a key that no row has is an error too: `delete in users (change 1 of 1): No row has the
  key 12345`.
* `create()` gives a new row, with the defaults of the struct (which are the constant defaults of the database), made
  with the allocator of the connection. Set its members before `saveChanges`. After the commit, the row as the database
  stored it (with its generated columns) is written into it. Nothing is written into it when the save fails.
* Creates of one table that follow each other are one `INSERT ... VALUES (...), (...) RETURNING *`, of at most
  `dbms.MaxRowsInInsert` (1000) rows, and fewer when the rows have so many columns that they would take more than the
  65535 parameters Postgres allows. Postgres gives the rows back in the order of the values, so each row gets its own
  id. A create of another table in between starts a new `INSERT`, so the order of the changes stays the same.
* `update` takes a pointer to a row and writes every column that isn't the key or generated. It is for a row that
  isn't tracked: a row that `get` or `all` gave is saved without it.
* `delete` takes the key, which is copied.
* `db.pendingChanges()` is the number of changes that `saveChanges` would write, with the tracked rows that changed.
* The context, its list of changes and the copies of the tracked rows are made with the allocator of the connection.

#### Tracking

* `get` and `all` of a table with a key track the rows they give: the context keeps a copy of each. `saveChanges`
  compares each tracked row with its copy and writes an `UPDATE` of only the columns that changed, to the row with the
  key it was read with. Strings and optionals are compared by their values.
* The created rows are tracked after they are saved, so a change after `saveChanges` is saved by the next one.
* A tracked row that is also given to `update` is written once, by the `update`. A tracked row that is deleted isn't
  updated, and isn't tracked any more.
* After a save, the copies are the rows as they are, so only what changes after it is saved next time.
  `discardChanges()` does the same without saving: what was set on the rows stays, but isn't saved.
* `db.track = false` (it is a member of the embedded context) turns tracking off, e.g before reading many rows that
  won't change. `db.untrackAll()` forgets the tracked rows, e.g before the list that `all` gave is freed.
* The rows of `all` are in the list it gives, so they must still be there at `saveChanges`. Don't add to that list, as
  its rows can move.

Not yet:

* Rows that refer to rows made in the same save: `order.userId = ada.id` is 0 before `ada` is saved. Save the user
  first. Later, the generator can read the foreign keys and fill in `userId` when the insert of `ada` gives its id.

### 4. Queries with lambdas

Like LINQ of C#: the lambdas are expression trees (`Expr<F>`, see `docs/expression_trees.md`) that `std/dbms/query.sd`
makes into SQL. The values they use are parameters, so `minAge` below is sent as `$1`.

```odin
db := database.open(&conn);
minAge := 18;

adults, err := db.users
    .where(u => u.age ?? 0 >= minAge && u.name.startsWith("A"))
    .orderBy(u => u.name)
    .limit(20)
    .all();                                   // SELECT * FROM users WHERE ... ORDER BY "name" LIMIT 20

ada, err2  := db.users.where(u => u.email == email).first();          // a *User, null when there is none
n, err3    := db.users.where(u => u.email == null).count();           // IS NULL
admins, err4 := db.users.where(u => u.role == .admin).exists();

rows, err5  := db.users.select(u => {name: u.name, nextYear: (u.age ?? 0) + 1}).all();   // [..]struct { name: string; nextYear: i32; }
names, err6 := db.users.orderByDescending(u => u.id).select(u => u.name).all();          // [..]string

db.users.where(u => u.age >= "18");   // A compile error, as the lambda is checked like any other
```

* `where`, `orderBy`, `orderByDescending`, `limit`, `offset` and `select` give a new `dbms.Query<T>`, so a query can
  be the start of others. Nothing is sent before `all`, `first`, `count` or `exists`. `db.users.query()` is the
  query of every row, and `query.toSql()` gives the SQL and its parameters.
* In a lambda the members of the row are its columns. These can be made into SQL: `==` (`=`), `!=`, `<`, `<=`, `>`,
  `>=`, `&&` (`AND`), `||` (`OR`), `!` (`NOT`), `+`, `-`, `*`, `/`, `%`, `??` (`coalesce`), `== null` (`IS NULL`),
  on text `startsWith(s)`, `endsWith(s)`, `indexOf(s)`, `toLower()`, `toUpper()` and `.len`, and on times `after(t)`,
  `before(t)` and `equal(t)`. A column that can be `NULL` takes `?.`, e.g `p.publishedAt?.after(since) ?? false`. Anything else, e.g
  another method, is an error that `all` gives. A part without the row, e.g `minAge - 1` or `now()`, is a value.
* The rows of `all` and `first` of a keyed table are tracked, like those of `get`, so changes to them are saved by
  `saveChanges`. The rows of `select` are not: they are anonymous structs or values made of some columns.
* `select` gives a struct literal with names, whose names are the columns of the result, or one value. A `where`
  after `select` is an error; put it before.

### 6. Relations

The generator reads the foreign keys of one column, and writes a method for each end on the rows. For
`posts.author_id`, which refers to `authors.id`:

```odin
Post :: struct {
    ...
    authorId: i64;

    @Hidden dbLink: dbms.Link;   // The database it came from. Not a column, and fmt and json leave it out

    // The row of authors that authorId refers to, or null
    author :: method () -> (*Author, string) { ... }
}

Author :: struct {
    ...
    // The rows of posts whose author_id is the id of this one, as a query
    posts :: method () -> dbms.Query<Post> { ... }
}
```

so a program has

```odin
author, err := post.author();
published, err2 := author.posts().where(p => p.status == .published).orderBy(p => p.title).all();
count, err3 := author.posts().count();
```

* The row a relation gives is tracked like the row of `get`, so changes to it are saved by `saveChanges`. A row that
  is tracked already is given again, instead of a copy: `post.author()` twice gives the same `*Author`, and so does
  the author that was just created and saved.
* The many end is a `dbms.Query<T>`, so it is narrowed, sorted, counted or selected from like any query.
* A foreign key that can be `NULL` gives null when it is `NULL`, e.g `user.referredByRow()`.
* Names: `author_id` gives `author()`; a column without `_id`, e.g `referred_by`, gives `referredByRow()`. The other
  end is the name of the table, e.g `posts()`, or with the column when a table has two foreign keys to the same
  table, or to itself, e.g `postsByEditor()` and `usersByReferredBy()`. A name that a member has gets `Row` or
  `Rows` at its end.
* The rows know their database through a hidden member, a `dbms.Link`, which the tables set on the rows they give:
  `create`, `get`, `all` and the queries. A row made otherwise, e.g `Post{...}`, has none, and its relations give
  the error `dbms.NoLinkError`. `database.open` registers the tables with the context, so the link finds them.
* Under the hood: `dbms.related<T>(link, table, column, value)` and `dbms.relatedRows<T>(link, table)`, in
  `std/dbms/relations.sd`.

## Gaps in the language

Found while writing this, in the order they would help:

* Assigning several results to existing variables, `count, err = conn.exec(...)`. Today each call needs new
  names (`err1`, `err2`, ...) or a function around it.
* `defer`, to free a result on every return.
* Attributes on structs, e.g `@dbms.Table("users")`, for tables written by hand.
* Types for decimals and UUIDs. They are strings for now. Times are `time.Time`, but without named zones.
