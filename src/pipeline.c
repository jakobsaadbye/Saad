#include <time.h>
#include <stdio.h>
#include "code_generator.c"

#ifdef __APPLE__
    #include <sys/uio.h>
    #include <fcntl.h> 
    #include <unistd.h>
#else
    #include <io.h>
#endif

void dump_tokens(Lexer *lexer, int start_token_idx) {
    printf("\nTokens: \n");
    for (int i = start_token_idx; i < lexer->tokens.count; i++) {
        Token token = ((Token *)lexer->tokens.items)[i];
        printf("%s ", token_type_to_str(token.type));
    }
    printf("\n\n");
}

typedef struct CompilerReport {
    clock_t lex_time_start;
    clock_t lex_time_end;
    clock_t parse_time_start;
    clock_t parse_time_end;
    clock_t typer_time_start;
    clock_t typer_time_end;
    clock_t codegen_time_start;
    clock_t codegen_time_end;
    clock_t asm_and_link_time_start;
    clock_t asm_and_link_time_end;
} CompilerReport;

void print_compiler_report(CompilerReport cr) {
    double dt_lex      = (double)(cr.lex_time_end - cr.lex_time_start) / CLOCKS_PER_SEC;
    double dt_parse    = (double)(cr.parse_time_end - cr.parse_time_start) / CLOCKS_PER_SEC;
    double dt_typer    = (double)(cr.typer_time_end - cr.typer_time_start) / CLOCKS_PER_SEC;
    double dt_codegen  = (double)(cr.codegen_time_end - cr.codegen_time_start) / CLOCKS_PER_SEC;
    double dt_asm_link = (double)(cr.asm_and_link_time_end - cr.asm_and_link_time_start) / CLOCKS_PER_SEC;

    double dt_total = dt_lex + dt_parse + dt_typer + dt_codegen + dt_asm_link;

    printf("\nFront-end time:\n");
    printf("   Lexer:\t %.3lf s.\n", dt_lex);
    printf("   Parser:\t %.3lf s.\n", dt_parse);
    printf("   Total:\t %.3lf s.\n", dt_lex + dt_parse);
    printf("Backend-end time:\n");
    printf("   Typing:\t %.3lf s.\n", dt_typer);
    printf("   X86-codegen:\t %.3f s.\n", dt_codegen);
    printf("   Asm + Link:\t %.3f s.\n", dt_asm_link);
    printf("   Total:\t %.3lf s.\n", dt_typer+ dt_codegen);
    printf("\nTotal time: %.3lf s.\n", dt_total);
}

int old_stdout = -1;
void reset_stdout(void) {
    dup2(old_stdout, 1);
} 

void output_generated_code_to_file(CodeGenerator *cg, const char *output_path) {
    FILE *f = fopen(output_path, "w");
    if (f == NULL) {
        printf("%s:%d: error: Failed to open file '%s'\n", __FILE__, __LINE__, output_path);
        exit(1);
    }

    fwrite(cg->head.buffer, 1, cg->head.cursor, f);
    fwrite(cg->data.buffer, 1, cg->data.cursor, f);
    fwrite(cg->code_header.buffer, 1, cg->code_header.cursor, f);
    fwrite("\n", 1, 1, f);
    fwrite(cg->code.buffer, 1, cg->code.cursor, f);

    fclose(f);
}

void cleanup() {
    reset_stdout();
}

bool compile_program(const char *main_path, bool output_to_console) {
    CompilerReport report = {0};

    old_stdout = dup(1);
    if (!output_to_console) {
        freopen("nul", "w", stdout);
    }

    
    Lexer lexer           = lexer_init();
    Parser parser         = parser_init(&lexer);
    ConstEvaluator ce     = const_evaluator_init(&parser);
    Typer typer           = typer_init(&parser, &ce);
    CodeGenerator codegen = code_generator_init(&parser);

    DynamicArray  parsed_files = da_init(8, sizeof(AstFile *));
    DynamicArray  file_paths_to_visit = da_init(2, sizeof(char *));
    int           file_path_cursor = 0;
    AstFile       *current_file = NULL;
    char          *current_file_path = NULL;

    da_append(&file_paths_to_visit, main_path);

    //
    // Parse all the files
    //
    while (true) {
        current_file_path = ((char **)file_paths_to_visit.items)[file_path_cursor];

        // Parse file
        current_file = (AstFile *) parse_file(&parser, current_file_path);
        if (current_file == NULL) {
            cleanup();
            return false;
        }
        da_append(&parsed_files, current_file);

        // Add any file imports as file paths to visit next
        for (int i = 0; i < current_file->imports.count; i++) {
            AstImport *ast_import = ((AstImport **)current_file->imports.items)[i];
            da_append(&file_paths_to_visit, ast_import->resolved_path);
        }

        file_path_cursor += 1;

        if (file_paths_to_visit.count == file_path_cursor) {
            break;
        }
    }

    // Link all the parsed files together via their imports
    for (int i = 0; i < parsed_files.count; i++) {
        AstFile *file = ((AstFile **)parsed_files.items)[i];
        for (int j = 0; j < file->imports.count; j++) {
            AstImport *import = ((AstImport **)file->imports.items)[j];
            
            // Do a linear scan to find the corresponding AstFile *
            for (int k = 0; k < parsed_files.count; k++) {
                AstFile *other_file = ((AstFile **)parsed_files.items)[k];

                if (strcmp(import->resolved_path, other_file->absolute_path) == 0) {
                    import->imported_file = other_file;
                }
            }
        }
    }

    AstFile *main_file = ((AstFile **)parsed_files.items)[0];

    // Typecheck
    bool typecheck_ok = check_file(&typer, main_file);
    if (!typecheck_ok) {
        cleanup();
        return false;
    }
    
    begin_emit_code(&codegen, main_file);

    make_directory("build");

    output_generated_code_to_file(&codegen, "./build/out.asm");

    report.asm_and_link_time_start = clock();
    system("nasm -fwin64 -g ./build/out.asm -o ./build/out.obj");
    // int exit_code = system("gcc -o ./build/out.exe ./build/out.obj -lkernel32 -lmsvcrt");
    int exit_code = system("gcc -o ./build/out.exe ./build/out.obj -Lpackages/raylib/lib -lraylib -lkernel32 -lmsvcrt -lgdi32 -lwinmm -lopengl32 -ld3d9 -ldxguid");
    if (exit_code != 0) {
        cleanup();
        return false;
    }
    report.asm_and_link_time_end = clock();


    if (output_to_console) {
        print_compiler_report(report);
        exit_code = system(".\\build\\out.exe");
    } else {
        exit_code = system(".\\build\\out.exe >nul");
    }
    
    if (exit_code != 0) {
        cleanup();
        return false;
    } 

    reset_stdout();
    return true;
}
