# Checking memory

`saad build --check-memory` (also with `--llvm`) builds a program that finds a use of memory that was freed: the
memory of an arena that was cleared or freed. It is for tests and debugging, as it costs speed and addresses, and a
program built without it has none of it.

The common bug is memory of a request that outlives the request. The http server gives each request an arena, and
what a handler allocates is in it, e.g `fmt.sprint` and the values of `r.pathValue`. After the response the arena is
cleared, and the next request uses the memory again. A handler that keeps some of it, e.g in a cache, then has
pointers into memory that the next requests write over. Without the check, the cache gives the data of another
request, or doesn't find what it holds. With it, the first use throws a `Panic`:

```odin
cache: Map<string, string>;   // On the heap

getPost :: (w: *http.ResponseWriter, r: *http.Request) {
    name  := fmt.sprint("%", r.pathValue("name"));   // In the arena of the request
    found := cache.tryGet(name);                      // The next request: Panic
    ...
    cache[name] = title;                              // The bug: the key and the title are in the arena
}
```

```
Panic: Use of freed memory: entry.key points into an arena that was cleared or freed
    at findSlot (std/prelude/collections.sd:545)
    at indexOfKey (std/prelude/collections.sd:528)
    at tryGet (std/prelude/collections.sd:430)
    at getPost (posts.sd:4)
```

The server answers it with a 500 and goes on. What is kept must be copied to memory that lives as long:
`cache[name.copy(mem.heap())] = title.copy(mem.heap());`. `tests/check_memory/http_cache.sd` has both.

## What it checks

* A pointer before it is read or written through: `p.x`, `*p` and `p[i]`, where the null checks are. The receiver of a
  method is checked at the call, as for null
* The bytes of a string, slice or list when it is indexed, and both strings of `==`, `<`, ..., which C compares
* What isn't in an arena isn't checked: the heap, the stack and globals

## How it works

* The chunks of `allocators.Arena` come from one range of 64 GB of addresses (reserved, not memory), and addresses are never
  used again: `clear()` gives back all chunks, also the one it keeps without the check. So a pointer into a freed chunk
  can't point at something new
* A byte for each page of 4 KB says if the page was freed. The check of a pointer is a subtraction and a compare, and
  for a pointer into the range the load of that byte. See `checkedAllocate` in `std/prelude/errors.sd` and
  `emit_memory_check` in `src/ir_lower.c`
* A freed page is made unreadable (`mprotect`, `VirtualFree`), and its memory is given back to the system. A read of
  it that the compiler doesn't see, e.g by C (`memcpy`, `printf`), crashes instead of giving what the memory holds now.
  So does a read past the end of a chunk, as there is an unreadable page after each
* Errors and their traces are on the heap, so an error thrown out of an arena, e.g of a request, outlives it
* `#if CHECK_MEMORY { ... }` keeps code for this build only, as `allocators.Arena` does

## What it doesn't find

* Uses of freed memory by C, e.g `memcpy` of `mem.copy` or the `%s` of `printf`: they crash, without a trace
* Memory of the heap that was freed (`mem.free`): only arenas are checked yet
* Memory that is used again without being freed, e.g a `bytes.Buffer` that is cleared and written again: a view into
  it sees the new bytes. The buffers of the connections of the http server are like this
* More than 64 GB of arenas in one run: the program ends with a message then
* On Windows it compiles, but hasn't been run yet
