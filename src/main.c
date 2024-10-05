#define DEBUG

#include "pipeline.c"
#include "lib/file.c"

int main(int argc, char const *argv[]) {
    if (argc < 2) { 
        printf("Error: Missing input file\n\n");
        printf("Usage:\n");
        printf("   saad.exe <input_file.sd>\n");
        return 1;
    }

    const char *input_file_path = argv[1];
    char *program = read_entire_file(input_file_path);

    bool success = send_through_pipeline(program, input_file_path, true);
    if (success) {
        printf("\n\nCompiled successfully.\n\n");
    } else {
        printf("There were errors.\n\n");
    }

    free(program);
    return 0;
}
