Todo list:
-------------------

* Better structs
  [] Constant members
  [] Default values
  [] Defining inner structs
  [] Struct embedding (inheritence)

* Global variables

* Better types
  [] Able to define Structs and Enums locally within scopes


* Functions:
  [] Higher order functions (passing functions as values)
  [] Able to call methods defined later
  [] Methods on any type (including arrays and primitives)

* Import system
  [] Detect circular dependencies
  [] Named imports
  [] Modules / Packages n' stuff

* Better constants / literals
  [] Hex values
  [] Binary values

* Generics
  [] For functions
  [] For structs
  [] Methods on generic types

* For-loops
  [] Able to iterate backwards
  [] Custom iterators

* Syntax
  [] Import for named imports, `use` for loading in all identifiers. Goes together with as keyword to rename import



* Better struct literals
  [] Able to take pointer to struct literal (struct literal should in that case be allocated as a local)

* Better enums
  [x] Get the names of enums
  [] Backing integer types for enums


* Bit operators
  [x] Shift left/right
  [x] Bitwise And
  [x] Bitwise Or




Bug fixes:
-------------------

// @Todo: Escape sequence for strings

* I think it also might make sense to revamp how we use the runtime value of the any type f.x for struct members
  and arrays. The value should NOT be stored on the type it self. Instead we should get that info from the .data on the any.
  Maybe provide some helper functions here

- Fix hash_table when collisions happen. Currently we crash if defining two structs with the same name !!!!
- @Investigate - examples/for_loops crashes once in a while. Is it a hisenbug?

- This should work: We need to allocate the array as a fixed allocation
```odin
for [1, 2, 3, 4, 5, 6, 7, 8, 9, 10] {
    print("%", it);
}
```

- Doing any kind of arithmatic including pointers should default to doing a pointer dereference followed by the normal arithmatic
- E.g the below should just multiply each number by 2 instead of offsetting the pointer address
```odin
xs := [1, 2, 3, 4, 5];
for *x in xs {
    x *= 2;
}
```


Later:
-------------------
- Do a fast-path for struct initialization where the expression tree is only 1-level deep to prevent too many asm instructions
- If, For and While statements should accept a statement instead of a block as their body to do inline style versions
- Error messages with multiple color highlighting. (For cases where the red underline can be confusing. E.g when doing pointer dereference)


Roadmap:
-------------------
- Array types (Mostly done, multidimensional arrays + heap allocated is still missing)
- Constants (Mostly done. Missing as struct member)
- Switch statement
- Heap allocation + Allocators
- Struct default values
- Bit operations
- Compiling to LLVM
- Foreign functions / C bindings
- Modules / Packages
- Own ARM64 backend
