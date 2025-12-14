#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

char *read_entire_file(const char *file_path);
bool check_file_exists(const char *file_path);
void change_directory(const char *dir);
char *get_current_directory();
void make_directory(const char *dir);

#endif

#ifdef FILE_IMPLEMENTATION

char *read_entire_file(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        printf("error: Failed to open file '%s'", file_path);
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
        printf("error: Not enough memory for allocation of the file");
        fclose(f);
        exit(1);
    }
    memset(buffer, '\0', file_size + 1);
    fread(buffer, sizeof(char), file_size, f);
    
    fclose(f);

    return buffer;
}

bool check_file_exists(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        return false;
    }
    fclose(f);
    return true;
}

void change_directory(const char *dir) {
    if (_chdir(dir) != 0) {
        printf("%s:%d: error: Failed to change directory to '%s'\n", __FILE__, __LINE__, dir);
        exit(1);
    }
}

char *get_current_directory() {
    char *buffer = _getcwd(NULL, 0);
    if (buffer == NULL) {
        printf("%s:%d: error: Failed to get current directory\n", __FILE__, __LINE__);
        exit(1);
    }
    return buffer;
}

void make_directory(const char *dir) {
    if (_mkdir(dir) == -1) {
        if (errno == EEXIST) {
            // Already exist
            return;
        } else {
            printf("%s:%d: error: Failed to make directory '%s'\n", __FILE__, __LINE__, dir);
            exit(1);
        }
    }
}

#endif