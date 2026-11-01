// Assembles a .asm file of the Saad compiler with its own assembler (src/x64_asm.c), for testing it against nasm.
//
//     asm_driver coff in.asm out.obj
//     asm_driver elf  in.asm out.o [prefix]
//     asm_driver pe   in.asm out.exe
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "x64_asm.c"
#include "object_writer.c"

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: asm_driver coff|elf in.asm out [prefix]\n"); return 2; }
    FILE *f = fopen(argv[2], "rb");
    if (!f) { perror(argv[2]); return 2; }
    fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
    char *text = malloc(size + 1); fread(text, 1, size, f); text[size] = 0; fclose(f);
    AsmObject obj;
    if (!x64_assemble(text, (int)size, &obj)) { fprintf(stderr, "%s: %s\n", argv[2], obj.error); return 1; }
    char error[256] = "";
    bool ok;
    if (strcmp(argv[1], "coff") == 0)     ok = coff_write(&obj, argv[3]);
    else if (strcmp(argv[1], "pe") == 0)  ok = pe_write(&obj, argv[3], error, sizeof(error));
    else                                  ok = elf_write(&obj, argv[3], argc > 4 ? argv[4] : "");
    if (!ok) fprintf(stderr, "%s\n", error);
    return ok ? 0 : 1;
}
