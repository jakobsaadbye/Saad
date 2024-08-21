#include "code_generator.c"
#include <time.h>


void dump_tokens(Lexer *lexer) {
    printf("\nTokens: \n");
    for (unsigned int i = 0; i < lexer->token_index_cursor; i++) {
        printf("%s ", token_type_to_str(lexer->tokens[i].type));
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

    printf("\n-----------------------------\n");
    printf("Front-end time:\n");
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

bool send_through_pipeline(char *program, const char *program_path, bool output_to_console) {
    CompilerReport cr = {0};


    cr.lex_time_start = clock();
    Lexer lexer = lexer_init(program, program_path);
    bool ok = lex(&lexer);
    if (!ok) return false;
    cr.lex_time_end = clock();

    // dump_tokens(&lexer);


    cr.parse_time_start = clock();
    Parser parser = parser_init(&lexer);
    AstCode *code = (AstCode *) parse_top_level_code(&parser);
    if (code == NULL) return false;
    cr.parse_time_end = clock();


    cr.typer_time_start = clock();
    Typer typer = typer_init(&parser);
    check_code(&typer, code);
    cr.typer_time_end = clock();


    cr.codegen_time_start = clock();
    CodeGenerator cg = code_generator_init(&parser);
    go_nuts(&cg, code);
    code_generator_dump(&cg, "./build/out.asm");
    cr.codegen_time_end = clock();

    cr.asm_and_link_time_start = clock();
    system("nasm -fwin64 -g ./build/out.asm -o ./build/out.obj");
    int exit_code = system("gcc -o ./build/out.exe ./build/out.obj -lkernel32 -lmsvcrt");
    if (exit_code != 0) return false;
    cr.asm_and_link_time_end = clock();


    if (output_to_console) {
        exit_code = system(".\\build\\out.exe");
        print_compiler_report(cr);
    } else {
        exit_code = system(".\\build\\out.exe >nul");
    }
    
    if (exit_code != 0) return false;

    return true;
}