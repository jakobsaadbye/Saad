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

:: The LLVM backend (--llvm), when llvm-config of a MinGW build of LLVM is on the PATH, e.g of MSYS2:
::   pacman -S mingw-w64-x86_64-llvm
:: LLVM is linked into saad.exe (its static libraries, only the parts for x64), so saad.exe needs no LLVM to run.
:: Set SAAD_LLVM=0 to build without it
set LLVM_CONFIG=
if not "%SAAD_LLVM%"=="0" where llvm-config >nul 2>nul && set LLVM_CONFIG=llvm-config
if "%LLVM_CONFIG%"=="" goto without_llvm

for /f "delims=" %%i in ('llvm-config --includedir') do set LLVM_INCLUDE=%%i
for /f "delims=" %%i in ('llvm-config --libdir') do set LLVM_LIBDIR=%%i
for /f "delims=" %%i in ('llvm-config --link-static --libs x86codegen x86asmparser passes') do set LLVM_LIBS=%%i
for /f "delims=" %%i in ('llvm-config --link-static --system-libs') do set LLVM_SYSTEM_LIBS=%%i
if not exist "%LLVM_INCLUDE%\llvm-c\Core.h" goto without_llvm

gcc %CFLAGS% -DSAAD_LLVM -isystem "%LLVM_INCLUDE%" -c src/main.c -o build/main.o
g++ -o ./bin/saad.exe build/ast.o build/lexer.o build/parser.o build/scope.o build/main.o ^
    -L"%LLVM_LIBDIR%" %LLVM_LIBS% %LLVM_SYSTEM_LIBS% -static
if errorlevel 1 exit /b 1
echo Built ./bin/saad.exe (with LLVM)
exit /b 0

:without_llvm
gcc %CFLAGS% -o ./bin/saad.exe ^
    build/ast.o ^
    build/lexer.o ^
    build/parser.o ^
    build/scope.o ^
    src/main.c
echo Built ./bin/saad.exe
