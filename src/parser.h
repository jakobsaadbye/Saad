#include "ast.h"

typedef struct Parser {
    Lexer *lexer;

    Arena ast_nodes;

    int current_token_index;
    int current_ast_node_index;

    AstBlock *current_scope;
    TypeTable type_table;

} Parser;

Parser           parser_init(Lexer *lexer);
AstCode         *parse_top_level_code(Parser *parser);
// Ast             *parse_statement(Parser *parser);
// AstDeclaration  *parse_declaration(Parser *parser, DeclarationFlags flags);
// AstAssignment   *parse_assignment(Parser *parser, AstExpr *lhs, Token op_token);
// AstFunctionDefn *parse_function_defn(Parser *parser);
// AstFunctionCall *parse_function_call(Parser *parser);
// AstStruct       *parse_struct(Parser *parser);
// AstEnum         *parse_enum(Parser *parser);
// AstIf           *parse_if(Parser *parser);
// AstFor          *parse_for(Parser *parser);
// AstPrint        *parse_print(Parser *parser);
// AstAssert       *parse_assert(Parser *parser);
// AstTypeof       *parse_typeof(Parser *parser);
// AstReturn       *parse_return(Parser *parser);
// Type            *parse_type(Parser *parser);
// AstExpr         *parse_expression(Parser *parser, int min_prec);
// AstExpr         *parse_range_or_normal_expression(Parser *parser);
// AstExpr         *parse_leaf(Parser *parser);

// AstDeclaration *make_declaration(Parser *parser, Token ident_token, AstExpr *expr, Type *type, DeclarationFlags flags);
// AstExpr        *make_member_access(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
// AstExpr        *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
// AstExpr        *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type);
// AstExpr        *make_literal_node(Parser *parser, Token token);
// AstIdentifier  *make_identifier_from_token(Parser *parser, Token ident_token, Type *type);
// AstIdentifier  *make_identifier_from_string(Parser *parser, const char *name, Type *type);

void *ast_allocate(Parser *parser, size_t size);
// void expect(Parser *parser, Token given, TokenType expected_type);
// void eat_token(Parser *parser);
// void vomit_token(Parser *parser);
// Token peek_token(Parser *parser, int lookahead);
// Token peek_next_token(Parser *parser);
// const char *label_color(const char *label);
// bool is_literal(Token token);
// bool is_type_specifier(Token token);
// bool is_primitive_type_token(Token token);
// bool is_assignment_operator(Token token);
// bool starts_struct_literal(Parser *parser);
// int get_precedence(OperatorType op);
int align_value(int value, int alignment);

void report_error_range(Parser *parser, Pos start, Pos end, const char *message, ...);
void report_error_ast(Parser *parser, const char* label, Ast *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);