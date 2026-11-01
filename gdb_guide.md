# Small helper file on how to use GDB (as its really arcane)

# To start debugging the output.exe file

Build the program with debug information first: `saad build main.sd --debug` (needs nasm and gcc). The normal build
makes out.exe without it.

```gdb
gdb .\build\out.exe
b main
run
layout asm
```

# Some useful commands

si: Skip instruction
s:  Skip until out of function
ni: Next instruction