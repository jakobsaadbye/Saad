#include "ast.h"
#include "lib/file.h"

typedef struct Parser {
    Lexer *lexer;

    Arena ast_nodes;

    int current_token_index;
    int current_ast_node_index;

    AstFunctionDefn *enclosing_function;
    AstBlock *current_scope;
    AstFile  *current_file;
    TypeTable type_table;

    bool inside_statement_header;
    bool inside_parameter_list;
    bool inside_extern_block;
} Parser;

Parser   parser_init(Lexer *lexer);
AstFile *parse_file(Parser *parser, char *file_path);

AstIdentifier  *generate_identifier(Parser *parser, char *ident_name, Type *type, IdentifierFlags flags);
TypeStruct     *generate_struct_type_with_data_and_count(Parser *parser, Type *type_pointed_to_by_data, char *struct_name);
TypeStruct     *generate_struct_for_dynamic_array(Parser *parser, Type *type_data);
TypeStruct     *generate_struct_for_slice(Parser *parser, Type *type_data);
TypePointer    *generate_pointer_to_type(Parser *parser, Type *type);
TypeArray      *generate_slice_of(Parser *parser, Type *type);
AstIdentifier  *add_member_to_struct_scope(Parser *parser, AstStruct *struct_defn, AstIdentifier *member);
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

