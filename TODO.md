Todo list:
-------------------

* Function call overhaul:
  - Multiple return values
  - Variadic arguments
  - Default arguments
  - Named arguments

* Better string handling
  - Iterating over strings
  - Indexing into them

* Better struct (default values and constants)

* Better enums
  - Get the names of enums
  - Backing integer types for enums

* Import system
  - Modules / Packages n' stuff


Bug fixes:
-------------------
- Fix hash_table when collisions happen. Currently we crash if defining two structs with the same name !!!!
- Assigning to functions with void type should not be allowed
- @Investigate - examples/for_loops crashes once in a while. Is it a hisenbug?


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
