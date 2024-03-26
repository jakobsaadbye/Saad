#include "code_generator.c"

bool send_through_pipeline(char *program, const char *program_path) {
    bool ok = false;

    Lexer lexer = lexer_init(program, program_path);
    ok = lex(&lexer);
    if (!ok) return false;

    #ifdef DEBUG
        printf("\nTokens: \n");
        for (unsigned int i = 0; i < lexer.token_index_cursor; i++) {
            printf("%s ", token_type_to_str(lexer.tokens[i].type));
        }
        printf("\n\n");
    #endif

    Parser parser   = parser_init(&lexer);
    AstCode *code = (AstCode *) parse_top_level_code(&parser);
    if (code == NULL) return false;

    Typer typer = typer_init(&parser);
    check_code(&typer, code);

    CodeGenerator cg = code_generator_init(&parser);
    go_nuts(&cg, code);
    code_generator_dump(&cg, "./build/out.asm");

    system("nasm -fwin64 -g ./build/out.asm -o ./build/out.obj");
    int exit_code = system("gcc -o ./build/out.exe ./build/out.obj -lkernel32 -lmsvcrt");

    return exit_code == 0;
}