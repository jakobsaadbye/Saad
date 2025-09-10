- Fix hash_table when collisions happen. Currently we crash if defining two structs with the same name !!!!


TODO:
  - Passing arrays to functions (Can we make it interopable with C without any modifications?)



Later:
-------------
- Do a fast-path for struct initialization where the expression tree is only 1-level deep to prevent too many asm instructions
- Asignign to functions with void type should not be allowed!!!!!!!!
- If, For and While statements should accept a statement instead of a block as their body to do inline style versions

Optimize this
mov		rax, 1
push		rax
mov		rax, 2
push		rax
mov		rax, 3
push		rax
mov		rax, 4
push		rax
lea		rax, -4[rbp]


- Function calls
   - Variadic functions
   - Default arguments
   - Named arguments
- Imports / Modules / Packages n' stuff

Current TODO:
    - :FloatRefactor

    - WrongForLoopSizing (remove temporary hack of aligning functions to 32 bytes just to fix for-loops wrong size!)
    - @Investigate - examples/for_loops crashes once in a while. Is it a hisenbug?

    - Backing integer types for enums
    - Error messages with multiple color highlighting. (For cases where the red underline can be confusing. E.g when doing pointer dereference)

Roadmap:
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
