#include <stdio.h>
#include <stdlib.h>

#define MAX_CMD_INPUT_SIZE 1024

char *read_entire_file(const char *file_path);

char *read_entire_file(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        printf("error: Failed to open %s", file_path);
        exit(1);
    }

    int failed = fseek(f, 0L, SEEK_END);
    if (failed) {
        printf("error: Failed to determine size of %s", file_path);
        exit(1);
        fclose(f);
    }
    int file_size = ftell(f);
    rewind(f);

    char *buffer = (char *)(malloc(file_size + 1));
    if (buffer == NULL) {
        printf("error: Buy more ram, LUL");
        fclose(f);
        exit(1);
    }
    memset(buffer, '\0', file_size + 1);
    fread(buffer, sizeof(char), file_size, f);
    
    fclose(f);

    return buffer;
}