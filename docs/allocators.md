# Allocators

All memory of Saad comes from an `Allocator`: `new(T)`, strings, `fmt.sprint`, lists, maps, `json`, `dbms` and
`mem.allocate`. `Allocator` is an interface of the prelude (`std/prelude/collections.sd`, see `docs/interfaces.md`), so
any type with its one method is one: the heap, `mem.Arena`, and an allocator that a program or a library writes. They
all work with the rest of the standard library.

```odin
Allocator :: interface {
    // A new block when memory is null, the block with another size, keeping what fits of it, or none when newSize is 0
    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void;
}
```

* `memory` is null for a new block.
* `oldSize` is what the caller knows of the size of the block: at least the bytes it uses of it, at most the size it
  asked for, and 0 for a new one. An allocator can rely on it to copy what a block has when it grows, as `mem.Arena`
  does, but not on it being exact: a string of a `StringBuilder` is shorter than the buffer it came from.
* `newSize` is 0 to free the block.
* `align` is a power of two, 8 unless the caller asks for more, e.g `mem.allocate(4096, &arena, 4096)`.

## Using them

```odin
p := mem.allocate(64);                  // zeroed, from the allocator of the thread
p = mem.resize(p, 64, 128);             // the size it has and the size it gets
mem.free(p, 128);
point := new(Point, &arena);            // a T of new, from an allocator
mem.delete(point, &arena);              // knows the size of a Point

list.allocator = &arena;                // a list grows in the arena
title := post.title.copy(mem.heap());   // a string that lives longer
```

* A null allocator is the allocator of the thread: the heap, unless it is set. `mem.setAllocator(&arena)` sets it
  and gives the one before, and the http server sets it to the arena of a request while a handler runs.
* A list, a map, a `StringBuilder` and a `bytes.Buffer` keep the allocator of their first memory.
* `mem.heap()` is the heap. A block with an alignment over 16, which `malloc` doesn't promise, is made a bit bigger.
* `mem.Arena` is an allocator: `&arena`. Freeing a block of it does nothing, as its memory is freed all at once
  with `arena.clear()` or `arena.free()`. A smaller size keeps the block, and the last block of the arena grows where
  it is when there is room, so a list that grows in an arena isn't copied each time.

## Writing one

A struct with the method is an allocator, without naming the interface:

```odin
// Counts the bytes in use, and takes the memory from another allocator
Counting :: struct {
    inner: Allocator;
    bytes: i64;

    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void {
        self.bytes += newSize - oldSize;
        return resizeMemory(self.inner, memory, oldSize, newSize, align);
    }
}

counting := Counting{inner: mem.heap()};
names: [..]string;
names.allocator = &counting;
```

`resizeMemory(allocator, memory, oldSize, newSize, align)` of the prelude calls the allocator, or the allocator of the
thread when it is null.

## Not done yet

* `mem.setHeap`, to make another allocator the heap of the whole program.
* The chunks of `mem.Arena` come from C, not from an allocator it is given.
* `toCString` and `c.string.free()` use C's `free` for what the heap gave.
* A check that an allocator works with the standard library, e.g `mem.testAllocator`.
* More allocators: a pool for many values of one size, and a scratch arena of the thread with marks.
