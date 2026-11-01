#include <setjmp.h>
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
    bool unnamed_parameters; // The next parameter list is of a function type with only the types of its parameters e.g (T) -> U
    bool inside_argument_list;
    bool inside_extern_block;
    char *extern_library;  // The library of the extern block that is parsed, e.g "kernel32.dll". NULL for extern "C"
    bool reported_error;

    int split_shift_right_token; // Index + 1 of a '>>' token whose first '>' has been eaten as the end of a list of type arguments e.g List<List<int>>. 0 if none
    AstStruct *reparse_self_struct; // Set when parsing an instance of a generic method that was declared inside this struct

    // The prelude in std/prelude is imported by every file without an import
    char *runtime_file_path;
    char *collections_file_path;
    AstFile *collections_file; // std/prelude/collections.sd, holding List and Map. Every other file imports it
    char *slices_file_path;    // std/prelude/slices.sd, the methods on slices. Every file but itself imports it
    char *c_bindings_path;     // Absolute path of std/c. The prelude is built on these files, so they don't import it

    DynamicArray namespaces; // of *Namespace. All the namespaces found in the project and the standard library
    char *builtin_string_file_path;

    jmp_buf *syntax_error; // Where parse_file() continues after a syntax error, see stop_parsing()
    int          skipped_statements; // How many statements with syntax errors were skipped
    DynamicArray poisoned_names; // of char *. Names declared by statements with syntax errors. See is_poisoned_name()

    char *(*read_source)(const char *path); // Reads a file to parse. NULL means read_entire_file(), 'saad index --overlay' gives the text of an editor
} Parser;

Parser   parser_init(Lexer *lexer);
Namespace *find_namespace(Parser *parser, const char *name);
Namespace *add_namespace(Parser *parser, const char *name);
AstFile *parse_file(Parser *parser, char *file_path);
bool     is_poisoned_name(Parser *parser, const char *name);

AstIdentifier  *generate_identifier(Parser *parser, char *ident_name, Type *type, IdentifierFlags flags);
TypeStruct     *generate_struct_type_with_data_and_count(Parser *parser, Type *type_pointed_to_by_data, char *struct_name);
TypeStruct     *generate_struct_for_slice(Parser *parser, Type *type_data);
AstFunctionDefn *reparse_generic_function(Parser *parser, AstFunctionDefn *generic, AstBlock *scope);
bool            reparse_default_values(Parser *parser, AstDeclaration *generic_decl, AstBlock *scope, DynamicArray *values);
TypeStruct     *generate_empty_struct_type(Parser *parser, char *name);
TypePointer    *generate_pointer_to_type(Parser *parser, Type *type);
TypeArray      *generate_slice_of(Parser *parser, Type *type);
AstIdentifier  *add_member_to_struct_scope(Parser *parser, AstStruct *struct_defn, AstIdentifier *member);
void *ast_allocate(Parser *parser, size_t size);
int   align_value(int value, int alignment);
int   align_up(int value, int alignment);
bool  debug_break(Ast *site, char *filename, int line);

AstBlock *make_empty_struct_scope(Parser *parser, AstStruct *struct_defn);

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

