# Saad Compiler

A just for fun compiler

# Changelog

## Update v0.2.5 - 25. Mar 2026

* Structs can not have constant members that live on the struct instance

```odin
Color :: struct {
    r, g, b, a: u8;

    Red   :: Color{255, 0, 0, 255};
    Green :: Color{0, 255, 0, 255};
    Blue  :: Color{0, 0, 255, 255};
}

main :: () {
    Print("%", Color.Red);
    Print("%", Color.Green);
    Print("%", Color.Blue);
}
```

* Structs can now be defined in any order

Before, structs had to be declared before any using structs like in C.
Now there is no order to how structs can be defined

* Bug fixes

** Fixed structs being allowed to have recursive definitions

```odin
Foo :: struct {
    x: Foo; // <-- This is not allowed
}
```

** Fixed and correctly detect cycles in struct sizing
```odin
A :: struct {
    b: B;
}

B :: struct {
    a: A; // <-- This is not allowed, since it would do a cycle back to typechecking A
}

```

## Update v0.2.4 - 16. Mar 2026

* BREAKING: New syntax for casting

The keyword 'cast' is now replaced with the suffix 'as' keyword, like in Rust

```odin
myString := "New cast syntax" as *u8;

memory := malloc(1024) as *u8;

mySmallNumbers := [1 as u8, 2, 3, 4, 5];
```

Auto-casting with 'xx' is left unchanged.

The 'as' way of casting is often more natural to write as you append to the right of your expression when you need a cast,
opposed to cast(type) being prefix and requiring paranthesis.

## Update v0.2.3 - 13. Mar 2026

* Structs can now appear as constants

```odin
MyColor :: Color{255, 127, 0, 255};
```

### Minor fixes

* Fixed wrong casting behavior for integers. Now works correctly with the new fmt.Print
* Fixed wrong struct sizing leading to weird behavior

## Update v0.2.2 - 07. Mar 2026

### Better enums

Enums now have the members:

* ```count```: To get the number of enum values
* ```names```: To get the string representation of the enum values
* ```values```: To get an array of all the enum values of the enum

```odin
Fruit :: enum {
    mango,
    kiwi,
    watermelon,
    dragonfruit,
    pear,
    apple,
}

main :: () {

    print("%\n", Fruit.count);

    for name in Fruit.names {
        print("%\n", name);
    }

    for value in Fruit.values {
        print("%\n", cast(int) value);
    }

}
```

### Minor fixes
* Fix ```append``` not working with literals
* Fix lots of issues in runtime.sd after strings are now 16 bytes slices

## Update v0.2.1 - 18. Feb 2026

* Introduction of default parameters and positional arguments for functions

Function parameters can now have default values

```odin
Myprint :: (format: string, args: ...any, newline := true) {
    Print(format, ...args);
    if newline {
        Print("\n");
    }
}

main :: () {
    Myprint("Default parameters are %", "useful during refactoring or providing an extensible api", newline = false);
}


```

## Update v0.2 - 1. Feb 2026

* Introduction of runtime type reflection. 

Its now possible to inspect the type of variables at runtime using the new ```std/reflect.sd``` package

See ```tests/runtime_reflection.sd``` as an example of how to use it

* Introduced variadic function parameters

Its now possible to declare a function parameter as being variadic and passing variadic arguments to functions just like in  
any other language.

Example:
```odin
printArguments :: (args: ...any) {
    for arg, i in args {
        print("Arg % = %", i, arg);
    }
}

main :: () {
    printArguments("Hello! My name is Charles", 420);
}
```

* External C functions now has to be declared inside an extern block instead of being at the end of each function definition

Old syntax:
```odin
printf  :: (format: string, args: ...any) -> int #extern "C"
sprintf :: (buffer: *u8, format: string, args: ...any) -> int #extern "C"
```

New syntax:
```odin
extern "C" {
    printf  :: (format: string, args: ...any) -> int;
    sprintf :: (buffer: *u8, format: string, args: ...any) -> int;
}
```