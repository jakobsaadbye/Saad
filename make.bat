@echo off

:: Set compiler flags
set CFLAGS=-Wall -Wextra -pedantic -ggdb

:: List of source files (space-separated)
set FILES=ast lexer parser scope lib/string_builder lib/dynamic_array lib/table lib/arena

:: Compile each source file
for %%f in (%FILES%) do (
    gcc -c src/%%f.c -o build/%%f.o
)

:: Link all object files into one executable
gcc %CFLAGS% -o ./bin/saad.exe ^
    build/ast.o ^
    build/lexer.o ^
    build/parser.o ^
    build/scope.o ^
    build/lib/string_builder.o ^
    build/lib/dynamic_array.o ^
    build/lib/table.o ^
    build/lib/arena.o ^
    src/main.c