# Small helper file on how to use GDB (as its really arcane)

# To start debugging the output.exe file

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