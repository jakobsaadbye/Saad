Current TODO:
    - :WrongForLoopSizing (remove temporary hack of aligning functions to 32 bytes just to fix for-loops wrong size!)
    - Make Array be a struct where you can do array.count or array.data

    - Backing integer types for enums
    - Better scoping (structs should have their own scope, same with enums)
    - Error messages with multiple color highlighting. (For cases where the red underline can be confusing. E.g when doing pointer dereference)
    - Unary dot for enum access

Roadmap:
    - Revisit of for-loops (DONE)
    - Pointer types (Mostly done, void pointer autocast is missing)
    - Array types (Mostly done, multidimensional arrays + heap allocated is still missing)
    - Constants (Mostly done. Missing as struct member)
    - Switch statement
    - Type casts
    - Heap allocation + Allocators
    - Structs as function parameters
    - Struct default values
    - Bit operations
    - Compiling to LLVM
    - Foreign functions / C bindings
    - Modules / Packages
    - Own ARM64 backend
