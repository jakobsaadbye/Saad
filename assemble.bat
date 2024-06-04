set ASM_FILE=./build/out.asm

nasm -fwin64 -g %ASM_FILE% -o ./build/out.obj

gcc -o ./build/out.exe ./build/out.obj -lkernel32 -lmsvcrt