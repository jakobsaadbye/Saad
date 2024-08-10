Roadmap:
    - Pointer types / Array types / Primitive type sizes
    - Structs as function parameters
    - Constant declarations


TODO:
    - Make copy from AstEnum to TypeEnum, likewise with AstStruct
    - Resolve *TypeInfo type names in function call

Fix semi-colon being off by one.

    main :: () {
        y : Vector3 = .{x = 1.0, y = 2.0, z = 3.0}
        printXXX(x.scale + cube.xy.y);
    }
    .\test.sd:18:10 error: Expected a ';'
    18 |     print(x.scale + cube.xy.y);


