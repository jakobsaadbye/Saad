## Bugs and improvements:

[x]: Navigating to fields and hovering fields on "self" inside struct in struct methods
     The self of a method written in a struct is in the index (with an empty name at 'method'), so it has a hover
     ("self: *Circle") and goes to its method. Namespaced types like time.Time are now a use of the namespace and of
     the type, each colored by itself.
[x]: Syntax highlighting gets marked as string after a " inside a character literal. This happens in string_builder.sd where
    sb.addChar('"'); starts messing with syntax highlighting
     The grammar handles characters since a959411; checked with vscode-textmate on std/json/encode.sd. The extension
     is now 0.2.0, so build and install it again (see README.md) to get the new grammar.

[x]: Inferred struct literals are not getting auto-complete on the members
    For example: people: []Person = [{age: 18, "Alex"}, {age: 28, "Katja"}, {age: 25, name: "Jakob"}, {THIS IS NOT BEING AUTO-COMPLETED}];

[x]: I want that the standard library is indexed as 1 unit such that navigating through methods shows uses in the entire standard library and not just for its own namespace
     'saad index --all-std', and one project for the files of std/

[x]: (improvement): Add rename. The usual f2 that renames methods / functions safely. For example doing a rename of a function in the standard library will the name of the function at all call sites in the standard library. User programs will still need to rename themselves. Maybe something could be done on a language level to make the transition to a new function name easier???
     Done for everything but namespaces. A name of the standard library is renamed from a file of std/.
     The language level part is still open. Today the old name can stay as a @deprecated function that calls the new
     one, like addAnyValue in std/fmt/string_builder.sd, which warns at each use. A quick fix on that warning that
     renames the use would make the move easy for programs.

[x]: (improvement): Prompt to fill-out all missing cases in a switch statement.
     A quick fix on the error "The switch doesn't handle ..."

[x]: Robustness: losing the language server's features while typing
     * A missing import no longer empties the index (the whole program was gone, so nothing worked in any file)
     * A file keeps the program it is part of while an error leaves it out, instead of becoming a program of its own
     * When an error drops files from the index, the names of the last index are kept
     * Completion works in lines that aren't finished, e.g 'xs.filter(x => x.' with no ')'

[x]: Expression trees: completion, hover and go to definition in the lambda of an Expr<F>
