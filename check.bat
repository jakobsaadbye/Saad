:: Build
set CFLAGS=-Wall -Wextra -pedantic -ggdb
gcc ./checks/checker.c -o ./build/checker.exe %CFLAGS%

:: Run
.\build\checker.exe