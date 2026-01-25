set ASM_FILE=./build/out.asm

nasm -fwin64 -g %ASM_FILE% -o ./build/out.obj

gcc -o ./build/out.exe ./build/out.obj -LC:/Saad/packages/raylib/lib -LC:/Saad/packages/runtime/lib -lruntime_support -lraylib -lkernel32 -lmsvcrt -lgdi32 -lwinmm -lopengl32 -ld3d9 -ldxguid