#include "dirent.h"
#include "../src/pipeline.c"

#define TEST_OUTPUT_LEN 60

void check_entire_directory(const char *dir_name, bool should_fail) {
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

        if (strcmp(extension, "sd") != 0) continue;

        char file_path[512];
        sprintf(file_path, "%s/%s", dir_name, filename);

        char pad_str[TEST_OUTPUT_LEN];
        memset(pad_str, 0, TEST_OUTPUT_LEN);

        int pad_len = TEST_OUTPUT_LEN - strlen(file_path);
        for (int i = 0; i < pad_len; i++) {
            pad_str[i] = '.';
        }

        printf("Run %s %s ", file_path, pad_str);
        
        bool compiled = compile_program(file_path, false);
        
        bool ok = false;
        if (should_fail)
            ok = !compiled;
        else
            ok = compiled;

        if (ok) {
            printf(COLOR_GREEN"OK\n"COLOR_RESET);
            succeeded += 1;
        } else {
            printf(COLOR_RED"FAILED\n"COLOR_RESET);
            failed += 1;
        }

        num_tests += 1;
    }

    // printf("\nRan %d tests, %d OK, %d FAILED\n", num_tests, succeeded, failed);
    // printf("------------------------------------\n");
}

int main() {
    check_entire_directory("examples", false);
    check_entire_directory("checks", false);
    check_entire_directory("checks/should_fail", true);

    return 0;
}
