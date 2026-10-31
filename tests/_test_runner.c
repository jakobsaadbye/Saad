#include "dirent.h"
#include "../src/compiler.c"

#define TEST_OUTPUT_LEN 60
#define TEST_CAPTURE_PATH "build/test_output.txt"

// Returns a copy of the text with the ANSI color codes removed
char *strip_colors(const char *text) {
    char *result = malloc(strlen(text) + 1);
    int j = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\x1b' && text[i + 1] == '[') {
            i += 2;
            while (text[i] != '\0' && text[i] != 'm') i++;
            if (text[i] == '\0') break;
            continue;
        }
        result[j++] = text[i];
    }
    result[j] = '\0';
    return result;
}

// A should_fail test can require the error output to contain some text with lines like:
//   // expect-error: Conflicting types for type parameter 'T'
// Returns the first expected text missing from the output, or NULL if all of them are there
char *find_missing_expected_error(const char *file_path, const char *output) {
    char *source = read_entire_file((char *) file_path);
    if (source == NULL) return NULL;

    const char *marker = "// expect-error: ";
    char *cursor = source;
    while ((cursor = strstr(cursor, marker)) != NULL) {
        cursor += strlen(marker);
        char *line_end = cursor;
        while (*line_end != '\0' && *line_end != '\n' && *line_end != '\r') line_end++;

        char *expected = calloc(line_end - cursor + 1, 1);
        memcpy(expected, cursor, line_end - cursor);
        if (strstr(output, expected) == NULL) return expected;
        free(expected);

        cursor = line_end;
    }
    return NULL;
}

// Compiles the program while capturing everything it prints. Returns the output without colors
char *compile_and_capture(CompilerConfig *config, const char *file_path, bool *compiled) {
    fflush(stdout);
    int saved_stdout = dup(1);
    FILE *capture = fopen(TEST_CAPTURE_PATH, "w");
    if (capture == NULL) {
        *compiled = compile_program(config, file_path, false);
        return NULL;
    }
    dup2(fileno(capture), 1);

    *compiled = compile_program(config, file_path, true);

    fflush(stdout);
    dup2(saved_stdout, 1);
    close(saved_stdout);
    fclose(capture);

    char *output = read_entire_file(TEST_CAPTURE_PATH);
    if (output == NULL) return NULL;
    return strip_colors(output);
}

void check_entire_directory(CompilerConfig *config, const char *dir_name, bool should_fail) {
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
        char *dot = strrchr(filename, '.'); // Entries without a dot (e.g folders) have no extension
        if (dot == NULL || strcmp(dot + 1, "sd") != 0) continue;

        char file_path[512];
        sprintf(file_path, "%s/%s", dir_name, filename);

        char pad_str[TEST_OUTPUT_LEN];
        memset(pad_str, 0, TEST_OUTPUT_LEN);

        int pad_len = TEST_OUTPUT_LEN - strlen(file_path);
        for (int i = 0; i < pad_len; i++) {
            pad_str[i] = '.';
        }

        printf("Run %s %s ", file_path, pad_str);
        
        bool compiled = false;
        char *missing_error = NULL;
        if (should_fail) {
            char *output = compile_and_capture(config, file_path, &compiled);
            if (output) missing_error = find_missing_expected_error(file_path, output);
        } else {
            compiled = compile_program(config, file_path, false);
        }

        bool ok = false;
        if (should_fail)
            ok = !compiled && missing_error == NULL;
        else
            ok = compiled;

        if (ok) {
            printf(COLOR_GREEN"OK\n"COLOR_RESET);
            succeeded += 1;
        } else if (!compiled && missing_error) {
            printf(COLOR_RED"FAILED (wrong error)\n"COLOR_RESET);
            printf("    expected the error to contain: %s\n", missing_error);
            failed += 1;
        } else {
            printf(COLOR_RED"FAILED\n"COLOR_RESET);
            failed += 1;
        }
        fflush(stdout); // Output is buffered when it goes to a pipe (e.g PowerShell), so don't lose it if something crashes

        num_tests += 1;
    }

    // printf("\nRan %d tests, %d OK, %d FAILED\n", num_tests, succeeded, failed);
    // printf("------------------------------------\n");
}

int main() {
    CompilerConfig config = get_standard_compiler_config();

    // check_entire_directory(&config, "examples", false);
    check_entire_directory(&config, "tests", false);
    check_entire_directory(&config, "tests/std", false);
    check_entire_directory(&config, "tests/backend", false);
    check_entire_directory(&config, "tests/should_fail", true);

    return 0;
}
