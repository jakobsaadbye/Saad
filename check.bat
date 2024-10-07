@echo off

:: Set compiler flags
set CFLAGS=-Wall -Wextra -pedantic -ggdb

:: List of source files (space-separated)
set FILES=ast lexer parser scope

:: Compile each source file
for %%f in (%FILES%) do (
    gcc -c src/%%f.c -o build/%%f.o
)

:: Link all object files into one executable
gcc %CFLAGS% -o ./build/checker.exe ^
    build/ast.o ^
    build/lexer.o ^
    build/parser.o ^
    build/scope.o ^
    checks/checker.c

:: Run
.\build\checker.exe