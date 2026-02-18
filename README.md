# Saad Compiler

A just for fun compiler

# Changelog

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