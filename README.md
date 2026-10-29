# Saad Compiler

A just for fun compiler

# Changelog

## Update v0.3.1 - 29. Oct 2026

* BREAKING: The signed integer types are renamed from `s8`, `s16`, `s32`, `s64` to `i8`, `i16`, `i32`, `i64`

```odin
a: i32 = -5;
b := 100 as i64;
```

`int` is still an alias for `i32`.

* Type aliases

A constant declaration whose value is a type declares a type alias. It works for every kind of type,
including instances of generic structs:

```odin
Byte     :: u8;
IntPtr   :: *int;
Ints     :: []int;
IntList  :: List<int>;
Hue      :: Color;
Callback :: (x: int) -> int;

main :: () {
    b: Byte = 255;
    numbers := IntList{};
    numbers.push(1);    // Methods of List<int> work through the alias
    h: Hue = .Red;
}
```

An alias is the same type as the type it names, so an `IntList` can be passed where a `List<int>` is wanted,
and `typeof` gives the name of the aliased type. Aliases can be used before they are declared.

## Update v0.3.0 - 29. Oct 2026

* Generic functions

Functions can take type parameters, written in angle brackets after `::`. The types are inferred from the
arguments of each call, and each distinct set of types gets its own copy of the function.

```odin
max :: <T>(elements: ...T) -> T {
    best := elements[0];
    for e in elements {
        if e > best { best = e; }
    }
    return best;
}

main :: () {
    a := max(3, 9, 4);   // T = int
    b := max(1, 2.5);    // T = float, a float literal wins over an integer literal
    x: i64 = 1;
    c := max(x, 10);     // T = i64, literals take the type of a typed argument
}
```

Typed arguments must agree: `max(a, b)` with an `i32` and an `i64` is an error, nothing is widened.
Every type parameter has to be used by a parameter, as there is no syntax to give the types explicitly yet.

* Methods on generic structs

Methods declared outside a struct declare the type parameters of the receiver:

```odin
set :: method <K, V>(m: *Map<K, V>, key: K, value: V) {
    append(m.keys, key);
    append(m.values, value);
}
```

The receiver must use each type parameter exactly once, e.g `Map<K, V>`. `Map<string, V>` is an error, as methods
cannot be specialized for some types.

* Methods declared inside structs

Methods can be declared inside a struct, where they get an implicit `self`. `method` gives a read-only `self`,
and `method *` a `self` that can be changed:

```odin
Map :: struct <K, V> {
    keys:   [..]K;
    values: [..]V;

    get :: method (key: K) -> V {
        for i in 0..self.keys.count {
            if self.keys[i] == key { return self.values[i]; }
        }
        empty: V;
        return empty;
    }

    set :: method * (key: K, value: V) {
        append(self.keys, key);
        append(self.values, value);
    }
}
```

Members are only reached through `self`, e.g `self.keys`. Methods don't change the size or members of the struct.

In a `method`, nothing in `self` can be changed: assigning to `self.count`, taking `&self.pos`, calling a
`method *` on `self` and `for *` over a fixed size array member are errors. Copy it into a local first if you
want to change a copy (`copy := self.pos`). Read-only is shallow: what the members point to can still be changed,
e.g the elements of a dynamic array member.

* Methods can now be called before they are declared, and on values that are not variables, e.g `make_map().set(k, v)`
  (the value is put in a temporary, so the change is lost)

Bug fixes:

* Fixed untyped literals passed as varargs not taking the type of the variadic parameter e.g `f(a, 100)` with `f :: (xs: ...i64)`
* Fixed a struct declared after a variable of its type not having its members resolved

## Update v0.2.9 - 29. Oct 2026

* Generic structs

Structs can now take type parameters, written in angle brackets after `struct`

```odin
List :: struct <T> {
    elements: [..]T;
    count: int;
}

Pair :: struct <A, B> {
    first:  A;
    second: B;
}

main :: () {
    numbers: List<int>;
    append(numbers.elements, 1);

    pair  := Pair<int, float>{1, 2.5};
    pairs := Pair<List<int>, *Pair<int, int>>{};
}
```

Every distinct set of type arguments creates its own struct (monomorphization), so `List<int>` and `List<float>` are two
separate types with their own sizes. Writing `List<int>` in two places refers to the same type.

Not supported yet: methods on generic structs (methods on a specific instance like `*List<int>` work), constants
inside generic structs, function types as members of generic structs and type aliases like `MyList :: List<int>`

Bug fixes:

* Fixed keywords directly followed by an operator e.g `int>` being lexed as identifiers

## Update v0.2.8 - 30. May 2026

Big changes to functions to make them first-class citizens

Function definitions can now appear as normal expressions to create annonymous functions.

For example:
```odin
MyButton :: struct {
    onClick: () -> void;
}

main :: () {
    // Here we assign a function to the onClick handler
    btn := MyButton{
        onClick = () {
            Print("You clicked!");
        }
    };

    btn.onClick(); // Prints You clicked!

    // Here we define a fixed array of functions
    myFuncs := [
        () { Print("0"); },
        () { Print("1"); },
        () { Print("2"); },
    ];

    for func in myFuncs {
        func(); // 0, 1, 2
    }
}
```

Bug fixes:

* Fixed assignment to pointer that would dereference the pointer on assigment making normal pointer assignment impossible
* Fixed Print not producing newlines correctly because of default arguments being wrong

## Update v0.2.7 - XX. Mar 2026

* Fixed break and continue in for-loops or while-loops not correctly being attached to the correct loop
* Fixed name collision when having two constants with the same name in different scopes
* Fixed crash due to .data array member not being resolved
* Fixed wrong precedence with casting using 'as'
* Fixed constant identifiers not transforming to float
* Fixed `append` being broken when appending structs in a variable

## Update v0.2.6 - 26. Mar 2026

* Constants within structs can now refer to other constants within the same struct

```odin
Color :: struct {
    r, g, b, a: u8;

    Red   :: Color{255, 0, 0, 255};
    Green :: Color{0, 255, 0, 255};
    Blue  :: Color{0, 0, 255, 255};

    Rainbow :: [Red, Green, Blue];
}
```

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

* Structs can now be defined and used in any order

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