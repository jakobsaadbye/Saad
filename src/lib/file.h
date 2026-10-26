#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <direct.h>
    #define PATH_SEPARATOR     '\\'
    #define PATH_SEPARATOR_STR "\\"
    #define NULL_DEVICE        "nul"
#else
    #include <unistd.h>
    #include <limits.h>
    #include <sys/stat.h>
    #define PATH_SEPARATOR     '/'
    #define PATH_SEPARATOR_STR "/"
    #define NULL_DEVICE        "/dev/null"
#endif

char *read_entire_file(const char *file_path);
bool check_file_exists(const char *file_path);
void change_directory(const char *dir);
char *get_current_directory();
void make_directory(const char *dir);
char *get_absolute_path(const char *relative_path);
char *get_directory_from_path(char *path);
char *join_path(const char *a, const char *b);
char *get_executable_path();

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
    size_t read = fread(buffer, sizeof(char), file_size, f);

    // Normalize CRLF line endings to LF. Windows text-mode fopen already does this,
    // but other platforms hand us the raw '\r' characters
    size_t out = 0;
    for (size_t i = 0; i < read; i++) {
        if (buffer[i] != '\r') buffer[out++] = buffer[i];
    }
    buffer[out] = '\0';
    
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
#ifdef _WIN32
    int result = _chdir(dir);
#else
    int result = chdir(dir);
#endif
    if (result != 0) {
        printf("%s:%d: error: Failed to change directory to '%s'\n", __FILE__, __LINE__, dir);
        exit(1);
    }
}

char *get_current_directory() {
#ifdef _WIN32
    char *buffer = _getcwd(NULL, 0);
#else
    char *buffer = getcwd(NULL, 0);
#endif
    if (buffer == NULL) {
        printf("%s:%d: error: Failed to get current directory\n", __FILE__, __LINE__);
        exit(1);
    }
    return buffer;
}

void make_directory(const char *dir) {
#ifdef _WIN32
    int result = _mkdir(dir);
#else
    int result = mkdir(dir, 0777);
#endif
    if (result == -1) {
        if (errno == EEXIST) {
            // Already exist
            return;
        } else {
            printf("%s:%d: error: Failed to make directory '%s'\n", __FILE__, __LINE__, dir);
            exit(1);
        }
    }
}

// must be freed by caller
char *get_absolute_path(const char *relative_path) {
#ifdef _WIN32
    return _fullpath(NULL, relative_path, 0);
#else
    return realpath(relative_path, NULL);
#endif
}

// Strips the filename of the given path to give the directory where the path is located
char *get_directory_from_path(char *path) {
    char *last_slash = strrchr(path, PATH_SEPARATOR);
    if (last_slash) {
        *last_slash = '\0';
    }
    return path;
}

// Joins two paths with the platform separator. Must be freed by caller
char *join_path(const char *a, const char *b) {
    size_t len = strlen(a) + 1 + strlen(b) + 1;
    char *result = malloc(len);
    snprintf(result, len, "%s%s%s", a, PATH_SEPARATOR_STR, b);
    return result;
}

#ifdef _WIN32
    // Declared by hand to avoid pulling in all of windows.h
    __declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(void *module, char *filename, unsigned long size);
#endif

// Absolute path of the running executable or NULL if it couldn't be determined. Must be freed by caller
char *get_executable_path() {
    char *buffer = calloc(4096, 1);
#ifdef _WIN32
    unsigned long len = GetModuleFileNameA(NULL, buffer, 4096);
    if (len == 0 || len >= 4096) { free(buffer); return NULL; }
#else
    ssize_t len = readlink("/proc/self/exe", buffer, 4095);
    if (len <= 0) { free(buffer); return NULL; }
#endif
    return buffer;
}

#endif
