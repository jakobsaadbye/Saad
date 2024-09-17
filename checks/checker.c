#include "dirent.h"
#include "../src/lib/file.c"
#include "../src/pipeline.c"

void check_entire_directory(const char *dir_name, bool failing_is_success) {
    DIR *dir = opendir(dir_name);
    if (dir == NULL) {
        printf("error: Failed to open directory '%s'", dir_name);
        exit(-1);
    }
    
    int succeeded = 0;
    int failed = 0;
    int num_tests = 0;
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
            char file_path[512];
            sprintf(file_path, "%s/%s", dir_name, filename);

            printf("Run %s\t\t", file_path);
            
            bool ok = false;
            char *program = read_entire_file(file_path);
            bool compiled = send_through_pipeline(program, file_path, false);

            if (failing_is_success)
                ok = !compiled;
            else
                ok = compiled;

            if (ok) {
                printf("OK\n");
                succeeded += 1;
            } else {
                printf("FAILED\n");
                failed += 1;
            }

            free(program);
            num_tests += 1;
        }
    }

    printf("\nRan %d tests, %d OK, %d FAILED\n", num_tests, succeeded, failed);
    printf("------------------------------------\n");
}

int main() {
    check_entire_directory("examples", false);
    check_entire_directory("checks", false);
    check_entire_directory("checks/should_fail", true);

    return 0;
}


