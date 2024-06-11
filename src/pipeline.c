#include "code_generator.c"

void dump_tokens(Lexer *lexer) {
    printf("\nTokens: \n");
    for (unsigned int i = 0; i < lexer->token_index_cursor; i++) {
        printf("%s ", token_type_to_str(lexer->tokens[i].type));
    }
    printf("\n\n");
}

bool send_through_pipeline(char *program, const char *program_path, bool output_to_console) {
    bool ok = false;

    Lexer lexer = lexer_init(program, program_path);
    ok = lex(&lexer);
    if (!ok) return false;

    // dump_tokens(&lexer);

    Parser parser   = parser_init(&lexer);
    AstCode *code = (AstCode *) parse_top_level_code(&parser);
    if (code == NULL) return false;

    Typer typer = typer_init(&parser);
    check_code(&typer, code);

    CodeGenerator cg = code_generator_init(&parser);
    go_nuts(&cg, code);
    code_generator_dump(&cg, "./build/out.asm");

    // Assemble + linking
    system("nasm -fwin64 -g ./build/out.asm -o ./build/out.obj");
    int exit_code = system("gcc -o ./build/out.exe ./build/out.obj -lkernel32 -lmsvcrt");
    if (exit_code != 0) return false;

    if (output_to_console) {
        exit_code = system(".\\build\\out.exe");
    } else {
        exit_code = system(".\\build\\out.exe >nul");
    }
    
    if (exit_code != 0) return false;

    return true;
}