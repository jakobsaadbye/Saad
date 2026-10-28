#include <time.h>
#include <stdio.h>
#include "typer.c"

// Backend: AST -> IR -> register allocation -> x64
#include <math.h>
#include "ir.c"
#include "ir_lower.c"
#include "regalloc.c"
#include "x64_emit.c"

#ifdef _WIN32
    #include <io.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

typedef struct CompilerReport {
    CompilerConfig *config;
    clock_t lex_time_start;
    clock_t lex_time_end;
    clock_t parse_time_start;
    clock_t parse_time_end;
    clock_t typer_time_start;
    clock_t typer_time_end;
    clock_t ir_time_start;
    clock_t ir_time_end;
    clock_t codegen_time_start;
    clock_t codegen_time_end;
    clock_t asm_and_link_time_start;
    clock_t asm_and_link_time_end;
} CompilerReport;


// Finds the root folder of the compiler (the folder containing std/runtime.sd).
// Uses the SAAD_ROOT environment variable if set, otherwise searches upwards from the executable's location
char *find_compiler_root() {
    char *env_root = getenv("SAAD_ROOT");
    if (env_root != NULL && env_root[0] != '\0') {
        return env_root;
    }

    char *exe_path = get_executable_path();
    if (exe_path != NULL) {
        char *dir = get_directory_from_path(exe_path);

        for (int depth = 0; depth < 4; depth++) {
            char *std_dir      = join_path(dir, "std");
            char *runtime_path = join_path(std_dir, "runtime.sd");
            bool found = check_file_exists(runtime_path);
            free(std_dir);
            free(runtime_path);

            if (found) return dir;

            // Go one folder up
            char *last_slash = strrchr(dir, PATH_SEPARATOR);
            if (last_slash == NULL) break;
            *last_slash = '\0';
        }
    }

    return "C:\\Saad"; // Fallback to the default install location
}

CompilerConfig get_standard_compiler_config() {
    CompilerConfig config = {0};

    config.compiler_path  = find_compiler_root();
    config.stdlib_path    = join_path(config.compiler_path, "std");
    config.working_directory = get_current_directory();
    config.file_extension = "sd";
    config.backend = BACKEND_X64;
    config.optimization_level = OPTIMIZATION_LEVEL_O0;
    config.run_program = true;

    return config;
}

void dump_tokens(Lexer *lexer, int start_token_idx) {
    printf("\nTokens: \n");
    for (int i = start_token_idx; i < lexer->tokens.count; i++) {
        Token token = ((Token *)lexer->tokens.items)[i];
        printf("%s ", token_type_to_str(token.type));
    }
    printf("\n\n");
}

void print_compiler_report(CompilerReport cr) {
    double dt_lex      = (double)(cr.lex_time_end - cr.lex_time_start) / CLOCKS_PER_SEC;
    double dt_parse    = (double)(cr.parse_time_end - cr.parse_time_start) / CLOCKS_PER_SEC;
    double dt_typer    = (double)(cr.typer_time_end - cr.typer_time_start) / CLOCKS_PER_SEC;
    double dt_ir       = (double)(cr.ir_time_end - cr.ir_time_start) / CLOCKS_PER_SEC;
    double dt_codegen  = (double)(cr.codegen_time_end - cr.codegen_time_start) / CLOCKS_PER_SEC;
    double dt_asm_link = (double)(cr.asm_and_link_time_end - cr.asm_and_link_time_start) / CLOCKS_PER_SEC;

    double dt_frontend = dt_lex + dt_parse + dt_typer;
    double dt_backend = dt_ir + dt_codegen + dt_asm_link;
    double dt_total = dt_frontend + dt_backend;

    printf("\nFront-end time:\n");
    printf("   Lexer:\t %.3lf s.\n", dt_lex);
    printf("   Parser:\t %.3lf s.\n", dt_parse);
    printf("   Typing:\t %.3lf s.\n", dt_typer);
    printf("   Total:\t %.3lf s.\n", dt_frontend);

    printf("Backend-end time:\n");
    printf("   IR:\t\t %.3f s.\n", dt_ir);
    printf("   x64 codegen:\t %.3f s.\n", dt_codegen);
    printf("   Asm + Link:\t %.3f s.\n", dt_asm_link);
    printf("   Total:\t %.3lf s.\n", dt_backend);
    printf("Total time: %.3lf s.\n\n", dt_total);
}

int old_stdout = -1;
void reset_stdout(void) {
    dup2(old_stdout, 1);
} 

void cleanup() {
    reset_stdout();
}

bool resolve_import(CompilerConfig *config, Parser *parser, AstImport *import, AstFile *current_file) {

    char *import_path = malloc(strlen(import->string) + 8);
    sprintf(import_path, "%s.%s", import->string, config->file_extension);

    if (strcmp(import_path, current_file->absolute_path) == 0) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)import, "Recursive import of '%s'", import->string);
        return false;
    }

    if (check_file_exists(import_path)) {
        import->resolved_path = import_path;
    } else {
        // Check in the stdlib folder
        change_directory(config->stdlib_path);
        if (check_file_exists(import_path)) {
            import->resolved_path = join_path(config->stdlib_path, import_path);
        }
        change_directory(config->working_directory);
    }

    if (import->resolved_path == NULL) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)import, "Unable to resolve import '%s'", import->string);
        report_error_ast(parser, LABEL_NOTE, NULL, "Current working directory is: %s", config->working_directory);
        return false;
    }

    return true;
}

void write_text_file(const char *path, StringBuilder sb) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        printf("%s:%d: error: Failed to open file '%s'\n", __FILE__, __LINE__, path);
        exit(1);
    }
    fwrite(sb.buffer, 1, sb.cursor, f);
    fclose(f);
}

// Parses the main file and every file it imports (directly or indirectly). The main file is parsed_files[0]
static bool parse_all_files(CompilerConfig *config, Parser *parser, const char *main_path, DynamicArray *parsed_files) {
    DynamicArray paths_to_visit = da_init(8, sizeof(char *));
    da_append(&paths_to_visit, main_path);

    for (int cursor = 0; cursor < paths_to_visit.count; cursor++) {
        char *path = ((char **)paths_to_visit.items)[cursor];

        // Skip files that have already been parsed
        // @Speed: Optimize this O(N2) to a lookup
        bool is_visited = false;
        for (int i = 0; i < parsed_files->count; i++) {
            AstFile *file = ((AstFile **)parsed_files->items)[i];
            if (strcmp(path, file->absolute_path) == 0) {
                is_visited = true;
                break;
            }
        }
        if (is_visited) continue;

        AstFile *file = parse_file(parser, path);
        if (file == NULL) return false;
        da_append(parsed_files, file);

        // Visit the imported files next
        for (int i = 0; i < file->imports.count; i++) {
            AstImport *import = ((AstImport **)file->imports.items)[i];

            // @Improve: These error messages when we get a proper module system
            if (!resolve_import(config, parser, import, file)) return false;

            da_append(&paths_to_visit, import->resolved_path);
        }
    }

    da_free(&paths_to_visit);
    return true;
}

// Points every import at the parsed file it refers to
static bool link_imports(Parser *parser, DynamicArray parsed_files) {
    for (int i = 0; i < parsed_files.count; i++) {
        AstFile *file = ((AstFile **)parsed_files.items)[i];

        for (int j = 0; j < file->imports.count; j++) {
            AstImport *import = ((AstImport **)file->imports.items)[j];

            for (int k = 0; k < parsed_files.count; k++) {
                AstFile *other_file = ((AstFile **)parsed_files.items)[k];
                if (strcmp(import->resolved_path, other_file->absolute_path) == 0) {
                    import->imported_file = other_file;
                    break;
                }
            }

            if (import->imported_file == NULL) {
                report_error_ast(parser, LABEL_ERROR, (Ast *)import, "Compiler Error: Failed to link the following import");
                return false;
            }
        }
    }
    return true;
}

// Assembles ./build/out.asm and links it into ./build/out.exe
static bool assemble_and_link(CompilerConfig *config) {
    if (system("nasm -fwin64 -g ./build/out.asm -o ./build/out.obj") != 0) {
        printf("error: Failed to assemble ./build/out.asm\n");
        return false;
    }

#ifdef _WIN32
    char link_command[2048];
    snprintf(link_command, sizeof(link_command),
        "gcc -o ./build/out.exe ./build/out.obj "
        "-L\"%s/packages/runtime/lib\" "
        "-L\"%s/packages/raylib/lib\" "
        "-lruntime_support -lraylib "
        "-lkernel32 -lmsvcrt -lgdi32 -lwinmm -lopengl32 -ld3d9 -ldxguid",
        config->compiler_path, config->compiler_path
    );
    return system(link_command) == 0;
#else
    // The x64 backend targets Win64, so we can only assemble (not link or run) on other platforms
    (void) config;
    printf("note: Skipping linking. The Win64 target can only be linked on Windows\n");
    return true;
#endif
}

// Runs ./build/out.exe. Returns true if the program exited with code 0
static bool run_executable(bool output_to_console) {
#ifdef _WIN32
    int exit_code = system(output_to_console ? ".\\build\\out.exe" : ".\\build\\out.exe >nul");
    return exit_code == 0;
#else
    (void) output_to_console;
    printf("error: Running programs is only supported on Windows. Use 'build' instead\n");
    return false;
#endif
}

bool compile_program(CompilerConfig *config, const char *main_path, bool output_to_console) {
    CompilerReport report = {
        .config = config
    };

    old_stdout = dup(1);
    if (!output_to_console) {
        freopen(NULL_DEVICE, "w", stdout);
    }
    
    Lexer lexer           = lexer_init();
    Parser parser         = parser_init(&lexer);
    parser.runtime_file_path        = join_path(config->stdlib_path, "runtime.sd");
    parser.builtin_string_file_path = join_path(config->stdlib_path, "builtin_string.sd");
    ConstEvaluator ce     = const_evaluator_init(&parser);
    Typer typer           = typer_init(&parser, &ce);

    // ----------- Parse -----------
    report.parse_time_start = clock();
    DynamicArray parsed_files = da_init(8, sizeof(AstFile *));
    if (!parse_all_files(config, &parser, main_path, &parsed_files) || !link_imports(&parser, parsed_files)) {
        cleanup();
        return false;
    }
    AstFile *main_file = ((AstFile **)parsed_files.items)[0];
    report.parse_time_end = clock();

    // ----------- Typecheck -----------
    report.typer_time_start = clock();
    if (!check_file(&typer, main_file)) {
        cleanup();
        return false;
    }
    report.typer_time_end = clock();

    make_directory("build");

    // ----------- IR -----------
    report.ir_time_start = clock();
    IrModule *module = lower_program(&parser, main_file);
    if (module == NULL) {
        cleanup();
        return false;
    }

    StringBuilder ir_text = sb_init(1 << 14);
    ir_dump_module(module, &ir_text);
    write_text_file("./build/out.ir", ir_text);

    if (!ir_verify_module(module)) {
        printf("Compiler Error: The generated IR is invalid. See ./build/out.ir\n");
        cleanup();
        return false;
    }
    report.ir_time_end = clock();

    // ----------- x64 -----------
    report.codegen_time_start = clock();
    X64Output x64 = x64_emit_module(module);
    write_text_file("./build/out.regalloc.txt", x64.regalloc_dump);
    write_text_file("./build/out.asm", x64.assembly);
    report.codegen_time_end = clock();

    // ----------- Assemble + link -----------
    report.asm_and_link_time_start = clock();
    if (!assemble_and_link(config)) {
        cleanup();
        return false;
    }
    report.asm_and_link_time_end = clock();

    if (output_to_console) {
        print_compiler_report(report);
    }

    if (config->run_program && !run_executable(output_to_console)) {
        cleanup();
        return false;
    }

    reset_stdout();
    return true;
}
