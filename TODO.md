TODO:
    - Backing integer types for enums
    - Allow enum values to be assigned to constant integer expressions
    - Better printf

Roadmap:
    - Constants
    - Pointer types
    - Array types
    - Heap allocation + Allocators
    - Structs as function parameters
    - Switch statement
    - Bit operations
    - Foreign functions + C bindings
    - Modules / Packages



Fix semi-colon being off by one.

    main :: () {
        y : Vector3 = .{x = 1.0, y = 2.0, z = 3.0}
        printXXX(x.scale + cube.xy.y);
    }
    .\test.sd:18:10 error: Expected a ';'
    18 |     print(x.scale + cube.xy.y);


