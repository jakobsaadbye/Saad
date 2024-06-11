#include "dirent.h"
#include "../src/lib/file.c"
#include "../src/pipeline.c"

void check_entire_directory(const char *dir_name) {
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        printf("error: Failed to open directory '%s'", dir_name);
        exit(-1);
    }

    struct dirent *file;
    while (true) {
        file = readdir(dir);
        if (file == NULL) break;

        char *filename = file->d_name;
        char *c = &filename[file->d_namlen - 1];
        while (*c != '.') {
            c--;
        }
        char *extension = c + 1;

        if (strcmp(extension, "sd") == 0) {
            char file_path[256];
            sprintf(file_path, "%s/%s", dir_name, filename);

            printf("Compiling %s\n", file_path);
            
            char *program = read_entire_file(file_path);
            bool success  = send_through_pipeline(program, file_path, false);

            assert(success);

            printf("   OK\n");

            free(program);
        }
    }
}

int main() {
    check_entire_directory("examples");
    check_entire_directory("checks");

    return 0;
}


