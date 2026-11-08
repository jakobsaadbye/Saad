# Allocators

All memory of Saad comes from an `Allocator`: `new(T)`, strings, `fmt.sprint`, lists, maps, `json`, `dbms` and
`mem.allocate`. `Allocator` is an interface (see `docs/interfaces.md`), so any type with its three methods
is one: the heap, `allocators.Arena`, `allocators.Pool`, and an allocator that a program or a library writes. They all
work with the rest of the standard library.

Where they are:

| File | |
| --- | --- |
| `std/prelude/allocator.sd` | The `Allocator` interface, the allocator of the thread, `resizeMemory`, `freeMemory`, `resizeByCopy` and the heap (`Heap`). In the prelude, as `List`, `Map` and `new` need them |
| `std/allocators/arena.sd` | `allocators.Arena` |
| `std/allocators/pool.sd` | `allocators.Pool` |
| `std/mem.sd` | The functions that use them, e.g `mem.allocate`, `mem.free` and `mem.heap()` |

`allocator.sd` is in the lowest layer of the prelude (see `docs/standard_library.md`), so it can't use `panic`. The
allocators of the namespace can use all of the prelude. A new allocator goes in the namespace, as a file of its own.

```odin
Allocator :: interface {
    allocate   :: method * (size: i64, align: i64) -> *void;                          // null when there is no memory
    resize     :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void;   // can move it
    deallocate :: method * (memory: *void, size: i64, align: i64);
}
```

* A size is what the caller knows of the size of a block: at least the bytes it uses of it, and at most the size it
  asked for. An allocator can rely on it to copy what a block has when it grows, as `allocators.Arena` does, but not on it
  being exact: a string of a `StringBuilder` is shorter than the buffer it came from.
* `align` is a power of two, 8 unless the caller asks for more, e.g `mem.allocate(4096, &arena, 4096)`.
* What frees all of an allocator at once, e.g `arena.clear()` and `pool.free()`, isn't in the interface, as not every
  allocator can do it, e.g the heap. It is a method of the allocator itself.

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
* `resizeMemory(allocator, memory, oldSize, newSize, align)` is the realloc of an allocator: it calls
  `allocate` when memory is null, `deallocate` when newSize is 0, and else `resize`.

## The allocators of the standard library

* `mem.heap()`: `malloc`, `realloc` and `free`. A block with an alignment over 16, which `malloc` doesn't promise, is
  made a bit bigger.
* `allocators.Arena`: memory that is freed all at once with `arena.clear()` or `arena.free()`. `deallocate` does nothing. A
  smaller size keeps the block, and the last block grows where it is when there is room, so a list that grows in an
  arena isn't copied each time.
* `allocators.Pool`: blocks of one size, e.g for the nodes of a tree. A block that is given back goes on a list and is the
  next one given out, so there is no searching and no fragmentation, and unlike an arena, blocks are given back one by
  one. It takes chunks of `perChunk` blocks from another allocator, `inner` (the heap when it is null).

```odin
pool := allocators.Pool{blockSize: sizeof(Node)};
node := new(Node, &pool);
mem.delete(node, &pool);               // the next new(Node, &pool) gets this block
pool.free();                           // gives back all the chunks
```

## Writing one

A struct with the three methods is an allocator, without naming the interface. This is the `Pool` of
`std/allocators/pool.sd`, shortened:

```odin
Pool :: struct {
    blockSize: i64;            // At least 8, as a free block holds the pointer to the next one
    perChunk:  i64 = 64;       // The blocks of each chunk
    inner:     Allocator;      // Where the chunks come from, the heap when it is null
    freeList:  *FreeBlock;     // The blocks that were given back
    chunks:    *Chunk;         // The chunks, newest first
    inUse:     i64;

    allocate :: method * (size: i64, align: i64) -> *void {
        if size > self.blockSize || align > 8 { panic("A pool was asked for a block bigger than its blockSize"); }
        self.inUse += 1;
        if self.freeList != null {                       // a block that was given back
            block := self.freeList;
            self.freeList = block.next;
            c.memset(block, 0, self.blockSize as int);
            return block;
        }
        ...                                              // else the next block of the newest chunk, or a new chunk
    }

    // A block can't grow: it is blockSize bytes
    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void {
        if newSize > self.blockSize { panic("A block of a pool can't grow over the blockSize of the pool"); }
        return memory;
    }

    deallocate :: method * (memory: *void, size: i64, align: i64) {
        block := memory as *FreeBlock;                   // the block holds the next free one
        block.next = self.freeList;
        self.freeList = block;
        self.inUse -= 1;
    }

    free :: method * () { ... }                          // gives back all the chunks to inner
}
```

An allocator that can't change a block where it is can write its `resize` with `resizeByCopy(self, memory, oldSize,
newSize, align)` of `allocators`, which makes a new block with `allocate`, copies what fits, and gives the old one back
with `deallocate`. A wrapper calls another allocator, e.g one that counts:

```odin
Counting :: struct {
    inner: Allocator;
    bytes: i64;

    allocate :: method * (size: i64, align: i64) -> *void {
        self.bytes += size;
        return resizeMemory(self.inner, null, 0, size, align);
    }
    resize :: method * (memory: *void, oldSize: i64, newSize: i64, align: i64) -> *void {
        self.bytes += newSize - oldSize;
        return resizeMemory(self.inner, memory, oldSize, newSize, align);
    }
    deallocate :: method * (memory: *void, size: i64, align: i64) {
        self.bytes -= size;
        resizeMemory(self.inner, memory, size, 0, align);
    }
}

counting := Counting{inner: mem.heap()};
names.allocator = &counting;
```

## Not done yet

* `mem.setHeap`, to make another allocator the heap of the whole program.
* The chunks of `allocators.Arena` come from C, not from an allocator it is given.
* `toCString` and `c.string.free()` use C's `free` for what the heap gave.
* A check that an allocator works with the standard library, e.g `mem.testAllocator`.
* A scratch arena of the thread with marks.
