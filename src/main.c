#define DEBUG

#include "compiler.c"

typedef enum CommandKind {
    COMMAND_BUILD,
    COMMAND_RUN,
    COMMAND_CHECK,
    COMMAND_INDEX,
    COMMAND_COMPLETE,
} CommandKind;

typedef struct CommandOption {
    const char *name;
    const char *description;
} CommandOption;

typedef struct Command {
    const char    *name;
    CommandKind    kind;
    const char    *description;
    bool           for_editors;   // Used by the language server, not typed by people. Listed apart
    CommandOption  options[12];   // What 'saad <command> help' shows
} Command;

#ifdef SAAD_LLVM
#define LLVM_DESCRIPTION "Optimize with LLVM"
#else
#define LLVM_DESCRIPTION "Optimize with LLVM (not in this build)"
#endif

#define BUILD_OPTIONS \
    { "--debug",     "Add debug information, for LLDB or GDB" }, \
    { "--llvm",      LLVM_DESCRIPTION }, \
    { "-O0 .. -O3",  "How much LLVM optimizes: -O2, or -O0 with --debug" }, \
    { "--unchecked", "Leave out the bounds checks of indexes and the null checks of pointers" }, \
    { "--check-memory", "Find uses of the memory of arenas that were cleared or freed, at a cost of speed and memory" }, \
    { "--os <os>",   "The OS of #if: windows or linux" }, \
    { "--elf",       "Write build/out.o for Linux (tools/linux)" }, \
    { "--dump",      "Write the IR and the registers to build/out.ir and out.regalloc.txt" }

// Add new commands here
static Command commands[] = {
    { "build", COMMAND_BUILD, "Compile the program to build/out.exe", false, { BUILD_OPTIONS } },
    { "run",   COMMAND_RUN,   "Compile the program and run it", false, {
        BUILD_OPTIONS,
        { "-- <args>",   "Arguments for the program, e.g -- a.txt" } } },
    { "check", COMMAND_CHECK, "Report the errors of the program", true, {
        { "--json",      "Write the errors as JSON" },
        { "--os <os>",   "The OS of #if: windows or linux" } } },
    { "index", COMMAND_INDEX, "Write the names of the program as JSON", true, {
        { "--overlay <path>=<file>", "Read file instead of path" },
        { "--std",       "Also the standard library" },
        { "--all-std",   "Check all of the standard library too" },
        { "--os <os>",   "The OS of #if: windows or linux" } } },
    { "complete", COMMAND_COMPLETE, "Write the completions at a place as JSON", true, {
        { "--at <file:line:character>", "Where, from 0 (needed)" },
        { "--overlay <path>=<file>", "Read file instead of path" } } },
};
static const int commands_count = sizeof(commands) / sizeof(commands[0]);

void print_usage() {
    printf("Usage: saad <command> <file.sd> [options]\n\n");
    printf("Commands:\n");
    for (int i = 0; i < commands_count; i++) {
        if (!commands[i].for_editors) printf("  %-10s %s\n", commands[i].name, commands[i].description);
    }
    printf("\nOptions of a command: saad <command> help\n\n");
    printf("For editors (used by the language server):\n");
    for (int i = 0; i < commands_count; i++) {
        if (commands[i].for_editors) printf("  %-10s %s\n", commands[i].name, commands[i].description);
    }
    printf("\n");
}

// The options of a command: 'saad build help'
void print_command_usage(Command *command) {
    printf("Usage: saad %s <file.sd> [options]\n\n", command->name);
    printf("%s.\n\n", command->description);
    printf("Options:\n");
    int width = 0;
    for (int i = 0; i < 12 && command->options[i].name; i++) {
        int length = (int) strlen(command->options[i].name);
        if (length > width) width = length;
    }
    for (int i = 0; i < 12 && command->options[i].name; i++) {
        printf("  %-*s  %s\n", width, command->options[i].name, command->options[i].description);
    }
    printf("\n");
}

static bool is_help(const char *arg) {
    return strcmp(arg, "help") == 0 || strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0;
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
    config->opt_level = -1;
    bool os_given = false;
    for (int i = first_option; i < argc; i++) {
        const char *option = argv[i];

        if (strcmp(option, "--") == 0) {
            // The rest is for the program, e.g saad run tool.sd -- input.txt
            config->program_argc = argc - (i + 1);
            config->program_argv = &argv[i + 1];
            break;
        }
        if (strcmp(option, "--json") == 0) { config->json = true; continue; }
        if (strcmp(option, "--std") == 0)  { program_index.with_std = true; continue; }
        if (strcmp(option, "--all-std") == 0) { program_index.with_std = true; program_index.all_std = true; continue; }
        if (strcmp(option, "--debug") == 0) { config->debug = true; continue; }
        if (strcmp(option, "--dump") == 0)  { config->dump = true; continue; }
        if (strcmp(option, "--elf") == 0)   { config->emit_elf = true; if (!os_given) target_os = "linux"; continue; }
        if (strcmp(option, "--os") == 0 && i + 1 < argc) {
            // The OS of #if, e.g for 'saad index' of the code for Linux
            const char *os = argv[++i];
            if (strcmp(os, "windows") != 0 && strcmp(os, "linux") != 0) {
                printf("Error: --os is windows or linux\n\n");
                return false;
            }
            target_os = os;
            os_given  = true;
            continue;
        }
        if (strcmp(option, "--unchecked") == 0) { bounds_checks = false; continue; } // No bounds checks of indexes
        if (strcmp(option, "--check-memory") == 0) { memory_checks = true; continue; } // See checkedAllocate in std/prelude/errors.sd
        if (strcmp(option, "--llvm") == 0)  { config->use_llvm = true; continue; }
        if (option[0] == '-' && option[1] == 'O' && option[2] >= '0' && option[2] <= '3' && option[3] == 0) {
            config->opt_level = option[2] - '0';   // -O0 to -O3, with --llvm
            continue;
        }
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
    if (is_help(command_name)) {
        // saad help, or saad help build
        Command *about = argc >= 3 ? find_command(argv[2]) : NULL;
        if (about) print_command_usage(about);
        else       print_usage();
        return 0;
    }

    Command *command = find_command(command_name);
    if (command == NULL) {
        printf("Error: Unknown command '%s'\n\n", command_name);
        print_usage();
        return 1;
    }

    if (argc < 3) {
        printf("Error: Missing the file of the program\n\n");
        print_command_usage(command);
        return 1;
    }
    if (is_help(argv[2])) {
        print_command_usage(command);
        return 0;
    }

    const char *main_file_path = argv[2];
    if (!check_file_exists(main_file_path)) {
        printf("Error: Could not find input file '%s'\n", main_file_path);
        return 1;
    }

    CompilerConfig config = get_standard_compiler_config();
    config.caller_directory = get_current_directory();

    if (!parse_options(&config, argc, argv, 3)) {
        print_command_usage(command);
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
        keep_inferred_literals     = true;
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
