#define DEBUG

#include "pipeline.c"

int main(int argc, char const *argv[]) {
    if (argc < 2) { 
        printf("Error: Missing input file\n\n");
        printf("Usage:\n");
        printf("   saad.exe <input_file.sd>\n");
        return 1;
    }

    const char *main_file_path = argv[1];

    bool success = compile_program(main_file_path, true);
    if (success) {
        printf("\n\nCompiled successfully.\n\n");
    } else {
        printf("There were errors.\n\n");
    }

    return 0;
}
