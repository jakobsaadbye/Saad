#include "ast.h"

typedef struct Parser {
    Lexer *lexer;

    Arena ast_nodes;

    int current_token_index;
    int current_ast_node_index;

    AstBlock *current_scope;
    TypeTable type_table;

    bool inside_statement_header;
} Parser;

Parser   parser_init(Lexer *lexer);
AstCode *parse_top_level_code(Parser *parser);

void *ast_allocate(Parser *parser, size_t size);
int   align_value(int value, int alignment);

void report_error_range(Parser *parser, Pos start, Pos end, const char *message, ...);
void report_error_ast(Parser *parser, const char* label, Ast *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);

#define For(T, arr, body)                                      \
    for (int _i = 0; _i < (arr).count; _i++) {                 \
        T it = ((T *)(arr).items)[_i];                         \
        int it_index = _i;                                     \
        (void)it_index;                                        \
        body                                                   \
    }                                                          

