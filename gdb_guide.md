# Debugging Saad programs

Build the program with debug information: `saad build main.sd --debug` (with the x64 backend, or with `--llvm`). It
has DWARF debug information, which LLDB and GDB read: the lines of the program, its functions and their variables.

## LLDB

```lldb
lldb build/out.exe
command script import tools/lldb/saad_formatters.py     # Shows strings, lists, optionals, unions and any like Saad
breakpoint set -f main.sd -l 12
run
frame variable          # The variables
bt                      # The calls
next                    # The next line
step                    # Into a call
```

## GDB

```gdb
gdb build/out.exe
break main.sd:12
run
info locals
bt
next
```

# Some useful commands

si: Skip instruction
s:  Skip until out of function
ni: Next instruction
