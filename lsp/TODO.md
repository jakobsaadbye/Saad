## Bugs and improvements:

[]: Navigating to fields and hovering fields on "self" inside struct in struct methods
[]: Syntax highlighting gets marked as string after a " inside a character literal. This happens in string_builder.sd where
    sb.addChar('"'); starts messing with syntax highlighting
  
[]: Inferred struct literals are not getting auto-complete on the members
    For example: people: []Person = [{age: 18, "Alex"}, {age: 28, "Katja"}, {age: 25, name: "Jakob"}, {THIS IS NOT BEING AUTO-COMPLETED}];

[]: I want that the standard library is indexed as 1 unit such that navigating through methods shows uses in the entire standard library and not just for its own namespace

[]: (improvement): Add rename. The usual f2 that renames methods / functions safely. For example doing a rename of a function in the standard library will the name of the function at all call sites in the standard library. User programs will still need to rename themselves. Maybe something could be done on a language level to make the transition to a new function name easier???

[]: (improvement): Prompt to fill-out all missing cases in a switch statement.

