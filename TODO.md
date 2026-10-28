IR backend (ir.c, ir_lower.c, regalloc.c, x64_emit.c)
-----------------------------------------------

Done:
[x]: Portable IR with own types, single-definition vregs, verifier (ir_verify_module)
[x]: Memory model: locals in stack slots, aggregates by address, copies are memcpy
[x]: Function calls (Saad + C ABI), return values, default arguments, variadics, methods
[x]: Array initialization: fixed arrays, slices, dynamic arrays (+ append)
[x]: Runtime type information (RTTI & any type), typeof, builtin print
[x]: Address-of / Dereference
[x]: While loops, for loops, break / continue
[x]: Short-circuit && and ||
[x]: Linear scan register allocation with spilling, callee-saved registers for values living across calls
[x]: Stack probing for frames >= 4 KB (Windows)

Not supported yet (reported as "Not implemented in the IR backend yet: ..."):
[]: Global variables
[]: C functions returning structs of size 3, 5, 6 or 7 bytes
[]: `return foo()` where foo returns multiple values
[]: Names of enums with a value range above 4096 (print / any)
[]: Runtime type information for function, tuple and variadic types

Next steps:
[]: ARM64 backend: a TargetDesc + arm64_emit.c (the IR and regalloc are target independent)
[]: LLVM backend: IR maps almost 1:1 (vreg -> SSA value, slot -> alloca, ptradd -> getelementptr i8)
[]: Optimizations: promote slots to vregs (mem2reg), constant folding, use immediates and memory operands in x64_emit.c
[]: Compile-time interpreter on top of the IR


[]: Disallow integer and floating point arithmetic
a := 3;
b := 4.5 as f64;
c := a + b;



Functions as values:

[]: Figure out what the hell to do with annonymous methods? Maybe just don't allow them
[]: Functions inside constant arrays
  - e.g functionInArray :: [() -> void {}]


Passing tests:
---------------

All tests in .\tests (including tests\backend_*.sd for the IR backend) pass with the IR backend.




Bug backlog:
----------------

----------------



Future development
-----------------

* Better dynamic arrays
  [] Able to create them with an initial capacity
  [] More methods on them
  [] Able to specify an allocator

* Better structs
  [x] Constant members
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
