# The standard library

```
std/
  prelude/      Always there, without an import
    runtime.sd      Types for reflection (Type, TypeStruct, ...), attributeOf and the runtime functions
    collections.sd  List<T> ([..]T) and Map<K, V>
    slices.sd       Methods on slices, arrays and lists: sort, reverse, indexOf, contains, ...
    expression_trees.sd  Expr<F> and ExprNode: a lambda as the tree of what it does (see docs/expression_trees.md)
    string.sd       The methods of string (slice, split, indexOf, free, ...) and of char (isDigit, isLetter, isSpace,
                    toUpper, toLower, digitValue, ...)
  c/            namespace c: bindings for the C runtime (stdio, stdlib, string)
  dbms/         namespace dbms: run SQL and read rows into structs, dbms.Table<T> and dbms.KeyedTable<T, K> (see docs/dbms.md)
    postgres/       namespace dbms.postgres: the Postgres driver, through libpq.dll, and EntityGenerator, which writes the
                    tables of a schema as Saad files
  fmt/          namespace fmt: print, sprint and StringBuilder
  io/           namespace io: bytes on stdin, stdout and stderr
  json/         namespace json: encode, decode, parse and json.Value
  net/          namespace net: TCP connections and listeners
    sys.sd          namespace net.sys: the typed bindings of Winsock and BSD sockets
    http/           namespace net.http: an HTTP/1.1 server with a router, and a client
  os/           namespace os: files, environment variables, processes
    windows.sd      namespace os.windows: the Windows API (threads, Sleep, ...)
  thread/       namespace thread: threads, Mutex, Condition and WaitGroup, on Windows and Linux
  web/          namespace web: endpoints, functions that are called at the url of their name with JSON
  vendor/       Bindings for libraries the standard library doesn't own
    raylib.sd       namespace raylib
  bytes.sd      namespace bytes: Buffer, bytes written at the end and read from the front
  math.sd       namespace math
  mem.sd        namespace mem: allocate, resize, free, copy, set, equal, and arenas
  random.sd     namespace random
  time.sd       namespace time
```

The folders only keep things tidy: a namespace is what a file says it is (see `docs/namespaces.md`). The prelude
is imported by the compiler, the rest by `import` or `use`. The files in `c/` and `prelude/runtime.sd` don't
get the prelude, as the prelude is built on them.

## fmt

```odin
fmt.print("% is % years old", name, age);          // with a newline
fmt.print("no newline", newline: false);
fmt.print("% is 100\% done", name);                // \% is a %, and %% two values
text := fmt.sprint("Score: %", score);              // like print, but returns the text
text.free();
joined := fmt.join(["a", "b", "c"], ", ");          // "a, b, c"
```

### StringBuilder

```odin
sb: fmt.StringBuilder;              // ready to use. fmt.newStringBuilder(1024) starts with room for 1024 bytes
sb.add("% + % = %", 1, 2, 3);       // formatted like fmt.print
sb.addLine("total: %", total);      // with a newline after it
sb.addString(name);
sb.addChar(',');
sb.addRepeated(' ', 4);             // e.g indentation
sb.addValue(person);                // a value like fmt.print writes it
sb.addQuoted(name);                 // "Ada", with escapes. addQuotedChar('a') is 'a'

for n in numbers { sb.add("%, ", n); }
sb.removeLast(2);                   // the ", " after the last number

sb.length(); sb.isEmpty(); sb.endsWith("!");
text := sb.toString();              // the text, not a copy: it changes with the builder. .copy() to keep it
sb.clear();                         // empty, and keeps its memory for more
sb.free();
```

The text of a StringBuilder is always followed by a 0 byte, so `sb.buffer` can be given to C.

Values are written as they are written in Saad, so printed values can be copied back into a program:

```odin
fmt.print("%", Person{name: "Ada", age: 36});   // Person{name: "Ada", age: 36}
fmt.print("%", ["a", "b"]);                     // ["a", "b"]
fmt.print("%", Shape{circle: Circle{2.0}});     // Shape{circle: Circle{radius: 2.000000}}
fmt.print("%", "Ada");                          // Ada. A string on its own has no quotes
```

Strings in a struct, an array or a union get quotes, with `"`, `\` and newlines escaped, and a char gets single quotes
(`'a'`). An enum value is `.open`,
an anonymous struct `{w: 1.000000, h: 2.000000}`, and the variant without a value of a generic union `.none`.

## os

```odin
text, err := os.readFile("config.txt");     // the whole file. err is an os.NotFound when it isn't there
text := os.readFile("config.txt");          // the same, and the error is thrown (see docs/errors.md)
os.writeFile("out.txt", "Hello");           // creates or replaces. An os.IOError when it can't, thrown here
os.appendFile("log.txt", "line\n");
os.fileExists("out.txt");                   // a bool: it asks, and can't fail
os.renameFile("out.txt", "new.txt");
_ := os.removeFile("new.txt");              // taken: a file that isn't there isn't thrown
os.makeDir("out");                          // also fine when it was there before
home, found := os.getEnv("HOME");
code := os.run("echo hi");                  // runs a shell command, returns its exit code
text, code := os.output("git status");      // runs a shell command, returns what it wrote and its exit code
args := os.args();                          // the command line, args[0] is the program
os.exit(1);
```

`os.output` runs the command with cmd.exe and puts it in quotes first, so a command that starts with a quoted path
works, e.g `os.output("\"C:/Program Files/tool.exe\" \"my file.txt\"")`.

`os.windows` has the bindings for the Windows API, e.g `CreateThread`, critical sections, condition variables,
`Sleep`, `GetStdHandle`, `ReadFile` and `WriteFile`.

## io

Bytes on the standard streams, for a program that another program talks to, e.g the language server (`lsp/`):

```odin
line, ok := io.readLine();      // without "\n" or "\r\n". ok is false at the end of the input
body, ok := io.read(52);        // exactly 52 bytes
waiting := io.hasInput();       // bytes have arrived, so a read won't wait
io.write("hello\n");            // stdout, nothing added
io.writeError("a log line\n");  // stderr
```

The input is binary, so `\r\n` stays as it is. `fmt.print` goes through the C runtime, which has its own buffer, so
don't mix it with `io.write`.

## time

Dates and times like the time package of Go:

```odin
t := time.now();                                // a time.Time, in UTC
birthday := time.date(1815, 12, 10);             // also hour, minute, second, nanosecond and offset
t.year(); t.month(); t.day(); t.hour(); t.weekday(); t.yearDay();

due := t.add(90 * time.Minute).addDate(0, 1, 0); // a Duration is nanoseconds: an i64
waited := due.sub(t);                            // a Duration
if due.after(t) { ... }                          // also before, equal and compare
fmt.print(time.durationText(waited));            // e.g 1h30m0s

fmt.print(t.format("Mon 2 Jan 2006 15:04"));     // Go's layout of Mon Jan 2 15:04:05 2006 -07:00
fmt.print(t.format(time.RFC3339));               // also DateTime, DateOnly and TimeOnly
parsed, ok := time.parse("2026-11-02 10:15:00.25+01");   // RFC 3339 and ISO 8601, like Postgres writes them
copenhagen := t.inOffset(3600);                  // the same instant, shown at +01:00

seconds := t.unix();
again := time.fromUnix(seconds);
elapsed := time.since(start);
```

* A `Time` is seconds since January 1, 1970 UTC, the nanoseconds after them, and the offset of the zone it is shown
  in. There are no named zones yet, only offsets. The zero `Time` is January 1, 1970 UTC.
* `==` compares the zone too; `equal` compares the instant.
* `fmt` prints a `Time` as RFC 3339 text, and `json` writes and reads it as a string. `dbms` reads timestamp and date
  columns into it.
* The layouts of `format`: `2006 06` year, `01 1 Jan January` month, `02 2 _2` day, `002` day of the year,
  `Monday Mon` weekday, `15 03 3` hour, `04 4` minute, `05 5` second, `PM pm`, `.000` (or `.999` without zeros at the
  end) fractions, `-07:00 -0700 -07` the offset, and `Z07:00 Z0700 Z07` with `Z` for UTC.

For measuring and waiting:

```odin
start := time.clock();              // seconds, high resolution
time.sleep(100);                    // milliseconds

watch := time.startStopwatch();
...
fmt.print("took % ms", watch.milliseconds());   // also seconds() and restart()
```

## bytes

`bytes.Buffer`: bytes that are written at the end and read from the front, like Go's `bytes.Buffer` and `bufio`, e.g
the bytes of a socket.

```odin
buffer: bytes.Buffer;                      // Ready to use, or bytes.newBuffer(capacity, allocator)
space := buffer.space(4096);               // Room at the end ([]u8), to read into
buffer.wrote(n);                           // The n bytes that were read into it
line, found := buffer.readLine();          // Without \n or \r\n. found is false when the line isn't all there
headers, ended := buffer.readUntil("\r\n\r\n");
body, ok := buffer.read(length);
```

* Writing: `write(text)`, `writeByte(b)`, and `space(n)` with `wrote(n)`.
* Reading: `read(n)`, `readUntil(separator)`, `readLine()`, `readByte()`, `peekByte()` and `skip(n)`. A read gives
  a view of the buffer, without a copy, so it changes when the buffer does: `copy()` it to keep it. Nothing is read
  when the bytes aren't all there yet.
* `unread()`, `available()`, `toString()`, `clear()`, `compact()` and `free()`. When it grows, the bytes that are
  read are dropped first.

## net

TCP over IPv4, on Windows (Winsock) and Linux (BSD sockets).

```odin
listener := net.listen(":8080");         // ":0" lets the system choose the port: listener.addr.port
conn := listener.accept();
n, err := conn.read(buffer);              // []u8. n is 0 with net.Closed when the other end closed
conn.write("hello\n");
conn.setReadTimeout(5000);                // Milliseconds. A read that waits longer gives net.Timeout
conn.close();

conn := net.dial("example.com:80");
addr := net.resolve("localhost", 80);     // net.Addr: ip and port, toString() is 127.0.0.1:80
```

* Errors are of the kinds `net.Timeout`, `net.Closed`, `net.Refused`, `net.InUse` and `net.NetError`.
* `listener.close()` wakes an `accept()` that waits, which gives `net.Closed`.
* The bindings are in `net.sys`, with the structs of the headers written out for both systems (`SockAddrIn`,
  `AddrInfo`, `WsaData`, `TimeVal`) and the types of the calls named (`Socket`, `Length`, `AddrLength`).
* IPv6 and UDP aren't there yet.

## net.http

HTTP/1.1, like Go's `net/http`: a `Router` routes requests to handlers, a `Server` answers them on a pool of threads,
and a client sends requests.

```odin
import net.http;

showPost :: (w: *http.ResponseWriter, r: *http.Request) {
    id := r.pathValue("id");
    page, given := r.queryValue("page");    // Of ?page=2, decoded
    kind, _ := r.header("Content-Type");     // Without case
    w.setHeader("Content-Type", "application/json");
    w.writeHeader(201);                      // 200 when it isn't set
    w.write(json.encode(post));
}

router: http.Router;
router.handle("GET /posts/{id}", showPost);
router.handle("POST /posts", createPost);
router.handle("/static/", files);           // Every path below /static/
http.listenAndServe(":8080", &router);

response := http.get("http://localhost:8080/posts/1");
defer response.free();
// response.status, response.body, response.header("Content-Type")
response = http.post(url, "application/json", body);
response = http.request("PUT", url, headers, body);
```

* **Patterns:** `[METHOD ]/path`. A segment `{name}` matches any segment, and a path that ends with `/` matches
  every path below it. The pattern with the most segments that are written out wins. `GET` also answers `HEAD`.
  A path that matches with another method gives a 405 with `Allow`, and no match a 404 (or `router.notFound`).
* **Responses:** a handler writes into a buffer, and the response is sent when it returns, with `Content-Length`.
  `w.error(status, text)` and `w.redirect(url)` help. A handler that throws gives a 500, and the error is written to
  stderr (`server.log`); the server goes on.
* **Server:** `server.listen(address)`, then `server.serve()`, which returns after `server.shutdown()`. Fields:
  `router` or `handler`, `workers` (0 is 2 for each processor, at least 4), `readTimeout` and `writeTimeout`
  (milliseconds), and `limits` (`headBytes`, `bodyBytes`). Connections are kept alive, and bodies can be
  `Content-Length` or chunked. Bad requests get 400, 408, 413, 431, 501 or 505.
* **Memory:** the texts of a request are in an arena of the connection, freed when the handler returns. `copy()`
  what is kept for longer.
* **Client:** `get`, `post` and `request`, a connection for each request. The texts of a response are freed with
  `response.free()`. Only `http://`: put the server behind a proxy, e.g Caddy or nginx, for https.

## web

Endpoints: a backend whose API is its functions. A function with `@web.endpoint` is called at the url of its name,
with its parameters from the JSON of the request, and its result is the JSON of the response.

```odin
import web;

Session :: struct {
    @embed base: web.Session;    // request, response and app
    db: *blog.Db;                // What the program wants in each session
}

@web.endpoint(.get)
getPosts :: (session: *Session, authorId: int) -> [..]Post { ... }

@web.endpoint                     // POST
createPost :: (session: *Session, post: Post) -> Post, error { ... }

main :: () {
    app := web.newApp<Session>("/v1");
    app.onStart(openSession);     // Called with each new session, e.g to give it a database context
    app.onFinish(closeSession);   // After the endpoint, also when it threw
    app.listenAndServe(":8080");
}
```

```
GET  /v1/getPosts?authorId=2                         -> [{"id": 1, ...}]
POST /v1/createPost   {"post": {"title": "Hi"}}      -> {"id": 3, "title": "Hi"}
```

* The url is the prefix and the name of the function, so the names of endpoints must be unique: `newApp` gives an
  error when two are the same, or when an endpoint doesn't take the session as its first parameter.
* POST takes the parameters as the members of a JSON object in the body, and GET from the query (a string as it is,
  other types as JSON, e.g `?ids=[1,2]`). A parameter that isn't there is a 400, unless it is an optional (`?int`).
* The result is JSON: the value, an array of the values when there are more, or `null`. A given or thrown error is
  `{"error": message, "kind": kind}`, with the status of its kind: `web.BadRequest` 400, `web.Unauthorized` 401,
  `web.Forbidden` 403, `web.NotFound` 404, `web.Conflict` 409, others 500. A `Panic` is a 500 that doesn't tell what
  went wrong.
* The session is new for each request, and freed with it. While the endpoint runs, the allocator of the thread is the
  arena of the request, so what it allocates, also its result, is freed after the response. Give what lives longer
  the heap, e.g `post.title.copy(mem.heap())`. A `dbms.Pool` gives each session a connection (see `docs/dbms.md`).
* `app.server` is the `http.Server`, e.g `app.server.workers` or `app.server.shutdown()`.
* It is built on `attributedFunctions()` and their `invoke()` (see `docs/reflection.md`). A TypeScript client can be
  made from the same: the names, parameters and results of the endpoints.

## thread

Threads, and what they share safely. On Windows (`CreateThread`, `SRWLOCK`, `CONDITION_VARIABLE`) and Linux
(`pthread`), with `#if` (see `docs/compile_time_if.md`).

```odin
import thread;

work :: (job: *Job) {
    ...
    job.group.done();
}

group: thread.WaitGroup;
group.add(jobs.count);
for i in 0..jobs.count {
    t := thread.spawn(work, &jobs[i]);   // A Thread, or an error when it can't be started
}
group.wait();

counter.mutex.lock();
counter.value += 1;
counter.mutex.unlock();
```

* `spawn(function, argument)` starts a thread that runs `function(argument)`, e.g `spawn(work, &job)` for
  `work :: (job: *Job)`, and `t.join()` waits for it to end. `start(function)` runs a function without an argument.
  An error that the function doesn't catch ends the program, with its trace, like one in `main`.
* `Mutex` (`lock`, `unlock`), `Condition` (`wait(&mutex)`, `signal`, `broadcast`) and `WaitGroup` (`add`, `done`,
  `wait`). Their zero values are ready to use. Don't copy one that is in use.
* `sleep(milliseconds)`, `yield()`, `currentId()` and `processorCount()`.
* Each thread has its own `@threadLocal` globals (see `docs/namespaces.md`), e.g the error that is being thrown.

## random

Named like Python's random module. A xorshift64* generator: fast and good for games and simulations, not for
security.

```odin
dice  := random.between(1, 6);      // 1 to 6, both included
f     := random.random();           // from 0 up to but not including 1
x     := random.uniform(-1.0, 1.0); // from -1 up to but not including 1
name  := random.choice(names);      // one of them
picks := random.choices(names, 3);  // 3 of them, maybe the same one more than once
some  := random.sample(names, 2);   // 2 different ones
random.shuffle(cards);              // any slice, list or fixed array
if random.chance(0.1) { ... }       // true one in ten times
height := random.gauss(170.0, 10.0); // normal distribution with a mean and a standard deviation
bits  := random.bits();             // 64 random bits
```

* The functions use a generator that is seeded from the clock the first time it is used. `random.seed(42)` makes
  them repeatable.
* `rng := random.newRandom(42);` is a generator of your own, with the same methods: `rng.between(1, 6)`,
  `rng.choice(names)`, `rng.shuffle(cards)`, ...

## Methods on slices (prelude)

Slices, fixed arrays and lists have these methods without an import. They are generic methods on `[]T` in
`std/prelude/slices.sd`, so any library can add methods like them.

```odin
xs := [3, 1, 2];
xs.sort();                          // with <, quicksort with insertion sort for small ranges
xs.isSorted();
xs.reverse();
xs.indexOf(3);                      // -1 when not found, compares with ==
names.contains("anna");
xs.swap(0, 1);
```

With a function, which can be a function literal or the name of a function:

```odin
doubled := xs.map((x: int) -> int { return x * 2; });   // a new [..]int
names   := users.map(nameOf);                           // [..]string, U is inferred from nameOf
ids     := users.map<i64>(idOf);                        // or given
adults  := users.filter(isAdult);                       // the elements that it is true for, in order
total   := prices.reduce(0.0, (sum: f64, p: f64) -> f64 { return sum + p; });
user := users.find(isAdult);                            // the first one, a ?User that is empty when there is none
users.findIndex(isAdult);                               // -1 when there is none
users.some(isAdult);  users.every(isAdult);  users.countWhere(isAdult);
```

`map` and `filter` take an allocator for the new list as their last argument, e.g `xs.filter(isEven, &alloc)`. Null,
the default, is the heap. A function type can leave out the names of its parameters: `(T) -> U`.

A type's own method of the same name wins, e.g `List` has its own `indexOf`, `contains` and `reverse`.

## json

```odin
Person :: struct {
    @json.Field(name: "first_name")
    firstName: string;
    age:       int = 18;
    tags:      [..]string;
    address:   *Address;
    extra:     json.Value;            // any JSON

    @json.Field(skip: true)
    cache: *void;

    @json.Field(omitEmpty: true)
    nickname: string;
}

text := json.encode(person);              // {"first_name":"Ada","age":36,...}
text := json.encode(person, json.Pretty); // over more lines

p, err := json.decode<Person>(text);      // err is null when it worked, otherwise a json.DecodeError, e.g "address.zip: Expected a number, but got a string"
err2 := json.decodeInto(text, &settings); // into a value that exists, which keeps what the JSON doesn't have

value, parseErr := json.parse(text);      // a json.Value of any shape
name := value.get("first_name").asString();
first := value.get("tags").at(0).asString("none");
value.toJson();
```

* `encode` and `decode` work through reflection: bool, numbers, strings, enums (by name), structs, unions,
  fixed arrays, slices, `List<T>`, `Map<string, V>`, pointers and `json.Value`. A union is the name of a variant
  without a value, e.g `"empty"`, or an object with the name and the value, e.g `{"circle": {"radius": 2}}`.
* `json.Field` is an attribute (see `docs/reflection.md`): `name` renames a member, `skip` leaves it out, and
  `omitEmpty` leaves out 0, false, "", null and things without elements.
* Optionals (`?T`) are written as their value or `null`, and `null` is read as empty. See `docs/optionals.md`.
* Decoding keeps members that are not in the JSON, so default values stay, and skips members it doesn't know.
  `decodeInto` keeps the values a value already has.
  Integers are read from the text, so an `i64` or `u64` doesn't lose digits, and they must fit their type.
* Both take options as the last argument:

  ```odin
  json.encode(person, json.Pretty);         // indent: 2
  person, err := json.decode<Person>(text, json.SnakeCase);
  json.encode(person, json.Options{indent: 4, names: .snakeCase, enumsAsNumbers: true, omitEmpty: true});
  ```

  | Option | Default | |
  | --- | --- | --- |
  | `indent` | 0 | Spaces per level. 0 writes everything on one line |
  | `names` | `.asWritten` | How member names are written and read: `.camelCase`, `.pascalCase`, `.snakeCase`, `.kebabCase`, `.upperSnake`. A name from `json.Field` is used as it is |
  | `enumValues` | `.asWritten` | The same for the values of enums |
  | `enumsAsNumbers` | false | Writes enums as numbers. Both are read |
  | `omitEmpty` | false | `omitEmpty` for every member |
  | `strict` | false | Decoding fails on members the struct doesn't have |

  The standard options are `json.Compact`, `json.Pretty`, `json.CamelCase`, `json.SnakeCase` and `json.KebabCase`.
  `json.styleName("userID", .snakeCase)` gives `user_id`: words are split where the case changes and at `_` and `-`.
* `json.Value` has `get`, `at`, `has`, `length`, `keys`, `asString`, `asNumber`, `asInt`, `asBool`, `isNull`,
  `toJson`, `clone` and `free`, and is made with `newObject`, `newArray`, `newString`, `newNumber`, `newBool`,
  `newNull`, `put` and `push`.

## dbms

`users, err := conn.query<User>("SELECT * FROM users WHERE id > $1", 10)` reads rows into structs, `conn.exec` runs
statements and `conn.queryOne` reads one row or value. `dbms.postgres` connects to Postgres. See `docs/dbms.md`.

## new

`new` makes one value in memory of its own and gives a pointer to it. The memory comes from an `Allocator`, the heap
when none is given, so a program doesn't need `malloc`:

```odin
p := new(Player);                  // a *Player with the defaults of Player, e.g health: int = 100
q := new(Player, &allocator);      // from an allocator, e.g of an arena
r := new Point{3, 4};              // a copy of a value, also new(Point{3, 4})
n := new(int);
freeMemory(p);                     // or mem.free(p), with the allocator it came from
```

`new(T, allocator)` is `allocateNew<T>(allocator)` and `new value` is `allocateCopy(value)`, functions of the prelude
(`std/prelude/collections.sd`) that take the memory with `resizeMemory`.

## math, mem, c

* `math`: the functions of C's `math.h` (`sin`, `sqrt`, `pow`, ...).
* `mem`: memory, so the standard library and programs don't call C for it. `mem.allocate(size, allocator)` gives
  zeroed memory, `mem.resize(p, size, allocator)` is like `realloc`, `mem.free(p, allocator)`, and `mem.copy(to, from,
  n)`, `mem.set(to, byte, n)` and `mem.equal(a, b, n)` are like `memcpy`, `memset` and `memcmp`. The allocator is
  optional: the allocator of the thread when there is none. The standard library uses these and `new`; `mem` and the
  prelude are the only places that call C for memory.
* **The allocator of the thread** is what a null allocator means, e.g for `new(T)`, strings, `fmt.sprint`, lists,
  maps, `json` and `dbms`. It is the heap unless it is set: `mem.setAllocator(&alloc)` sets it and gives the one before,
  and `mem.allocator()` reads it. The http server sets it to the arena of the request while a handler or an endpoint
  runs, so everything it allocates, also its result, is freed after the response, without `free`.
* A list, a map, a `StringBuilder` and a `bytes.Buffer` keep the allocator of their first memory, so they grow and
  are freed with it after the allocator of the thread changed.
* What lives longer than a request is given the heap, `mem.heap()`: `store.posts.allocator = mem.heap();` or
  `title.copy(mem.heap())` (`copy` and `free` of a string take an allocator).
* `mem.Arena`: memory that is freed all at once, and the sizes `Kilobyte` and `Megabyte`. The zero value is
  ready to use: `arena.allocate(size)` gives zeroed memory, `arena.allocator()` an `Allocator` for lists, maps and
  `dbms.Connection`, and `arena.clear()` and `arena.free()` free all of it.

```odin
arena: mem.Arena;
alloc := arena.allocator();
names: [..]string;
names.allocator = &alloc;      // the list grows inside the arena
arena.free();
```
* `c`: `printf`, `fopen`, `strlen`, ... Use it when nothing else fits; for memory, use `new` and `mem`. Sizes that are
  `size_t` in C are `i64` here.
* `c.string`: a string of C, the pointer to its bytes, which end with a 0. It is passed to C like a `*u8`, and the
  bindings (`c`, `os.windows`, libpq, raylib) take and give it where C has a `char *`. A string literal is one where
  one is wanted, as its bytes end with a 0. A `string` isn't, as it doesn't have to end with a 0: `s.toCString()`
  makes a copy that does.

```odin
greet :: (name: c.string) { printf("hello %s\n", name.data); }   // A vararg of C is the pointer itself: .data

greet("Ada");                       // a literal
cs := name.toCString();             // a copy of a string, with a 0 after it
greet(cs);
cs.free();

home := getenv("HOME");             // a c.string from C
if !home.isNull() {
    text := home.toString();        // a string of the same bytes, without copying them. length() counts them
}
p: *u8 = ...;
c.string{p}.toString();             // a pointer of C, made into one
```

## Ideas for later

* `strings` with `contains`, `trim`, `replace`, `join` and a `parseInt`/`parseFloat`.
* `os.listDir`, and a `path` namespace (`join`, `extension`, `directory`).
* `math` functions of its own: `Pi`, `clamp`, `lerp`, `min`/`max`, vectors.
* `hash` (e.g fnv1a, crc32), `base64` (e.g fnv1a, crc32) and `threads` (a portable layer over `os.windows`).
