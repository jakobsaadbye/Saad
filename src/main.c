#define DEBUG

#include "compiler.c"

typedef enum CommandKind {
    COMMAND_BUILD,
    COMMAND_RUN,
    COMMAND_CHECK,
    COMMAND_INDEX,
    COMMAND_COMPLETE,
} CommandKind;

typedef struct Command {
    const char  *name;
    CommandKind  kind;
    const char  *description;
} Command;

// Add new commands here
static Command commands[] = {
    { "build", COMMAND_BUILD, "Compile the program to ./build/out.exe without running it. --debug makes it with nasm and gcc, with debug information for gdb" },
    { "run",   COMMAND_RUN,   "Compile the program and run it. Takes --debug too" },
    { "check", COMMAND_CHECK, "Check the program for errors without compiling it. With --json, the errors are written as JSON" },
    { "index", COMMAND_INDEX, "Write where every name of the program is declared and used, and its errors, as JSON for an editor. --overlay path=file reads file instead of path. --std also writes the standard library" },
    { "complete", COMMAND_COMPLETE, "Write what can be written at --at file:line:character (from 0), and the signature of the call there, as JSON for an editor. Takes --overlay too" },
};
static const int commands_count = sizeof(commands) / sizeof(commands[0]);

void print_usage() {
    printf("Usage:\n");
    printf("   saad.exe <command> <input_file.sd> [options]\n\n");
    printf("Commands:\n");
    for (int i = 0; i < commands_count; i++) {
        printf("   %-10s %s\n", commands[i].name, commands[i].description);
    }
    printf("\n");
}

Command *find_command(const char *name) {
    for (int i = 0; i < commands_count; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

// Parses the options after the input file. Returns false on an unknown option
bool parse_options(CompilerConfig *config, int argc, char const *argv[], int first_option) {
    for (int i = first_option; i < argc; i++) {
        const char *option = argv[i];

        if (strcmp(option, "--json") == 0) { config->json = true; continue; }
        if (strcmp(option, "--std") == 0)  { program_index.with_std = true; continue; }
        if (strcmp(option, "--debug") == 0) { config->debug = true; continue; }
        if (strcmp(option, "--elf") == 0)   { config->emit_elf = true; continue; }
        if (strcmp(option, "--at") == 0 && i + 1 < argc) {
            // file:line:character. The file can have a ':' itself, e.g C:/dev/main.sd
            char *at = strdup(argv[i + 1]);
            char *last = strrchr(at, ':');
            char *middle = NULL;
            if (last) { *last = '\0'; middle = strrchr(at, ':'); }
            if (!last || !middle) {
                printf("Error: --at takes file:line:character, e.g --at main.sd:10:4\n\n");
                return false;
            }
            *middle = '\0';
            char *absolute = get_absolute_path(at); // Now, as the compiler changes its directory to the one of the main file
            program_completion.path      = absolute ? absolute : at;
            program_completion.line      = atoi(middle + 1);
            program_completion.character = atoi(last + 1);
            i++;
            continue;
        }
        if (strcmp(option, "--overlay") == 0 && i + 1 < argc) {
            if (!add_overlay(argv[i + 1])) {
                printf("Error: --overlay takes path=file, e.g --overlay src/main.sd=/tmp/unsaved.sd\n\n");
                return false;
            }
            i++;
            continue;
        }

        // Add new options here, e.g.
        // if (strcmp(option, "-O1") == 0) { config->optimization_level = OPTIMIZATION_LEVEL_O1; continue; }

        printf("Error: Unknown option '%s'\n\n", option);
        return false;
    }

    return true;
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        printf("Error: Missing command\n\n");
        print_usage();
        return 1;
    }

    const char *command_name = argv[1];
    if (strcmp(command_name, "help") == 0 || strcmp(command_name, "-h") == 0 || strcmp(command_name, "--help") == 0) {
        print_usage();
        return 0;
    }

    Command *command = find_command(command_name);
    if (command == NULL) {
        printf("Error: Unknown command '%s'\n\n", command_name);
        print_usage();
        return 1;
    }

    if (argc < 3) {
        printf("Error: Missing input file\n\n");
        print_usage();
        return 1;
    }

    const char *main_file_path = argv[2];
    if (!check_file_exists(main_file_path)) {
        printf("Error: Could not find input file '%s'\n", main_file_path);
        return 1;
    }

    CompilerConfig config = get_standard_compiler_config();

    if (!parse_options(&config, argc, argv, 3)) {
        print_usage();
        return 1;
    }

    switch (command->kind) {
        case COMMAND_BUILD: config.run_program = false; break;
        case COMMAND_RUN:   config.run_program = true;  break;
        case COMMAND_CHECK: config.run_program = false; config.check_only = true; break;
        case COMMAND_INDEX: config.run_program = false; config.check_only = true; config.json = true; break;
        case COMMAND_COMPLETE: config.run_program = false; config.check_only = true; config.json = true; break;
    }

    char *main_file_path_abs = get_absolute_path(main_file_path);
    char *main_directory = get_directory_from_path(strdup(main_file_path_abs)); // It cuts the path where the directory ends

    config.working_directory = main_directory;

    if (command->kind == COMMAND_INDEX) {
        diagnostics.print      = false;
        program_index.enabled  = true;
        program_index.std_path = get_absolute_path(config.stdlib_path);
        bool ok = compile_program(&config, main_file_path_abs, false);
        write_index_json(stdout, ok);
        return ok ? 0 : 1;
    }

    if (command->kind == COMMAND_COMPLETE) {
        if (program_completion.path == NULL) {
            printf("Error: complete needs --at file:line:character\n\n");
            return 1;
        }
        diagnostics.print = false;
        program_completion.enabled = true;
        compile_program(&config, main_file_path_abs, false);
        write_completion_json(stdout);
        return 0;
    }

    if (command->kind == COMMAND_CHECK) {
        // Only the diagnostics are written, so an editor can read them
        if (config.json) diagnostics.print = false;
        bool ok = compile_program(&config, main_file_path_abs, !config.json);
        if (config.json) {
            write_diagnostics_json(stdout, ok);
        } else if (ok) {
            printf("\nNo errors.\n");
        } else {
            printf("There were errors.\n\n");
        }
        return ok ? 0 : 1;
    }

    bool success = compile_program(&config, main_file_path, true);
    if (success) {
        printf("\n\nCompiled successfully.\n\n");
        return 0;
    } else {
        printf("There were errors.\n\n");
        return -1;
    }
}
