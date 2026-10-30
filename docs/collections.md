# Collections

`List<T>` and `Map<K, V>` live in `std/collections.sd` and are written in Saad. The compiler has no
special knowledge of them. They get their syntax from a small **collection protocol**: methods with
certain names that any struct can declare. A library author could have written them, and can write
other collections that feel just as built in.

Every file imports `std/collections.sd` automatically. A file's own declarations win over it, so a
file can still declare its own `List` or `Map`. `[..]T` always means the standard `List<T>`.

## The collection protocol

| Syntax | Means | Method to declare |
| --- | --- | --- |
| `c[k]` (read) | `c.index(k)`, or `*c.indexRef(k)` if there is no `index` | `index :: method (key: K) -> V` |
| `c[k] = v`, `c[k] += v`, `&c[k]`, `c[k].x = v`, `c[k].mutatingMethod()` | `*c.indexRef(k)` | `indexRef :: method (key: K) -> *V` |
| `for x in c`, `for x, i in c`, `for *x in c` | loops over `c.elements()` | `elements :: method () -> []T` |
| `for key, value in c` | loops over `c.elements()`, taking the `key` and `value` members of each element | `elements` and `set`, with elements that have `key` and `value` members |
| `c` where a `[]T` is wanted, `print`, `any` | `c.elements()` | `elements` |
| `[a, b, c]` where a collection is wanted | `tmp.reserve(3); tmp.add(a); ...` | `add :: method * (value: T)`, `reserve` is optional |
| `[k1: v1, k2: v2]` | `tmp.reserve(2); tmp.set(k1, v1); ...` | `set :: method * (key: K, value: V)` |

Without a wanted type, `[k: v, ...]` is a standard `Map<K, V>` with the types of the first entry.
`[:]` is the empty map. It needs a wanted type, e.g. `m: Map<string, int> = [:];`.
`[a, b, c]` without a wanted type is still a fixed size array.

The typer rewrites these into ordinary method calls (`AstExpr.replacement`), so the backend only sees
calls. A collection literal is built in a hidden local (`AstArrayLiteral.collection_decl`).

```odin
// A complete collection: indexable, iterable, buildable from a literal and passable as a slice
Stack :: struct <T> {
    data:     *T;
    count:    i64;
    capacity: i64;

    add      :: method * (value: T) { ... }
    indexRef :: method (i: i64) -> *T { assert(i >= 0 && i < self.count); return &self.data[i]; }
    elements :: method () -> []T { ... }
}

s: Stack<int> = [1, 2, 3];
s[0] += 10;
for x in s { ... }
```

## `List<T>` (`[..]T`)

```odin
xs: [..]int = [1, 2, 3];
xs.add(4);
xs.insertAt(0, 0);
first := xs.removeAt(0);
last  := xs.removeLast();
for x in xs { ... }
total := sum(xs);   // sum :: (values: []int) -> int
```

| Method | |
| --- | --- |
| `add(value)` | Adds at the end |
| `insertAt(index, value)` | Inserts, moving the later values up |
| `removeAt(index) -> T` | Removes and returns, keeping the order |
| `swapRemoveAt(index) -> T` | Removes by moving the last value into the hole. O(1), changes the order |
| `removeLast() -> T` | Removes and returns the last value |
| `first() -> T`, `last() -> T` | Assert that the list is not empty |
| `tryFirst() -> (T, bool)`, `tryLast() -> (T, bool)` | For lists that might be empty |
| `isEmpty()`, `indexOf(value) -> i64` (-1 if missing), `contains(value)` | |
| `reverse()`, `copy() -> List<T>` | |
| `reserve(capacity)`, `clear()` (keeps the memory), `free()` | |

Members: `data: *T`, `count: i64`, `capacity: i64`, `allocator: *Allocator`. The capacity starts at
4 and doubles.

## `Map<K, V>`

```odin
ages := ["alice": 31, "bob": 27];
ages["carol"] = 40;        // Adds or changes
a := ages["alice"];        // Asserts that the key is there
b := ages.get("dave", 0);  // With a fallback
c, ok := ages.tryGet("dave");
counts[word] += 1;         // A missing key starts at the zero value
for name, age in ages { print("% is %\n", name, age); }
for *entry in ages { entry.value += 1; }   // To change the values
```

In `for key, value in m` the key and the value are copies. On a map the second name is the value, not the
index. A list keeps the index (`for x, i in xs`), as it has no `set` method.

| Method | |
| --- | --- |
| `set(key, value)` | Adds or changes |
| `get(key, fallback) -> V`, `tryGet(key) -> (V, bool)`, `has(key)` | |
| `remove(key) -> bool` | Keeps the order of the other entries. O(n) |
| `keys() -> [..]K`, `values() -> [..]V` | In insertion order |
| `isEmpty()`, `reserve(count)`, `clear()`, `free()` | |

How it works: `entries` is a dense `[..]Entry<K, V>` in insertion order, and `slots` is an open
addressing table (linear probing, a power of two in size, at most 70% full) holding `entry index + 1`,
with 0 for an empty slot. Iteration is a loop over `entries`. Growing rebuilds only `slots`, with the
hashes cached in the entries. Removing shifts `entries` down and rebuilds `slots`.

## Hashing

A key type needs `hash :: method (key: K) -> u64` and `==`. `std/collections.sd` gives a hash to all
the integer types, `f32`, `f64`, `bool` and `string` (FNV-1a), and has `hashU64` and `hashCombine`
for your own types. Structs compare with `==` member by member:

```odin
Point :: struct {
    x, y: int;
    hash :: method () -> u64 { return hashCombine(self.x.hash(), self.y.hash()); }
}

Suit :: enum { Hearts, Spades }
hash :: method (s: Suit) -> u64 { return (s as i64).hash(); }
```

A key type without a `hash` method is reported where the map is used, e.g.
`Type Color has no method 'hash'`.

## Allocators

`List` and `Map` get their memory through `allocator: *Allocator`, or the heap when it is null:

```odin
Allocator :: struct {
    context: *void;
    resize:  (context: *void, memory: *void, size: i64) -> *void; // Like realloc. A size of 0 frees
}
```

There are no destructors, so call `free()` when a collection is no longer needed.

## Not supported yet

* `map` and `filter` style methods, which need methods with type parameters of their own.
* Hashing for pointers and enums without writing a `hash` method.
* `remove` on a map is O(n), as it keeps the order of the entries.
