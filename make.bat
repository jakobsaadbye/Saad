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
::   With the LLVM backend (--llvm) when LLVM is installed with its C headers and LLVM-C.lib, e.g in
::   C:\Program Files\LLVM. Set LLVM_PATH for another folder
if "%LLVM_PATH%"=="" set "LLVM_PATH=C:\Program Files\LLVM"
set LLVM_FLAGS=
if exist "%LLVM_PATH%\include\llvm-c\Core.h" if exist "%LLVM_PATH%\lib\LLVM-C.lib" set LLVM_FLAGS=-DSAAD_LLVM -isystem "%LLVM_PATH%\include" "%LLVM_PATH%\lib\LLVM-C.lib"

gcc %CFLAGS% -o ./bin/saad.exe ^
    build/ast.o ^
    build/lexer.o ^
    build/parser.o ^
    build/scope.o ^
    src/main.c %LLVM_FLAGS%