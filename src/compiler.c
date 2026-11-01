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
#include <dirent.h>
#include <sys/stat.h>

//
// Namespaces
//
// A file declares its namespace with 'namespace game.physics;' at the top. Before parsing, the folder of the main file
// (and every folder below it) and the standard library are scanned for such files, so that 'import game.physics;' knows
// which files to load
//

// Returns the name in 'namespace a.b;' if the file starts with it (after comments), otherwise NULL
//
// Overlays: the text of a file that an editor has, which is not saved yet. 'saad index main.sd --overlay path=other' reads 'other'
// instead of 'path'
//

typedef struct Overlay {
    char *path;  // Absolute
    char *text_path;
} Overlay;

static Overlay overlays[64];
static int     overlay_count = 0;

bool add_overlay(const char *argument) {
    const char *equals = strchr(argument, '=');
    if (equals == NULL || overlay_count == 64) return false;

    char *path = calloc(equals - argument + 1, 1);
    memcpy(path, argument, equals - argument);
    char *absolute = get_absolute_path(path);
    overlays[overlay_count].path      = absolute ? absolute : path;
    overlays[overlay_count].text_path = strdup(equals + 1);
    overlay_count++;
    return true;
}

// Paths are not case sensitive on Windows, and an editor may write the drive letter in lowercase
static bool same_path(const char *a, const char *b) {
#ifdef _WIN32
    return _stricmp(a, b) == 0;
#else
    return strcmp(a, b) == 0;
#endif
}

// The file to read for a path: its overlay, or the path itself
static const char *source_path(const char *path) {
    if (overlay_count == 0) return path;
    char *absolute = get_absolute_path(path);
    if (absolute == NULL) return path;
    for (int i = 0; i < overlay_count; i++) {
        if (same_path(overlays[i].path, absolute)) return overlays[i].text_path;
    }
    return path;
}

static char *read_source(const char *path) {
    return read_entire_file(source_path(path));
}

static char *read_namespace_declaration(const char *path) {
    FILE *f = fopen(source_path(path), "rb");
    if (f == NULL) return NULL;

    char text[4096];
    size_t length = fread(text, 1, sizeof(text) - 1, f);
    fclose(f);
    text[length] = '\0';

    char *c = text;
    while (*c) {
        if (*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n') { c++; continue; }
        if (c[0] == '/' && c[1] == '/') { while (*c && *c != '\n') c++; continue; }
        if (c[0] == '/' && c[1] == '*') {
            c += 2;
            while (*c && !(c[0] == '*' && c[1] == '/')) c++;
            if (*c) c += 2;
            continue;
        }
        break;
    }

    if (strncmp(c, "namespace", 9) != 0 || !(c[9] == ' ' || c[9] == '\t')) return NULL;
    c += 9;
    while (*c == ' ' || *c == '\t') c++;

    char name[256];
    int n = 0;
    while (n < 255 && (starts_identifier(*c) || is_digit(*c) || *c == '.')) name[n++] = *c++;
    name[n] = '\0';
    if (n == 0) return NULL;
    return strdup(name);
}

static bool path_has_extension(const char *path, const char *extension) {
    size_t path_len = strlen(path), ext_len = strlen(extension);
    return path_len > ext_len + 1 && path[path_len - ext_len - 1] == '.' && strcmp(path + path_len - ext_len, extension) == 0;
}

static void scan_namespaces(CompilerConfig *config, Parser *parser, const char *dir, bool is_standard, int depth) {
    if (depth > 16) return;

    DIR *d = opendir(dir);
    if (d == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;
        if (name[0] == '.') continue;                 // ., .. and hidden folders like .git
        if (strcmp(name, "build") == 0) continue;     // Compiler output

        char *path = strcmp(dir, ".") == 0 ? strdup(name) : join_path(dir, name);

        struct stat info;
        if (stat(path, &info) != 0) continue;

        if (S_ISDIR(info.st_mode)) {
            // The standard library is scanned on its own, also when the project contains it
            char *absolute = get_absolute_path(path);
            char *stdlib   = get_absolute_path(config->stdlib_path);
            bool is_stdlib = !is_standard && absolute && stdlib && strcmp(absolute, stdlib) == 0;
            if (!is_stdlib) scan_namespaces(config, parser, path, is_standard, depth + 1);
            continue;
        }

        if (!path_has_extension(path, config->file_extension)) continue;

        char *ns_name = read_namespace_declaration(path);
        if (ns_name == NULL) continue;

        // The compiler later works from the folder of the main file, so the paths are made absolute now
        char *absolute = get_absolute_path(path);
        if (absolute == NULL) continue;

        Namespace *ns = add_namespace(parser, ns_name);
        if (is_standard) ns->is_standard = true;
        da_append(&ns->paths, absolute);
    }
    closedir(d);
}

static char *directory_of(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    if (backslash > slash) slash = backslash;
    if (slash == NULL) return strdup(".");

    size_t length = slash - path;
    char *dir = malloc(length + 1);
    memcpy(dir, path, length);
    dir[length] = '\0';
    return dir;
}

static void visit_namespace(DynamicArray *paths_to_visit, Namespace *ns) {
    for (int i = 0; i < ns->paths.count; i++) {
        da_append(paths_to_visit, ((char **)ns->paths.items)[i]);
    }
}

// Finds the namespace of 'import a.b;' or 'use a.b;'. 'import math.sin;' is the declaration 'sin' of the namespace 'math'
static bool resolve_namespace_import(Parser *parser, AstImport *import) {
    Namespace *ns = find_namespace(parser, import->path);
    if (ns) {
        import->ns = ns;
        return true;
    }

    char *last_dot = strrchr(import->path, '.');
    if (import->kind == IMPORT_NAMESPACE && last_dot) {
        char *parent = strdup(import->path);
        parent[last_dot - import->path] = '\0';

        Namespace *parent_ns = find_namespace(parser, parent);
        if (parent_ns) {
            import->kind = IMPORT_NAME;
            import->ns   = parent_ns;
            import->name = strdup(last_dot + 1);
            return true;
        }
    }

    report_error_ast(parser, LABEL_ERROR, (Ast *)import, "Unknown namespace '%s'", import->path);
    report_error_ast(parser, LABEL_HINT, NULL, "A file is in a namespace when it starts with 'namespace %s;'", import->path);
    return false;
}

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


// Finds the root folder of the compiler (the folder containing std/prelude/runtime.sd).
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
            char *prelude_dir  = join_path(std_dir, "prelude");
            char *runtime_path = join_path(prelude_dir, "runtime.sd");
            bool found = check_file_exists(runtime_path);
            free(std_dir);
            free(prelude_dir);
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

    // The imports of the prelude are resolved by the parser
    if (import->head.flags & AST_FLAG_COMPILER_GENERATED) return import->resolved_path != NULL;

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

    // import "fmt"; of a namespace of the standard library is the same as use fmt;
    if (import->resolved_path == NULL) {
        Namespace *ns = find_namespace(parser, import->string);
        if (ns && ns->is_standard) {
            import->kind = IMPORT_USE;
            import->path = import->string;
            import->ns   = ns;
            return true;
        }
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
    DynamicArray parsed_paths   = da_init(8, sizeof(char *)); // The absolute paths of the parsed files, as the same file can be reached by different paths
    da_append(&paths_to_visit, main_path);

    for (int cursor = 0; cursor < paths_to_visit.count; cursor++) {
        char *path = ((char **)paths_to_visit.items)[cursor];
        char *absolute = get_absolute_path(path);
        if (absolute == NULL) absolute = path;

        // Skip files that have already been parsed
        // @Speed: Optimize this O(N2) to a lookup
        bool is_visited = false;
        for (int i = 0; i < parsed_paths.count; i++) {
            if (strcmp(absolute, ((char **)parsed_paths.items)[i]) == 0) {
                is_visited = true;
                break;
            }
        }
        if (is_visited) continue;
        da_append(&parsed_paths, absolute);

        AstFile *file = parse_file(parser, path);
        if (file == NULL) return false;
        da_append(parsed_files, file);

        // The rest of the compiler works from the folder of the main file. Imports with a string are relative to
        // it and the output goes to its build folder
        if (cursor == 0) change_directory(config->working_directory);

        // A file of the project cannot join a namespace of the standard library
        if (file->ns && file->ns->is_standard) {
            char *absolute = get_absolute_path(path);
            char *stdlib   = get_absolute_path(config->stdlib_path);
            if (absolute && stdlib && strncmp(absolute, stdlib, strlen(stdlib)) != 0) {
                report_error_range(parser, (Pos){0, 1, 1}, (Pos){0, 1, 1}, "The namespace '%s' belongs to the standard library", file->ns->name);
                report_error_ast(parser, LABEL_HINT, NULL, "Give your namespace another name e.g mygame.%s", file->ns->name);
                return false;
            }
        }

        // The other files of the same namespace
        if (file->ns) visit_namespace(&paths_to_visit, file->ns);

        // Visit the imported files next
        for (int i = 0; i < file->imports.count; i++) {
            AstImport *import = ((AstImport **)file->imports.items)[i];

            if (import->kind != IMPORT_FILE) {
                if (!resolve_namespace_import(parser, import)) return false;
                visit_namespace(&paths_to_visit, import->ns);
                continue;
            }

            // @Improve: These error messages when we get a proper module system
            if (!resolve_import(config, parser, import, file)) return false;
            if (import->kind != IMPORT_FILE) {
                visit_namespace(&paths_to_visit, import->ns);
                continue;
            }

            da_append(&paths_to_visit, import->resolved_path);
        }

        // Namespaces used by a qualified name without an import e.g math.sin(x)
        for (int i = 0; i < file->referenced_roots.count; i++) {
            char  *root   = ((char **)file->referenced_roots.items)[i];
            size_t length = strlen(root);
            for (int j = 0; j < parser->namespaces.count; j++) {
                Namespace *ns = ((Namespace **)parser->namespaces.items)[j];
                if (strncmp(ns->name, root, length) == 0 && (ns->name[length] == '\0' || ns->name[length] == '.')) {
                    visit_namespace(&paths_to_visit, ns);
                }
            }
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
            if (import->kind != IMPORT_FILE) continue;

            for (int k = 0; k < parsed_files.count; k++) {
                AstFile *other_file = ((AstFile **)parsed_files.items)[k];
                char *a = get_absolute_path(import->resolved_path);
                char *b = get_absolute_path(other_file->absolute_path);
                bool same = (a && b) ? strcmp(a, b) == 0 : strcmp(import->resolved_path, other_file->absolute_path) == 0;
                if (same) {
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

//
// Diagnostics as JSON, for 'saad check --json'. The shape follows the Diagnostic of the Language Server Protocol:
//
//   {"version": 1, "success": false, "diagnostics": [
//       {"file": "/abs/main.sd", "severity": 1, "message": "...", "range": {"start": {"line": 3, "character": 4}, "end": {...}},
//        "related": [{"file": "...", "range": {...}, "message": "..."}]}]}
//
// Lines and characters count from 0, and a character is a byte of the UTF-8 text. Severity is 1 for errors and 2 for
// warnings. A note or a hint with a position is related information of the diagnostic before it, one without a position is
// added to its message
//

static void json_write_string(StringBuilder *sb, const char *text) {
    sb_append(sb, "\"");
    for (const char *c = text; c && *c; c++) {
        // Colors of the terminal e.g \x1b[1m
        if (*c == '\x1b' && c[1] == '[') {
            while (*c && *c != 'm') c++;
            if (!*c) break;
            continue;
        }
        switch (*c) {
        case '"':  sb_append(sb, "\\\""); break;
        case '\\': sb_append(sb, "\\\\"); break;
        case '\n': sb_append(sb, "\\n"); break;
        case '\r': sb_append(sb, "\\r"); break;
        case '\t': sb_append(sb, "\\t"); break;
        default:
            if ((unsigned char)*c < 32) sb_append(sb, "\\u%04x", (unsigned char)*c);
            else                        sb_append(sb, "%c", *c);
        }
    }
    sb_append(sb, "\"");
}

static void json_write_position(StringBuilder *sb, Pos pos) {
    int line = pos.line > 0 ? pos.line - 1 : 0;
    int col  = pos.col  > 0 ? pos.col  - 1 : 0;
    sb_append(sb, "{\"line\": %d, \"character\": %d}", line, col);
}

static void json_write_location(StringBuilder *sb, Diagnostic *d) {
    char *absolute = d->file_path ? get_absolute_path(d->file_path) : NULL;
    sb_append(sb, "\"file\": ");
    json_write_string(sb, absolute ? absolute : (d->file_path ? d->file_path : ""));
    sb_append(sb, ", \"range\": {\"start\": ");
    json_write_position(sb, d->start);
    sb_append(sb, ", \"end\": ");
    json_write_position(sb, d->start.line == -1 ? d->start : d->end);
    sb_append(sb, "}");
}

static bool is_main_diagnostic(Diagnostic *d) {
    return d->label == LABEL_ERROR || d->label == LABEL_WARNING;
}

static int severity_of(Diagnostic *d) {
    if (d->label == LABEL_ERROR)   return 1;
    if (d->label == LABEL_WARNING) return 2;
    if (d->label == LABEL_NOTE)    return 3;
    return 4;
}

// The diagnostics as a JSON array
// The errors that were reported, so far
static int error_count(void) {
    int count = 0;
    for (int i = 0; i < diagnostics.count; i++) {
        if (diagnostics.items[i].label == LABEL_ERROR) count++;
    }
    return count;
}

void write_diagnostics_array(StringBuilder *out) {
    StringBuilder sb = sb_init(1024);
    sb_append(&sb, "[");

    for (int i = 0; i < diagnostics.count;) {
        Diagnostic *d = &diagnostics.items[i];

        // The notes and hints after it
        int next = i + 1;
        while (next < diagnostics.count && !is_main_diagnostic(&diagnostics.items[next])) next++;
        if (!is_main_diagnostic(d)) next = i + 1; // A note on its own

        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\n  {");
        json_write_location(&sb, d);
        sb_append(&sb, ", \"severity\": %d, \"message\": ", severity_of(d));

        StringBuilder message = sb_init(128);
        sb_append(&message, "%s", d->message);
        for (int j = i + 1; j < next; j++) {
            Diagnostic *extra = &diagnostics.items[j];
            if (extra->start.line == -1) sb_append(&message, "\n%s: %s", extra->label == LABEL_HINT ? "hint" : "note", extra->message);
        }
        json_write_string(&sb, message.buffer);

        sb_append(&sb, ", \"related\": [");
        bool first = true;
        for (int j = i + 1; j < next; j++) {
            Diagnostic *extra = &diagnostics.items[j];
            if (extra->start.line == -1) continue;
            if (!first) sb_append(&sb, ", ");
            first = false;
            sb_append(&sb, "{");
            json_write_location(&sb, extra);
            sb_append(&sb, ", \"message\": ");
            json_write_string(&sb, extra->message);
            sb_append(&sb, "}");
        }
        sb_append(&sb, "]}");
        i = next;
    }

    sb_append(&sb, "\n]");
    sb_append(out, "%s", sb.buffer);
}

void write_diagnostics_json(FILE *out, bool success) {
    StringBuilder sb = sb_init(1024);
    sb_append(&sb, "{\"version\": 1, \"success\": %s, \"diagnostics\": ", success ? "true" : "false");
    write_diagnostics_array(&sb);
    sb_append(&sb, "}\n");
    fwrite(sb.buffer, 1, sb.cursor, out);
    fflush(out);
}

#include "index.c"
#include "complete.c"

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
    char *prelude_path              = join_path(config->stdlib_path, "prelude");
    char *c_bindings_path           = join_path(config->stdlib_path, "c");
    parser.runtime_file_path        = join_path(prelude_path, "runtime.sd");
    parser.builtin_string_file_path = join_path(prelude_path, "string.sd");
    parser.collections_file_path    = join_path(prelude_path, "collections.sd");
    parser.slices_file_path         = join_path(prelude_path, "slices.sd");
    parser.c_bindings_path          = get_absolute_path(c_bindings_path);
    parser.read_source              = read_source;
    ConstEvaluator ce     = const_evaluator_init(&parser);
    Typer typer           = typer_init(&parser, &ce);

    // ----------- Parse -----------
    report.parse_time_start = clock();
    DynamicArray parsed_files = da_init(8, sizeof(AstFile *));
    scan_namespaces(config, &parser, config->stdlib_path, true, 0);
    scan_namespaces(config, &parser, directory_of(main_path), false, 0);
    if (!parse_all_files(config, &parser, main_path, &parsed_files) || !link_imports(&parser, parsed_files)) {
        cleanup();
        return false;
    }
    AstFile *main_file = ((AstFile **)parsed_files.items)[0];
    report.parse_time_end = clock();

    // ----------- Typecheck -----------
    report.typer_time_start = clock();
    bool typed = check_file(&typer, main_file) && error_count() == 0; // Statements with errors were skipped, so the others were still checked
    index_declarations(parsed_files); // For 'saad index', also when there are errors, so an editor can still find what was checked
    if (program_completion.enabled) complete_program(&typer, parsed_files); // For 'saad complete', also when there are errors
    if (!typed) {
        cleanup();
        return false;
    }
    report.typer_time_end = clock();

    if (config->check_only) {
        reset_stdout();
        return true;
    }

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
