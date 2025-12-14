#define DEBUG

#include "compiler.c"

int main(int argc, char const *argv[]) {
    if (argc < 2) { 
        printf("Error: Missing input file\n\n");
        printf("Usage:\n");
        printf("   saad.exe <input_file.sd>\n");
        return 1;
    }

    const char *main_file_path = argv[1];

    CompilerConfig config = get_standard_compiler_config();

    bool success = compile_program(&config, main_file_path, true);
    if (success) {
        printf("\n\nCompiled successfully.\n\n");
    } else {
        printf("There were errors.\n\n");
    }

    return 0;
}
