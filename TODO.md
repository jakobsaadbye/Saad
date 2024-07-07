Fix semi-colon being off by one.

    main :: () {
        y : Vector3 = .{x = 1.0, y = 2.0, z = 3.0}
        printXXX(x.scale + cube.xy.y);
    }
    .\test.sd:18:10 error: Expected a ';'
    18 |     print(x.scale + cube.xy.y);