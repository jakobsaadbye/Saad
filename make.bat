@echo off

:: Set compiler flags
set CFLAGS=-Wall -Wextra -pedantic -ggdb

:: List of source files (space-separated)
set FILES=ast lexer parser scope

:: Compile each source file
for %%f in (%FILES%) do (
    :: gcc -c src/%%f.c -o build/%%f.o
    gcc %CFLAGS% -c src/%%f.c -o build/%%f.o
)

:: Link all object files into one executable
gcc %CFLAGS% -o ./bin/saad.exe ^
    build/ast.o ^
    build/lexer.o ^
    build/parser.o ^
    build/scope.o ^
    src/main.c