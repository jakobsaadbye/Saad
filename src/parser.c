#include "scope.h"

#include <stdarg.h>

#define MAX_PRECEDENCE 99
#define MIN_PRECEDENCE -99

Parser           parser_init(Lexer *lexer);
AstFile         *parse_file(Parser *parser, char *file_path);
Ast             *parse_statement(Parser *parser);
AstDeclaration  *parse_declaration(Parser *parser);
AstAssignment   *parse_assignment(Parser *parser, AstExpr *lhs, Token op_token);
AstDirective    *parse_directive(Parser *parser);
AstFunctionDefn *parse_function_defn(Parser *parser);
AstFunctionCall *parse_function_call(Parser *parser);
AstStruct       *parse_struct_defn(Parser *parser);
AstEnum         *parse_enum_defn(Parser *parser);
AstIf           *parse_if(Parser *parser);
AstFor          *parse_for(Parser *parser);
AstWhile        *parse_while(Parser *parser);
AstCast         *parse_cast(Parser *parser);
AstPrint        *parse_print(Parser *parser);
AstAssert       *parse_assert(Parser *parser);
AstNew          *parse_new(Parser *parser);
AstTypeof       *parse_typeof(Parser *parser);
AstReturn       *parse_return(Parser *parser);
Type            *parse_type(Parser *parser);
AstExpr         *parse_expression(Parser *parser, int min_prec);
AstExpr         *parse_range_or_normal_expression(Parser *parser);
AstExpr         *parse_leaf(Parser *parser);

AstDeclaration *make_declaration(Parser *parser, Token ident_token, AstExpr *expr, Type *type, DeclarationFlags flags);
AstExpr        *make_member_access(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
AstExpr        *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
AstExpr        *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type);
AstExpr        *make_literal_node(Parser *parser, Token token);
AstIdentifier  *make_identifier_from_token(Parser *parser, Token ident_token, Type *type);
AstIdentifier  *make_identifier_from_string(Parser *parser, const char *name, Type *type);

AstIdentifier  *generate_identifier(Parser *parser, char *ident_name, Type *type, IdentifierFlags flags);
TypeStruct     *generate_struct_for_dynamic_array(Parser *parser, Type *type_data);
TypeStruct     *generate_struct_for_slice(Parser *parser, Type *type_data);
TypeStruct     *generate_struct_type_with_data_and_count(Parser *parser, Type *type_data, char *struct_name);

bool starts_array_literal(Parser *parser);
bool starts_struct_literal(Parser *parser);

void *ast_allocate(Parser *parser, size_t size);
void expect(Parser *parser, Token given, TokenType expected_type);
void eat_token(Parser *parser);
void vomit_token(Parser *parser);
Token peek_token(Parser *parser, int lookahead);
Token peek_next_token(Parser *parser);
const char *label_color(const char *label);
bool is_literal(Token token);
bool is_type_specifier(Token token);
bool is_primitive_type_token(Token token);
bool is_assignment_operator(Token token);
int get_precedence(OperatorType op);
int align_value(int value, int alignment);

void report_error_range(Parser *parser, Pos start, Pos end, const char *message, ...);
void report_error_ast(Parser *parser, const char* label, Ast *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);


Parser parser_init(Lexer *lexer) {
    Parser parser = {0};
    parser.lexer         = lexer;
    parser.ast_nodes     = arena_init(4096);
    parser.type_table    = type_table_init();
    parser.enclosing_function = NULL;
    parser.current_scope = NULL;
    parser.current_file  = NULL;
    parser.inside_statement_header = false;
    parser.inside_parameter_list   = false;

    return parser;
}

void dump_file_declarations(AstFile *file) {
    printf("--------- Dump of %s ----------\n", file->absolute_path);
    for (int i = 0; i < file->scope->identifiers.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)file->scope->identifiers.items)[i];
        printf("%s:%d   %s: %s\n", file->absolute_path, ident->head.start.line, ident->name, type_to_str(ident->type));
    }
    printf("\n");
}

AstFile *parse_file(Parser *parser, char *file_path) {

    // Read in the file text
    char *file_text = read_entire_file(file_path);
    if (file_text == NULL) {
        return NULL;
    }
    
    // Tokenize the file text
    parser->lexer->file_path = file_path;
    parser->lexer->file_text = file_text;

    bool ok = lex_file(parser->lexer);
    if (!ok) {
        return NULL;
    }

    // Parse the tokens
    AstFile *file       = (AstFile *)(ast_allocate(parser, sizeof(AstFile)));
    file->head.kind     = AST_FILE;
    file->text          = file_text;
    file->absolute_path = file_path;
    file->statements    = da_init(64, sizeof(Ast *));
    file->imports       = da_init(8,  sizeof(AstImport *));
    file->scope         = new_block(parser, BLOCK_IMPERATIVE);

    parser->current_file  = file;
    parser->current_scope = file->scope;

    Token next = peek_next_token(parser);
    while (true) {
        next = peek_next_token(parser);
        if (next.type == TOKEN_END) break;

        Ast *stmt = parse_statement(parser);
        if (!stmt) return NULL;

        da_append(&file->statements, stmt);
    }

    // Eat the end token
    eat_token(parser);

    return file;
}

char *resolve_import_path(Parser *parser, char *path_string) {
    (void) parser;
    return path_string;
}

AstImport *parse_import(Parser *parser) {
    Token import_token = peek_next_token(parser);
    expect(parser, import_token, TOKEN_IMPORT);
    eat_token(parser);
    
    Token path_token = peek_next_token(parser);

    if (path_token.type != TOKEN_LITERAL_STRING) {
        report_error_token(parser, LABEL_ERROR, path_token, "Expected a string for the file or module to be imported");
        return NULL;
    }
    eat_token(parser);

    AstImport *ast_import  = ast_allocate(parser, sizeof(AstImport));
    ast_import->head.kind  = AST_IMPORT;
    ast_import->head.file  = parser->current_file;
    ast_import->head.start = import_token.start;
    ast_import->head.end   = path_token.end;
    ast_import->path_token = path_token;
    ast_import->string     = resolve_import_path(parser, ast_import->path_token.as_value.value.string.data);
    if (ast_import->string == NULL) {
        return NULL;
    }

    // Create a file object for the imported file
    // AstFile *imported_file = ast_allocate(parser, sizeof(AstFile));
    // imported_file->head.kind = AST_FILE;
    // imported_file. = AST_FILE;

    da_append(&parser->current_file->imports, ast_import);

    return ast_import;
}

AstDirective *parse_directive(Parser *parser) {
    Token next = peek_next_token(parser);
    expect(parser, next, '#');
    eat_token(parser);

    Token hash_tag = next;

    next = peek_next_token(parser);
    if (next.type != TOKEN_LITERAL_IDENTIFIER) {
        report_error_token(parser, LABEL_ERROR, next, "Syntax Error: Expected the name of a directive");
        return NULL;
    }
    eat_token(parser);

    char *dir_str = next.as_value.value.string.data;
    
    DirectiveKind dir_kind = DIRECTIVE_INVALID;    
    if (strcmp(dir_str, "import") == 0) dir_kind = DIRECTIVE_IMPORT;
    if (strcmp(dir_str, "extern") == 0) dir_kind = DIRECTIVE_EXTERN;

    if (dir_kind == DIRECTIVE_INVALID) {
        report_error_token(parser, LABEL_ERROR, next, "Syntax Error: '%s' is not the name of a directive", dir_str);
        return NULL;
    }

    AstDirective *dir = ast_allocate(parser, sizeof(AstDirective));
    dir->head.kind   = AST_DIRECTIVE;
    dir->head.file   = parser->current_file;
    dir->head.start  = hash_tag.start;
    dir->kind        = dir_kind;

    if (dir_kind == DIRECTIVE_IMPORT) {
        Token tok = peek_next_token(parser);
        if (tok.type != TOKEN_LITERAL_STRING) {
            report_error_token(parser, LABEL_ERROR, next, "Syntax Error: Expected an import string");
            return NULL;
        }
        eat_token(parser);

        char *import_name = tok.as_value.value.string.data;

        dir->import_name = import_name;
        dir->head.end    = tok.end;
    }

    if (dir_kind == DIRECTIVE_EXTERN) {
        Token tok = peek_next_token(parser);
        if (tok.type != TOKEN_LITERAL_STRING) {
            report_error_token(parser, LABEL_ERROR, next, "Syntax Error: Expected the extern ABI string. Currently only \"C\"");
            return NULL;
        }
        eat_token(parser);

        char *abi_string = tok.as_value.value.string.data;

        Abi abi = ABI_INVALID;
        if (strcmp(abi_string, "c") == 0) abi = ABI_C;
        if (strcmp(abi_string, "C") == 0) abi = ABI_C;

        if (abi == ABI_INVALID) {
            report_error_token(parser, LABEL_ERROR, tok, "Syntax Error: '%s' is not a valid abi. The following abi's are possible: [\"C\"]", abi_string);
            return NULL;
        }

        dir->extern_abi = abi;
        dir->head.end   = tok.end;
    }

    return dir;

}

AstBlock *parse_block(Parser *parser, AstBlock *inject_into_scope) {
    AstBlock *block;
    if (inject_into_scope) {
        block = inject_into_scope;
    } else {
        block = new_block(parser, BLOCK_IMPERATIVE);
    }

    Token start_token = peek_next_token(parser);
    expect(parser, start_token, '{');
    eat_token(parser);

    Token next;
    while (true) {
        next = peek_next_token(parser);
        if (next.type == '}' || next.type == TOKEN_END) break;

        Ast *stmt = parse_statement(parser);
        if (!stmt) return NULL;

        if (block->kind == BLOCK_DECLARATIVE) {
            bool valid = false;
            if (stmt->kind == AST_DECLARATION) valid = true;

            if (!valid) {
                report_error_ast(parser, LABEL_ERROR, (Ast *)stmt, "Invalid statement inside of a struct or enum. Only declarations are allowed");
                return NULL;
            }
        }

        da_append(&block->statements, stmt);
    }
    expect(parser, next, '}');
    eat_token(parser);

    block->head.kind  = AST_BLOCK;  
    block->head.file  = parser->current_file;
    block->head.start = start_token.start;
    block->head.end   = next.end;

    if (inject_into_scope) {
        // Closing the scope is done by the caller of parse_block
    } else {
        close_block(parser);
    }

    return block;
}

bool starts_declaration(Parser *parser, Token token) {
    Token next = token;
    int cursor = 0;

    while (true) {
        if (next.type == TOKEN_LITERAL_IDENTIFIER) {
            cursor += 1;
            next = peek_token(parser, cursor);

            if (next.type == ',') {
                cursor += 1;
                next = peek_token(parser, cursor);
                continue;
            }

            if (next.type == ':')                return true;
            if (next.type == TOKEN_COLON_EQUAL)  return true;
            if (next.type == TOKEN_DOUBLE_COLON) return true;

            // Check for multi declaration
            break;
        }

        break;
    }

    return false;
}

bool starts_function_defn(Parser *parser, Token token) {
    if (token.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == TOKEN_DOUBLE_COLON) {
        Token next = peek_token(parser, 2);

        if (next.type == TOKEN_METHOD) {
            next = peek_token(parser, 3);
            if (next.type == '(') {
                return true;
            }
        }
        
        else if (next.type == '(') {
            return true;
        }
    }

    return false;
}

Ast *parse_statement(Parser *parser) {
    Ast *stmt = NULL;
    bool statement_ends_with_semicolon = false;
    bool matched_a_statement = false;
    
    Token token = peek_next_token(parser);

    if (starts_function_defn(parser, token)) {
        stmt = (Ast *)(parse_function_defn(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == TOKEN_DOUBLE_COLON && peek_token(parser, 2).type == TOKEN_STRUCT) {
        stmt = (Ast *)(parse_struct_defn(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == TOKEN_DOUBLE_COLON && peek_token(parser, 2).type == TOKEN_ENUM) {
        stmt = (Ast *)(parse_enum_defn(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    else if (starts_declaration(parser, token)) {
        stmt = (Ast *)(parse_declaration(parser));
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_IMPORT) {
        stmt = (Ast *)(parse_import(parser));
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_RETURN) {
        stmt = (Ast *)(parse_return(parser));
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_BREAK || token.type == TOKEN_CONTINUE) {
        eat_token(parser);

        AstBreakOrContinue *boc = ast_allocate(parser, sizeof(AstBreakOrContinue));
        boc->head.kind          = AST_BREAK_OR_CONTINUE;
        boc->head.file          = parser->current_file;
        boc->head.start         = token.start;
        boc->head.end           = token.end;
        boc->token              = token;

        stmt = (Ast *)boc;
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_PRINT) {
        stmt = (Ast *)(parse_print(parser));
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_ASSERT) {
        stmt = (Ast *)(parse_assert(parser));
        statement_ends_with_semicolon = true;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_IF) {
        stmt = (Ast *)(parse_if(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_FOR) {
        stmt = (Ast *)(parse_for(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    else if (token.type == TOKEN_WHILE) {
        stmt = (Ast *)(parse_while(parser));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    }

    // else if (token.type == '#') {
    //     stmt = (Ast *) parse_directive(parser);
    //     matched_a_statement = true;
    // }

    else if (token.type == '{') {
        stmt = (Ast *)(parse_block(parser, NULL));
        statement_ends_with_semicolon = false;
        matched_a_statement = true;
    } 

    else {
        AstExpr *lhs = parse_expression(parser, MIN_PRECEDENCE);
        if (!lhs) {
            report_error_token(parser, LABEL_ERROR, peek_next_token(parser), "Invalid expression");
            return NULL;
        }

        Token next = peek_next_token(parser);
        if (is_assignment_operator(next)) {
            eat_token(parser);
            stmt = (Ast *)(parse_assignment(parser, lhs, next));
            if (stmt) {
                statement_ends_with_semicolon = true;
                matched_a_statement = true;
            }
        }
        else if (next.type == ';') {
            // Some expression that ends with a semi-colon.
            // The main reason why we have the AstExprStmt ast node is to allow method calls
            // to be called as an expression.
            //
            // e.g `1: foo.bar();`
            //
            // This unfortunately also opens up for other silly statements like this
            //
            // e.g `1: 5 + 3 + 2;`
            // 
            // The thing is, that there might be a sneaky function call in there
            // that have important side-effects that needs to be run.

            AstExprStmt *expr_stmt = ast_allocate(parser, sizeof(AstExprStmt));
            expr_stmt->head.head.kind  = AST_EXPR_STMT;
            expr_stmt->head.head.file  = parser->current_file;
            expr_stmt->head.head.start = lhs->head.start;
            expr_stmt->head.head.end   = next.end;
            expr_stmt->expr            = lhs;


            stmt = (Ast*) expr_stmt;
            matched_a_statement = true;
            statement_ends_with_semicolon = true;
        }
    }

    if (!matched_a_statement) {
        report_error_token(parser, LABEL_ERROR, token, "Invalid start of statement");
        return NULL;
    }
    if (!stmt && matched_a_statement) {
        // Error is already reported in the matched statement
        return NULL;
    }

    if (statement_ends_with_semicolon) {
        Token next = peek_next_token(parser);
        if (next.type != ';') {
            Token token_missing_semicolon = peek_token(parser, -2);
            report_error_range(parser, token_missing_semicolon.end, token_missing_semicolon.end, "Syntax Error: Expected a semi-colon");
            exit(1);
        }
        eat_token(parser);
    }

    return stmt;
}

AstLiteral *int_literal_from_value(Parser *parser, int value) {
    AstLiteral *lit     = (AstLiteral *)(ast_allocate(parser, sizeof(AstLiteral)));
    lit->head.head.kind = AST_LITERAL;
    lit->head.head.file = parser->current_file;
    lit->kind           = (LiteralKind)(TOKEN_LITERAL_INTEGER);
    lit->as.value.integer = value;
    lit->as.flags         = 0;
    return lit;
}

bool compare_enumerator(const void *key, const void *item) {
    AstEnumerator *etor = (AstEnumerator *)(item);
    return strcmp(key, etor->name) == 0;
}

TypeStruct *generate_empty_struct_type(Parser *parser, char *name) {
    AstStruct *struct_node       = ast_allocate(parser, sizeof(AstStruct));
    struct_node->head.kind       = AST_STRUCT;
    struct_node->head.file       = parser->current_file;
    struct_node->head.flags      = AST_FLAG_COMPILER_GENERATED;

    struct_node->scope           = ast_allocate(parser, sizeof(AstBlock));
    struct_node->scope->belongs_to_struct = struct_node;
    struct_node->scope->kind     = BLOCK_DECLARATIVE;
    struct_node->scope->identifiers = da_init(2, sizeof(AstIdentifier *));

    TypeStruct *struct_defn      = ast_allocate(parser, sizeof(TypeStruct));
    struct_defn->head.head.kind  = AST_TYPE;
    struct_defn->head.head.file  = parser->current_file;
    struct_defn->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    struct_defn->head.head.start = (Pos){0, 0, 0};
    struct_defn->head.head.end   = (Pos){0, 0, 0};
    struct_defn->head.kind       = TYPE_STRUCT;
    struct_defn->head.flags      = TYPE_IS_FULLY_SIZED;
    struct_defn->head.name       = name;
    struct_defn->head.size       = 0; // This will be calculated when adding new members
    struct_defn->alignment       = 0; // This will be calculated when adding new members
    struct_defn->node            = struct_node;

    struct_node->identifier      = generate_identifier(parser, name, (Type *)struct_defn, IDENTIFIER_IS_NAME_OF_STRUCT);

    return struct_defn;
}

void add_struct_member(Parser *parser, TypeStruct *struct_defn, char *name, Type *type) {
    AstIdentifier *member = generate_identifier(parser, name, type, IDENTIFIER_IS_STRUCT_MEMBER);

    DynamicArray members = struct_defn->node->scope->identifiers;

    if (members.count == 0) {
        member->member_index  = 0;
        member->member_offset = 0;
    } else {
        AstIdentifier *prev_member = ((AstIdentifier **)members.items)[members.count - 1];
        member->member_index  = prev_member->member_index + 1;
        member->member_offset = prev_member->member_offset + member->type->size;
    }

    add_identifier_to_scope(parser, struct_defn->node->scope, member);

    // Alignment + C-style sizing
    struct_defn->head.size += member->type->size;
    if (member->type->size >= 8) {
        struct_defn->alignment = 8;
    } else {
        if (member->type->size > (int)struct_defn->alignment) {
            struct_defn->alignment = member->type->size;
        }
    }
    struct_defn->head.size = align_value(struct_defn->head.size, struct_defn->alignment);

}

TypeStruct *generate_struct_for_any_type(Parser *parser) {

    // We generate the following two structs for the any type
    //
    // Type :: struct {
    //     kind:  TypeKind;
    //     size:  int;
    //     flags: int;
    // }

    // TypeAny :: struct {
    //     value: *void;
    //     type:  *Type;
    // }

    // Generate a type for Type
    TypeStruct *type_defn = generate_empty_struct_type(parser, "Type");
    add_struct_member(parser, type_defn, "kind", primitive_type(PRIMITIVE_INT)); // @Todo: Change this type to an enum
    add_struct_member(parser, type_defn, "size", primitive_type(PRIMITIVE_INT));
    add_struct_member(parser, type_defn, "flags", primitive_type(PRIMITIVE_INT));

    TypePointer *ptr_type_defn = generate_pointer_to_type(parser, (Type *) type_defn);

    // Generate the TypeAny type
    TypeStruct *any_defn = generate_empty_struct_type(parser, "any");
    add_struct_member(parser, any_defn, "data", (Type *) type_null_ptr);
    add_struct_member(parser, any_defn, "type", (Type *) ptr_type_defn);

    return any_defn;
}

Type *parse_type(Parser *parser) {
    Token next = peek_next_token(parser);

    if (is_primitive_type_token(next)) {
        eat_token(parser);
        TypePrimitive *ti = type_alloc(&parser->type_table, sizeof(TypePrimitive));
        TypePrimitive *primitive = &primitive_types[(next.type - TOKEN_TYPE_UINT) + 1]; // +1 to skip over invalid kind
        memcpy(ti, primitive, sizeof(TypePrimitive));
        ti->head.head.kind  = AST_TYPE;
        ti->head.head.file  = parser->current_file;
        ti->head.head.start = next.start;
        ti->head.head.end   = next.end;
        return (Type *)(ti);
    }

    if (next.type == TOKEN_ANY) {
        eat_token(parser);
        Token any_token = next;

        TypeAny *any = ast_allocate(parser, sizeof(TypeAny));
        any->head.head.kind  = AST_TYPE;
        any->head.head.file  = parser->current_file;
        any->head.head.start = any_token.start;
        any->head.head.end   = any_token.end;
        any->head.kind       = TYPE_ANY;
        any->head.size       = 16;
        any->struct_defn     = generate_struct_for_any_type(parser);

        return (Type *)(any);
    }

    if (next.type == TOKEN_LITERAL_IDENTIFIER) {
        // Struct or enum
        eat_token(parser);

        Type *ti   = type_alloc(&parser->type_table, sizeof(Type));
        ti->head.kind  = AST_TYPE;
        ti->head.file  = parser->current_file;
        ti->head.start = next.start;
        ti->head.end   = next.end;
        ti->kind       = TYPE_NAME;
        ti->name       = next.as_value.value.identifier.name;
        return ti;
    }

    if (next.type == TOKEN_TRIPLE_DOT) {
        // Variadic parameter
        eat_token(parser);
        Token dots = next;

        Type *type = parse_type(parser);
        if (!type) return NULL;

        TypeVariadic *variadic = type_alloc(&parser->type_table, sizeof(TypeVariadic));
        variadic->head.head.kind  = AST_TYPE;
        variadic->head.head.file  = parser->current_file;
        variadic->head.head.start = dots.start;
        variadic->head.head.end   = type->head.end;
        variadic->head.kind       = TYPE_VARIADIC;
        variadic->head.size       = 16;
        variadic->type            = type;
        variadic->slice           = generate_slice_of(parser, type);

        if (!parser->inside_parameter_list) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)variadic, "Variadic types are only allowed inside a functions parameter list");
            return NULL;
        }

        return (Type *)variadic;
    }

    if (next.type == '*') {
        // Pointer
        Token asterix = next;
        eat_token(parser);

        TypePointer *ptr = type_alloc(&parser->type_table, sizeof(TypePointer));
        
        Type *points_to = parse_type(parser);
        if (!points_to) return NULL;

        ptr->head.head.kind  = AST_TYPE;
        ptr->head.head.file  = parser->current_file;
        ptr->head.head.start = asterix.start;
        ptr->head.head.end   = points_to->head.end;
        ptr->head.kind       = TYPE_POINTER;
        ptr->head.size       = 8;
        ptr->pointer_to      = points_to;

        return (Type *)(ptr);
    }

    if (next.type == '[') {
        // Array
        eat_token(parser);

        Token open_bracket = next;

        AstExpr *capacity_expr = NULL;
        ArrayKind array_kind;

        next = peek_next_token(parser);
        if (next.type == TOKEN_DOUBLE_DOT) {
            eat_token(parser);
            array_kind = ARRAY_DYNAMIC;
            capacity_expr = NULL;
        } else if (next.type == ']') {
            array_kind = ARRAY_SLICE;
            capacity_expr = NULL;
        } else {
            array_kind = ARRAY_FIXED;
            capacity_expr = parse_expression(parser, MIN_PRECEDENCE);
            if (!capacity_expr) return NULL;
        }

        next = peek_next_token(parser);
        if (next.type != ']') {
            report_error_token(parser, LABEL_ERROR, open_bracket, "Expected a closing bracket to match this start bracket here");
            return NULL;
        }
        eat_token(parser);

        Type *elem_type = parse_type(parser);
        if (!elem_type) return NULL;

        // Generate a small struct definition for the array
        TypeStruct *struct_defn = NULL;

        if (array_kind == ARRAY_DYNAMIC) {
            struct_defn = generate_struct_for_dynamic_array(parser, elem_type);
        } else {
            struct_defn = generate_struct_for_slice(parser, elem_type);
        }


        // The actual array type
        TypeArray *array       = ast_allocate(parser, sizeof(TypeArray));
        array->head.head.kind  = AST_TYPE;
        array->head.head.file  = parser->current_file;
        array->head.head.start = open_bracket.start;
        array->head.head.end   = elem_type->head.end;
        array->head.kind       = TYPE_ARRAY;
        array->head.size       = 0; // Size is set later in resolve_type()
        array->array_kind      = array_kind;
        array->elem_type       = elem_type;
        array->struct_defn     = struct_defn;
        array->capacity_expr   = capacity_expr;
        array->capacity        = 0; // Set in typer
        array->count           = 0; // Set in typer

        return (Type *)(array);
    }   

    report_error_token(parser, LABEL_ERROR, next, "Expected a type");
    return NULL;
}

AstIdentifier *add_member_to_struct_scope(Parser *parser, AstStruct *struct_defn, AstIdentifier *member) {
    return add_identifier_to_scope(parser, struct_defn->scope, member);
}

TypePointer *generate_pointer_to_type(Parser *parser, Type *type) {
    TypePointer *ptr     = ast_allocate(parser, sizeof(TypePointer));
    ptr->head.head.kind  = AST_TYPE;
    ptr->head.head.start = type->head.start;
    ptr->head.head.end   = type->head.end;
    ptr->head.kind       = TYPE_POINTER;
    ptr->head.size       = 8;
    ptr->pointer_to      = type;    
    return ptr;
}

TypeArray *generate_slice_of(Parser *parser, Type *type) {
    TypeArray *slice = ast_allocate(parser, sizeof(TypeArray));
    slice->head.head.kind  = AST_TYPE;
    slice->head.head.file  = type->head.file;
    slice->head.head.start = type->head.start;
    slice->head.head.end   = type->head.end;
    slice->head.head.flags = AST_FLAG_COMPILER_GENERATED | AST_FLAG_IS_TYPE_CHECKED;
    slice->head.size       = 16;
    slice->head.kind       = TYPE_ARRAY;
    slice->array_kind      = ARRAY_SLICE;
    slice->elem_type       = type;
    slice->node            = NULL;
    slice->struct_defn     = generate_struct_for_slice(parser, type);

    return slice;
}

TypeStruct *generate_struct_for_slice(Parser *parser, Type *type_data) {
    TypeStruct *slice = generate_struct_type_with_data_and_count(parser, type_data, "slice");
    return slice;
}

TypeStruct *generate_struct_for_dynamic_array(Parser *parser, Type *type_data) {
    TypeStruct *dynamic_array = generate_struct_type_with_data_and_count(parser, type_data, "dynamic array");

    dynamic_array->head.size = 24;

    // Add .capacity for the runtime capacity of the array
    AstIdentifier *capacity = generate_identifier(parser, "capacity", primitive_type(PRIMITIVE_S64), IDENTIFIER_IS_STRUCT_MEMBER);

    capacity->member_index   = 2;
    capacity->member_offset  = 16;

    add_member_to_struct_scope(parser, dynamic_array->node, capacity);

    return dynamic_array;
}

TypeStruct *generate_struct_type_with_data_and_count(Parser *parser, Type *type_data, char *struct_name) {
    AstStruct *struct_node       = ast_allocate(parser, sizeof(AstStruct));
    struct_node->head.kind       = AST_STRUCT;
    struct_node->head.file       = parser->current_file;
    struct_node->head.flags      = AST_FLAG_COMPILER_GENERATED;
    struct_node->identifier      = NULL;

    struct_node->scope           = ast_allocate(parser, sizeof(AstBlock));
    struct_node->scope->belongs_to_struct = struct_node;
    struct_node->scope->kind     = BLOCK_DECLARATIVE;
    struct_node->scope->identifiers = da_init(2, sizeof(AstIdentifier *));

    TypeStruct *struct_defn      = ast_allocate(parser, sizeof(TypeStruct));
    struct_defn->head.head.kind  = AST_TYPE;
    struct_defn->head.head.file  = parser->current_file;
    struct_defn->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    struct_defn->head.head.start = type_data->head.start;
    struct_defn->head.head.end   = type_data->head.end;
    struct_defn->head.kind       = TYPE_STRUCT;
    struct_defn->head.name       = struct_name;
    struct_defn->head.size       = 16;
    struct_defn->alignment       = 8;
    struct_defn->node            = struct_node;

    TypePointer *type_ptr_data     = ast_allocate(parser, sizeof(TypePointer));
    type_ptr_data->head.head.kind  = AST_TYPE;
    type_ptr_data->head.head.file  = parser->current_file;
    type_ptr_data->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    type_ptr_data->head.head.start = type_data->head.start;
    type_ptr_data->head.head.end   = type_data->head.end;
    type_ptr_data->head.kind       = TYPE_POINTER;
    type_ptr_data->head.size       = 8;
    type_ptr_data->pointer_to      = type_data;

    AstIdentifier *data  = generate_identifier(parser, "data", (Type *) type_ptr_data,         IDENTIFIER_IS_STRUCT_MEMBER);
    AstIdentifier *count = generate_identifier(parser, "count", primitive_type(PRIMITIVE_S64), IDENTIFIER_IS_STRUCT_MEMBER);

    data->member_index   = 0;
    data->member_offset  = 0;
    count->member_index  = 1;
    count->member_offset = 8;

    add_member_to_struct_scope(parser, struct_node, data);
    add_member_to_struct_scope(parser, struct_node, count);

    return struct_defn;
}

AstEnum *parse_enum_defn(Parser *parser) {
    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_LITERAL_IDENTIFIER);
    eat_token(parser);
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL); // Type of ident is set later down to refer to the enum it self

    // Expose the enum as an identifier so enum values can be referenced explicitly using ENUM.value
    // @ScopeRefactoring
    {
        ident->flags |= IDENTIFIER_IS_NAME_OF_ENUM;
        AstIdentifier *existing = add_identifier_to_scope(parser, parser->current_scope, ident);
        if (existing) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)(ident), "Type '%s' already defined", existing->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)(existing), "Here is the previous definition of '%s'", existing->name);
            return NULL;
        }
    }


    Token next = peek_next_token(parser);
    expect(parser, next, TOKEN_DOUBLE_COLON);
    eat_token(parser);

    Token enum_token = peek_next_token(parser);
    expect(parser, enum_token, TOKEN_ENUM);
    eat_token(parser);

    next = peek_next_token(parser);
    expect(parser, next, '{');
    eat_token(parser);

    AstEnum *ast_enum = (AstEnum *)(ast_allocate(parser, sizeof(AstEnum)));
    ast_enum->enumerators = da_init(2, sizeof(AstEnumerator *));

    next = peek_next_token(parser);
    int enum_index = 0;
    while (next.type != '}' && next.type != TOKEN_END) {
        if (next.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == '=') {
            // Explicit value
            eat_token(parser);
            eat_token(parser);

            AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
            if (!expr) return NULL;

            AstEnumerator *etor = (AstEnumerator *)(ast_allocate(parser, sizeof(AstEnumerator)));
            etor->head.kind  = AST_ENUMERATOR;
            etor->head.file  = parser->current_file;
            etor->head.start = next.start;
            etor->head.end   = next.end;
            etor->parent     = ast_enum;
            etor->name       = next.as_value.value.identifier.name; // @Robustness - Should probably do a copy here so that we are not tied to the token array
            etor->expr       = expr;
            etor->index      = enum_index;

            da_append(&ast_enum->enumerators, etor);

            enum_index++;
        } else if (next.type == TOKEN_LITERAL_IDENTIFIER) {
            // Auto-increment
            eat_token(parser);

            AstEnumerator *etor = (AstEnumerator *)(ast_allocate(parser, sizeof(AstEnumerator)));
            etor->head.kind  = AST_ENUMERATOR;
            etor->head.file  = parser->current_file;
            etor->head.start = next.start;
            etor->head.end   = next.end;
            etor->parent     = ast_enum;
            etor->name       = next.as_value.value.identifier.name; // @Robustness - Should probably do a copy here so that we are not tied to the token array
            etor->expr       = NULL;
            etor->index      = enum_index;

            da_append(&ast_enum->enumerators, etor);

            enum_index++;
        } else {
            report_error_token(parser, LABEL_ERROR, next, "Expected name of the enumerator");
            exit(1);
        }

        next = peek_next_token(parser);
        if (next.type == ',' && peek_token(parser, 1).type == '}') {
            // Allow trailing comma
            eat_token(parser);
            next = peek_next_token(parser);
        } else if (next.type == '}') {
            break; 
        } else if (next.type == ',') {
            // Fallthrough and parse the next
            eat_token(parser);
            next = peek_next_token(parser);
        } else {
            Token token_missing_comma = peek_token(parser, -1);
            report_error_range(parser, token_missing_comma.end, token_missing_comma.end, "Expected a comma");
            exit(1);
        }
    }
    expect(parser, next, '}');
    eat_token(parser);

    ast_enum->head.kind  = AST_ENUM;
    ast_enum->head.file  = parser->current_file;
    ast_enum->head.start = ident->head.start;
    ast_enum->head.end   = peek_token(parser, -1).end;
    ast_enum->identifier = ident;

    TypeEnum *type_enum        = type_alloc(&parser->type_table, sizeof(TypeEnum));
    type_enum->head.head.kind  = AST_TYPE;
    type_enum->head.head.file  = parser->current_file;
    type_enum->head.head.start = ast_enum->head.start;
    type_enum->head.head.end   = ast_enum->head.end;
    type_enum->head.kind       = TYPE_ENUM;
    type_enum->head.name       = ident->name;
    type_enum->node            = ast_enum; // @Improvement - Probably also need to copy over the hashtable of enumerators
    type_enum->identifier      = ast_enum->identifier;
    type_enum->backing_type    = primitive_type(PRIMITIVE_INT); // @Improvement - Support for having a backing integer type

    Type *exists = type_add_user_defined(&parser->type_table, (Type *)(type_enum));
    assert(!exists);

    ident->type = (Type *)(type_enum);

    return ast_enum;
}

AstStruct *parse_struct_defn(Parser *parser) {
    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_LITERAL_IDENTIFIER);
    eat_token(parser);
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL);

    AstIdentifier *existing_ident = add_identifier_to_scope(parser, parser->current_scope, ident);
    if (existing_ident) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(ident), "Redeclaration of identifier '%s'", existing_ident->name);
        report_error_ast(parser, LABEL_NOTE, (Ast *)(existing_ident), "Here is the previous declaration of '%s'", existing_ident->name);
        return NULL;
    }

    Token next = peek_next_token(parser);
    expect(parser, next, TOKEN_DOUBLE_COLON);
    eat_token(parser);

    Token struct_token = peek_next_token(parser);
    expect(parser, struct_token, TOKEN_STRUCT);
    eat_token(parser);

    AstStruct *ast_struct = (AstStruct *)(ast_allocate(parser, sizeof(AstStruct)));
    AstBlock *scope = new_block(parser, BLOCK_DECLARATIVE);
    scope->belongs_to_struct = ast_struct;

    scope = parse_block(parser, scope);
    if (!scope) return NULL;
    close_block(parser);

    ast_struct->head.kind  = AST_STRUCT;
    ast_struct->head.file  = parser->current_file;
    ast_struct->head.start = ident_token.start;
    ast_struct->head.end   = scope->head.end;
    ast_struct->identifier = ident;
    ast_struct->scope      = scope;

    if (scope->identifiers.count == 0) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(ast_struct), "Structs must have atleast one member");
        return NULL;
    }

    TypeStruct *type_struct      = type_alloc(&parser->type_table, sizeof(TypeStruct));
    type_struct->head.head.kind  = AST_TYPE;
    type_struct->head.head.file  = parser->current_file;
    type_struct->head.head.start = ast_struct->head.start;
    type_struct->head.head.end   = ast_struct->head.end;
    type_struct->head.kind       = TYPE_STRUCT;
    type_struct->head.name       = ident->name;
    type_struct->identifier      = ident;
    type_struct->node            = ast_struct;

    type_add_user_defined(&parser->type_table, (Type *)(type_struct));

    ident->type = (Type *)type_struct;

    // if (existing) {
    //     if (existing->kind == TYPE_STRUCT) {
    //         TypeStruct *existing_struct = (TypeStruct *)(existing);
    //         report_error_ast(parser, LABEL_ERROR, (Ast *)(ast_struct->identifier), "Struct '%s' is already defined", ast_struct->identifier->name);
    //         report_error_ast(parser, LABEL_NOTE, (Ast *)(existing_struct->node), "Here is the previously defined struct");
    //         return NULL;
    //     } else {
    //         TypeStruct *existing_enum = (TypeStruct *)(existing);
    //         report_error_ast(parser, LABEL_ERROR, (Ast *)(ast_struct->identifier), "Struct '%s' is already defined as an enum", ast_struct->identifier->name);
    //         report_error_ast(parser, LABEL_NOTE, (Ast *)(existing_enum->node), "Here is the previously defined enum");
    //         return NULL;
    //     }
    // }
    

    return ast_struct;
}

AstAssignment *parse_assignment(Parser *parser, AstExpr *lhs, Token op_token) {
    assert(is_assignment_operator(op_token));

    bool valid_lhs = false;
    if (lhs->head.kind == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) 
        valid_lhs = true;
    if (lhs->head.kind == AST_MEMBER_ACCESS) 
        valid_lhs = true;
    if (lhs->head.kind == AST_ARRAY_ACCESS) 
        valid_lhs = true;
    if (lhs->head.kind == AST_UNARY) 
        valid_lhs = true;

    if (!valid_lhs) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(lhs), "Invalid expression as left-hand side of assignment");
        return NULL;
    }

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!expr) return NULL;

    AstAssignment *assign = (AstAssignment *)(ast_allocate(parser, sizeof(AstAssignment)));
    assign->head.kind     = AST_ASSIGNMENT;
    assign->head.file     = parser->current_file;
    assign->head.start    = lhs->head.start;
    assign->head.end      = expr->head.end;
    assign->lhs           = lhs;
    assign->op            = (AssignOp)(op_token.type);
    assign->expr          = expr;

    return assign;
}

AstCast *parse_cast(Parser *parser) {
    Token next = peek_next_token(parser);
    assert(next.type == TOKEN_XX || next.type == TOKEN_CAST);
    eat_token(parser);

    Token cast_token = next;

    if (cast_token.type == TOKEN_XX) {
        int precedence = get_precedence((OperatorType) TOKEN_CAST);
        AstExpr *expr = parse_expression(parser, precedence);
        if (!expr) return NULL;

        AstCast *cast         = ast_allocate(parser, sizeof(AstCast));
        cast->head.head.kind  = AST_CAST;
        cast->head.head.file  = parser->current_file;
        cast->head.head.start = cast_token.start;
        cast->head.head.end   = expr->head.end;
        cast->expr            = expr;
        cast->cast_to         = NULL;
        cast->is_auto         = true;

        return cast;
    } else {
        next = peek_next_token(parser);
        expect(parser, next, '(');
        eat_token(parser);

        Type *cast_to = parse_type(parser);
        if (!cast_to) return NULL;

        next = peek_next_token(parser);
        expect(parser, next, ')');
        eat_token(parser);

        int precedence = get_precedence((OperatorType) TOKEN_CAST);
        AstExpr *expr = parse_expression(parser, precedence);
        if (!expr) return NULL;

        AstCast *cast         = ast_allocate(parser, sizeof(AstCast));
        cast->head.head.kind  = AST_CAST;
        cast->head.head.file  = parser->current_file;
        cast->head.head.start = cast_token.start;
        cast->head.head.end   = expr->head.end;
        cast->expr            = expr;
        cast->cast_to         = cast_to;
        cast->is_auto         = false;

        return cast;
    }
}

AstFor *parse_for(Parser *parser) {
    ForKind kind = 0;
    AstIdentifier *index = NULL;
    AstIdentifier *iterator = NULL;
    AstExpr       *iterable = NULL;
    bool           is_by_pointer = false;
    Token          asterix = {0};

    Token for_token = peek_next_token(parser);
    eat_token(parser);

    parser->inside_statement_header = true;

    Token next = peek_next_token(parser);

    if (next.type == '*') {
        is_by_pointer = true;
        asterix = next;
        eat_token(parser);
        next = peek_next_token(parser);
    }

    // `for x in xs {...}`
    if (next.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == TOKEN_IN) {
        iterator = make_identifier_from_token(parser, next, NULL);

        eat_token(parser);
        eat_token(parser);

        iterable = parse_range_or_normal_expression(parser);
        if (!iterable) return NULL;

        kind = FOR_WITH_NAMED_ITERATOR;
    }

    // `for x, i in xs {...}`
    // `for x, i in 10..20 {...}`
    else if (next.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == ',') {
        iterator = make_identifier_from_token(parser, next, NULL);
        eat_token(parser);
        eat_token(parser);
        
        next = peek_next_token(parser);
        if (next.type != TOKEN_LITERAL_IDENTIFIER) {
            report_error_token(parser, LABEL_ERROR, next, "Expected an identifier for the index");
            return NULL;
        }
        eat_token(parser);
        index = make_identifier_from_token(parser, next, NULL);

        next = peek_next_token(parser);
        if (next.type != TOKEN_IN) {
            report_error_token(parser, LABEL_ERROR, next, "Expected the 'in' keyword here");
            return NULL;
        }
        eat_token(parser);

        iterable = parse_range_or_normal_expression(parser);
        if (!iterable) return NULL;
        
        kind = FOR_WITH_NAMED_ITERATOR_AND_INDEX;
    }
    // `for {...}`
    else if (next.type == '{') {
        iterator = NULL;
        iterable = NULL;
        kind = FOR_INFINITY_AND_BEYOND;
    }
    // `for xs {...}` (implicit 'it')
    else {
        iterable = parse_range_or_normal_expression(parser);
        if (!iterable) return NULL;

        Token next = peek_next_token(parser);
        if (next.type != '{' ) {
            report_error_range(parser, iterable->head.start, next.end, "Syntax Error: Invalid for-expression");
            return NULL;
        }

        iterator = make_identifier_from_string(parser, "it", NULL); // type is set later
        kind = FOR_WITH_EXPR;
    }

    parser->inside_statement_header = false;
    

    AstBlock *body;
    if (iterator || index) {
        // Push down the iterator into the scope of the body
        body = new_block(parser, BLOCK_IMPERATIVE);
        if (iterator) add_identifier_to_scope(parser, body, iterator);
        if (index)    add_identifier_to_scope(parser, body, index);
        
        body = parse_block(parser, body);
        if (!body) return NULL;
        close_block(parser);
    } else {
        body = parse_block(parser, NULL);
        if (!body) return NULL;
    }

    AstFor *ast_for = (AstFor *)(ast_allocate(parser, sizeof(AstFor)));
    ast_for->head.kind = AST_FOR;
    ast_for->head.file = parser->current_file;
    ast_for->head.start = for_token.start;
    ast_for->head.end = body->head.end;
    ast_for->kind = kind;
    ast_for->iterator = iterator;
    ast_for->index = index;
    ast_for->iterable = iterable;
    ast_for->body = body;
    ast_for->is_by_pointer = is_by_pointer;
    ast_for->asterix = asterix;

    return ast_for;
}

AstWhile *parse_while(Parser *parser) {
    Token while_token = peek_next_token(parser);
    eat_token(parser);
    assert(while_token.type == TOKEN_WHILE);

    parser->inside_statement_header = true;
    AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);
    if (!condition) return NULL;
    parser->inside_statement_header = false;
    
    AstBlock *body = parse_block(parser, NULL);
    if (!body) return NULL;

    AstWhile *ast_while = ast_allocate(parser, sizeof(AstWhile));
    ast_while->head.kind = AST_WHILE;
    ast_while->head.file = parser->current_file;
    ast_while->head.start = while_token.start;
    ast_while->head.end = body->head.end;
    ast_while->condition = condition;
    ast_while->body = body;
    ast_while->condition_label = 0;
    ast_while->done_label = 0;

    return ast_while;
}

AstExpr *parse_range_or_normal_expression(Parser *parser) {
    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!expr) return NULL;

    Token next = peek_next_token(parser);
    if (next.type == TOKEN_DOUBLE_DOT || next.type == TOKEN_TRIPLE_DOT) {
        eat_token(parser);
        AstExpr *end = parse_expression(parser, MIN_PRECEDENCE);
        if (!end) return NULL;

        AstRangeExpr *range_expr = (AstRangeExpr *)(ast_allocate(parser, sizeof(AstRangeExpr)));            
        range_expr->head.head.kind = AST_RANGE_EXPR;
        range_expr->head.head.file = parser->current_file;
        range_expr->head.head.start = expr->head.start;
        range_expr->head.head.end = end->head.end;
        range_expr->start = expr;
        range_expr->end = end;
        range_expr->inclusive = next.type == TOKEN_TRIPLE_DOT;

        return (AstExpr *)(range_expr);
    } else {
        return expr;
    }
}

AstReturn *parse_return(Parser *parser) {
    assert(parser->enclosing_function);

    AstReturn *ast_return = (AstReturn *)(ast_allocate(parser, sizeof(AstReturn)));
    ast_return->values = da_init(2, sizeof(AstExpr *));

    Token return_token = peek_next_token(parser);
    eat_token(parser);

    while (true) {
        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
        if (!expr) return NULL;

        da_append(&ast_return->values, expr);

        Token next = peek_next_token(parser);
        if (next.type == ',') {
            eat_token(parser);
            continue;
        } else {
            break;
        }
    }
    
    ast_return->head.kind  = AST_RETURN;
    ast_return->head.file  = parser->current_file;
    ast_return->head.start = return_token.start;
    ast_return->head.end   = ((AstExpr **)ast_return->values.items)[ast_return->values.count - 1]->head.end; // last return value
    ast_return->enclosing_function = parser->enclosing_function;

    return ast_return;
}

AstStructLiteral *parse_struct_literal(Parser *parser) {
    Type *explicit_type = NULL;
    Token start_token = peek_next_token(parser);
    if (start_token.type == TOKEN_LITERAL_IDENTIFIER) {
        explicit_type = parse_type(parser);
    }

    Token next = peek_next_token(parser);
    expect(parser, next, '{');
    eat_token(parser);

    AstStructLiteral *struct_literal = ast_allocate(parser, sizeof(AstStructLiteral));
    struct_literal->initializers     = da_init(2, sizeof(AstStructInitializer*));

    // Parse intializers
    next = peek_next_token(parser);
    AstIdentifier *designator = NULL;
    AstExpr *value            = NULL;
    while (next.type != '}' && next.type != TOKEN_END) {
        if (next.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == '=') {
            eat_token(parser);
            eat_token(parser);

            designator = make_identifier_from_token(parser, next, NULL);
            value      = parse_expression(parser, MIN_PRECEDENCE);
            if (!value) return NULL;
        } else {
            designator = NULL;
            value      = parse_expression(parser, MIN_PRECEDENCE);
            if (!value) return NULL;
        }

        AstStructInitializer *init = ast_allocate(parser, sizeof(AstStructInitializer));
        init->head.kind  = AST_STRUCT_INITIALIZER;
        init->head.file  = parser->current_file;
        init->head.start = designator != NULL ? designator->head.start : value->head.start;
        init->head.end   = value->head.end;
        init->designator = designator;
        init->value      = value;
        da_append(&struct_literal->initializers, init);


        next = peek_next_token(parser);
        if (next.type == ',' && peek_token(parser, 1).type == '}') {
            // Allow trailing comma
            eat_token(parser);
        } else if (next.type == '}') {
            // Reached end
            break;
        } else {
            expect(parser, next, ',');
            eat_token(parser);
        }
        next = peek_next_token(parser);
    }
    expect(parser, next, '}');
    eat_token(parser);

    struct_literal->head.head.kind  = AST_STRUCT_LITERAL;
    struct_literal->head.head.file  = parser->current_file;
    struct_literal->head.head.start = start_token.start;
    struct_literal->head.head.end   = next.end;
    struct_literal->explicit_type   = explicit_type;

    return struct_literal;
}

AstArrayLiteral *parse_array_literal(Parser *parser) {
    Token open_bracket = peek_next_token(parser);
    assert(open_bracket.type == '[');
    eat_token(parser);

    AstArrayLiteral *array_lit = ast_allocate(parser, sizeof(AstArrayLiteral));
    array_lit->expressions     = da_init(4, sizeof(AstExpr *));

    Token next = peek_next_token(parser);
    while (next.type != ']' && next.type != TOKEN_END) {
        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
        if (!expr) return NULL;

        da_append(&array_lit->expressions, expr);

        next = peek_next_token(parser);
        if (next.type != ']' && next.type != ',') {
            report_error_token(parser, LABEL_ERROR, next, "Expected a comma here");
            exit(1);
        }
        if (next.type == ',') {
            eat_token(parser);
            next = peek_next_token(parser);
            if (next.type == ']') break;
        }

    }
    expect(parser, next, ']');
    eat_token(parser);

    Token close_bracket = next;

    array_lit->head.head.kind  = AST_ARRAY_LITERAL;
    array_lit->head.head.file  = parser->current_file;
    array_lit->head.head.start = open_bracket.start;
    array_lit->head.head.end   = close_bracket.end;

    return array_lit;
}

AstExpr *parse_array_access(Parser *parser, Token open_bracket, AstExpr *left) {
    assert(open_bracket.type == '[');

    AstExpr *index_expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!index_expr) return NULL;

    Token next = peek_next_token(parser);
    if (next.type != ']') {
        report_error_token(parser, LABEL_ERROR, next, "Expected a closing bracket here");
        report_error_token(parser, LABEL_NOTE, open_bracket, "For this open bracket ...");
        return NULL;
    }
    eat_token(parser);

    AstArrayAccess *array_ac      = ast_allocate(parser, sizeof(AstArrayAccess));
    array_ac->head.head.kind      = AST_ARRAY_ACCESS;
    array_ac->head.head.file      = parser->current_file;
    array_ac->head.head.start     = left->head.start;
    array_ac->head.head.end       = next.end;
    array_ac->accessing           = left;
    array_ac->index_expr          = index_expr;
    array_ac->open_bracket        = open_bracket;
    array_ac->close_bracket       = next;

    return (AstExpr *)(array_ac);
}

AstFunctionCall *parse_function_call(Parser *parser) {
    AstFunctionCall *call = (AstFunctionCall *)(ast_allocate(parser, sizeof(AstFunctionCall)));
    call->arguments       = da_init(2, sizeof(AstExpr *));

    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_LITERAL_IDENTIFIER); // Should be impossible to fail
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '('); // Should also be impossible to fail
    call->paren_start_token = next;
    eat_token(parser);

    bool first_argument_seen = false;
    while (true) {
        //
        // Parse argument list
        //
        next = peek_next_token(parser);
        if (next.type == ')') {
            eat_token(parser);
            break;
        }

        if (first_argument_seen) {
            if (next.type != ',') {
                report_error_token(parser, LABEL_ERROR, next, "Expected a ',' between arguments");
                exit(1);    
            }
            eat_token(parser);
            next = peek_next_token(parser);
        }

        AstExpr *arg = parse_expression(parser, MIN_PRECEDENCE);
        if (!arg) return NULL;

        da_append(&call->arguments, arg);
        first_argument_seen = true;
    }

    call->head.head.kind  = AST_FUNCTION_CALL;
    call->head.head.file  = parser->current_file;
    call->head.head.start = ident_token.start;
    call->head.head.end   = next.end;
    call->identifer       = make_identifier_from_token(parser, ident_token, NULL); // The type of the identifier will be set later in the typer, so here i just specify void

    return call;
}

AstFunctionDefn *parse_function_defn(Parser *parser) {
    AstFunctionDefn *func_defn = (AstFunctionDefn *)(ast_allocate(parser, sizeof(AstFunctionDefn)));
    func_defn->parameters   = da_init(2, sizeof(AstIdentifier *));
    func_defn->return_types = da_init(2, sizeof(Type *));

    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_LITERAL_IDENTIFIER);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, TOKEN_DOUBLE_COLON);
    eat_token(parser);

    next = peek_next_token(parser);

    bool func_is_method = false;
    if (next.type == TOKEN_METHOD) {
        func_defn->method_token = next;
        func_is_method = true;
        eat_token(parser);
        next = peek_next_token(parser);
    }

    expect(parser, next, '(');
    func_defn->paren_start_token = next;
    eat_token(parser);

    parser->enclosing_function = func_defn;
    parser->inside_parameter_list = true;

    AstBlock *body = new_block(parser, BLOCK_IMPERATIVE); // Open a scope, so that the parameters can be pushed down into the scope of the body
    bool first_parameter_seen = false;
    while (true) {
        if (!first_parameter_seen) {
            next = peek_next_token(parser);
            if (next.type == ')') {
                eat_token(parser); 
                break;
            }
        }

        AstDeclaration *decl = parse_declaration(parser);
        if (!decl) {
            return NULL;
        }

        if (decl->flags & (DECLARATION_INFER | DECLARATION_TYPED)) {
            report_error_ast(parser, LABEL_ERROR, (Ast *) decl, "Default function arguments are currently not supported");
            return NULL;
        }

        if (decl->flags & (DECLARATION_CONSTANT)) {
            report_error_ast(parser, LABEL_ERROR, (Ast *) decl, "Constants are not allowed as function arguments");
            return NULL;
        }

        first_parameter_seen = true;

        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)decl->idents.items)[i];
            da_append(&func_defn->parameters, param);
        }

        next = peek_next_token(parser);
        if (next.type == ')') {
            eat_token(parser); 
            break;
        }

        if (next.type != ',') {
            report_error_token(parser, LABEL_ERROR, next, "Expected a ',' between parameters");
            return NULL;
        }

        eat_token(parser);
    }

    parser->inside_parameter_list = false;

    if (func_defn->is_method) {
        if (func_defn->parameters.count == 0) {
            report_error_token(parser, LABEL_ERROR, func_defn->method_token, "Method is missing the receiver type");
            return NULL;
        }
    }

    // Parse return type(s)
    next = peek_next_token(parser);

    if (next.type == TOKEN_RIGHT_ARROW) {
        eat_token(parser);

        while (true) {
            Type *return_type = parse_type(parser);
            if (!return_type) {
                return NULL;
            }
            da_append(&func_defn->return_types, return_type);

            next = peek_next_token(parser);

            if (next.type == ',') {
                eat_token(parser);
                continue;
            } else {
                break;
            }

        }
    } else {
        // Make void the return type
        Type *void_type = primitive_type(PRIMITIVE_VOID);
        da_append(&func_defn->return_types, void_type);
    }

    // Parse extern directive @Deprecate
    func_defn->calling_convention = CALLING_CONV_SAAD;
    bool is_extern = false;

    next = peek_next_token(parser);
    if (next.type == '#') {
        AstDirective *dir = parse_directive(parser);
        if (!dir) return NULL;

        if (dir->kind != DIRECTIVE_EXTERN) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)dir, "The #%s directive is not valid in this context", directive_names[dir->kind]);
            return NULL;
        }

        is_extern = true;

        if (dir->extern_abi == ABI_C) {
            func_defn->calling_convention = CALLING_CONV_MSVC;
        }
    }

    if (!is_extern) {
        body = parse_block(parser, body); // Here we tell parse_block to explicitly not make a new lexical scope, but instead use our existing function header
        if (!body) return NULL;
    }

    close_block(parser);
    parser->enclosing_function = NULL;

    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL); // The type of the identifier is set to a type representation of this function later down
    ident->flags |= IDENTIFIER_IS_NAME_OF_FUNCTION;

    if (func_is_method) {
        // The method will be bound to the receiving struct in typing
    } else {
        // Add the function to the current scope
        AstIdentifier *existing = add_identifier_to_scope(parser, parser->current_scope, ident);
        if (existing) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)ident, "Redeclaration of identifier '%s'", ident->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "Here is the previous declaration of '%s'", ident->name);
            return NULL;
        }
    }

    func_defn->head.kind   = AST_FUNCTION_DEFN;
    func_defn->head.file   = parser->current_file;
    func_defn->head.start  = ident_token.start;
    func_defn->head.end    = body->head.start;
    func_defn->identifier  = ident;
    func_defn->body        = body;
    func_defn->is_extern   = is_extern;
    func_defn->is_method   = func_is_method;
    func_defn->num_bytes_locals       = 0;
    func_defn->num_bytes_temporaries  = 0;
    func_defn->base_ptr               = 0;
    func_defn->temp_ptr               = 0;

    TypeFunction *func    = type_alloc(&parser->type_table, sizeof(TypeFunction));
    func->head.head.kind  = AST_TYPE;
    func->head.head.file  = parser->current_file;
    func->head.head.start = func_defn->head.start;
    func->head.head.end   = func_defn->head.end;
    func->head.kind       = TYPE_FUNCTION;
    func->node            = func_defn;

    // This is later down
    ident->type = (Type *)func;

    return func_defn;
}

AstIf *parse_if(Parser *parser) {
    Token if_token = peek_next_token(parser);
    eat_token(parser);

    parser->inside_statement_header = true;
    AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);
    parser->inside_statement_header = false;

    if (!condition) return NULL;

    AstBlock *block = parse_block(parser, NULL);
    if (!block) return NULL;

    AstIf *ast_if      = (AstIf *)(ast_allocate(parser, sizeof(AstIf)));
    ast_if->head.kind  = AST_IF;
    ast_if->head.file  = parser->current_file;
    ast_if->head.start = if_token.start;
    ast_if->head.end   = block->head.end;
    ast_if->then_block = block;
    ast_if->condition  = condition;
    ast_if->else_ifs   = da_init(2, sizeof(AstIf));

    while (true) {
        Token next = peek_next_token(parser);
        if (next.type == TOKEN_ELSE) {
            Token else_token = next;
            eat_token(parser);
            next = peek_next_token(parser);
            if (next.type == TOKEN_IF) {
                eat_token(parser);
                
                parser->inside_statement_header = true;            
                AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);
                if (!condition) return NULL;
                parser->inside_statement_header = false;
                AstBlock *block = parse_block(parser, NULL);
                if (!block) return NULL;

                AstIf else_if      = {0};
                else_if.head.kind  = AST_IF;
                else_if.head.file  = parser->current_file;
                else_if.head.start = else_token.start;
                else_if.head.end   = block->head.end;
                else_if.then_block = block;
                else_if.condition  = condition;

                da_append(&ast_if->else_ifs, else_if);

                continue; // Look for more else ifs
            }

            AstBlock *else_block = parse_block(parser, NULL);
            if (!else_block) return NULL;

            ast_if->else_block = else_block;
            break;
        }
        break;
    }

    return ast_if;
}

AstNew *parse_new(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_NEW);
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!expr) return NULL;

    AstNew *ast_new          = (AstNew *)(ast_allocate(parser, sizeof(AstNew)));
    ast_new->head.head.kind  = AST_NEW;
    ast_new->head.head.file  = parser->current_file;
    ast_new->head.head.start = start_token.start;
    ast_new->head.head.end   = expr->head.end;
    ast_new->expr            = expr;

    return ast_new;
}

AstTypeof *parse_typeof(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_TYPEOF);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!expr) return NULL;

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstTypeof *ast_typeof       = (AstTypeof *)(ast_allocate(parser, sizeof(AstTypeof)));
    ast_typeof->head.head.kind  = AST_TYPEOF;
    ast_typeof->head.head.file  = parser->current_file;
    ast_typeof->head.head.start = start_token.start;
    ast_typeof->head.head.end   = next.end;
    ast_typeof->expr            = expr;

    return ast_typeof;
}

AstSizeof *parse_sizeof(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_SIZEOF);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    Type *type = parse_type(parser);
    if (!type) return NULL;

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstSizeof *ast_sizeof       = (AstSizeof *)(ast_allocate(parser, sizeof(AstSizeof)));
    ast_sizeof->head.head.kind  = AST_SIZEOF;
    ast_sizeof->head.head.file  = parser->current_file;
    ast_sizeof->head.head.start = start_token.start;
    ast_sizeof->head.head.end   = next.end;
    ast_sizeof->type            = type;

    return ast_sizeof;
}

AstAssert *parse_assert(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_ASSERT);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
    if (!expr) return NULL;

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstAssert *assertion = (AstAssert *)(ast_allocate(parser, sizeof(AstAssert)));
    assertion->head.kind  = AST_ASSERT;
    assertion->head.file  = parser->current_file;
    assertion->head.start = start_token.start;
    assertion->head.end   = next.end;
    assertion->expr       = expr;

    return assertion;
}

AstPrint *parse_print(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_PRINT);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    AstPrint *print = (AstPrint *)(ast_allocate(parser, sizeof(AstPrint)));
    print->arguments = da_init(4, sizeof(AstExpr *));

    // @Copy-n-Paste from parse_function_call
    bool first_argument_seen = false;
    while (true) {
        //
        // Parse argument list
        //
        next = peek_next_token(parser);
        if (next.type == ')' || next.type == TOKEN_END) {
            break;
        }

        if (first_argument_seen) {
            if (next.type != ',') {
                report_error_token(parser, LABEL_ERROR, next, "Expected a ',' between arguments");
                exit(1);    
            }
            eat_token(parser);
            next = peek_next_token(parser);
        }

        AstExpr *arg = parse_expression(parser, MIN_PRECEDENCE);
        if (!arg) return NULL;

        da_append(&print->arguments, arg);
        first_argument_seen = true;
    }

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);
    
    print->head.kind  = AST_PRINT;
    print->head.file  = parser->current_file;
    print->head.start = start_token.start;
    print->head.end   = next.end;

    return print;
}

bool parse_expression_list(Parser *parser, DynamicArray *exprs) {
    Token next = {0};
    while (true) {
        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
        if (!expr) return false;

        da_append(exprs, expr);

        if (parser->inside_parameter_list) {
            // Prevent parsing the expressions of a function parameter list
            // as being multiple expressions.
            // e.g
            //
            // foo :: (a := 3, b: int) {...}
            //         ^------------- Here we are inside a parameter list and we want the comma to seperate the parameters and not the two expressions 3, b
            //
            break;
        }

        next = peek_next_token(parser);
        if (next.type == ',') {
            eat_token(parser);
            continue;
        }
        break;
    }

    return true;
}

AstDeclaration *parse_declaration(Parser *parser) {
    AstDeclaration *decl = (AstDeclaration *) ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.kind = AST_DECLARATION;
    decl->head.file = parser->current_file;

    decl->idents = da_init(2, sizeof(AstIdentifier *));
    decl->values = da_init(2, sizeof(AstExpr *));

    // Parse the identifiers
    Token next = {0};

    while (true) {
        next = peek_next_token(parser);
        if (next.type != TOKEN_LITERAL_IDENTIFIER) {
            report_error_token(parser, LABEL_ERROR, next, "Expected an identifier here");
            return NULL;
        }

        AstIdentifier *ident = make_identifier_from_token(parser, next, NULL);
        ident->decl = decl;
        da_append(&decl->idents, ident);

        eat_token(parser);
        next = peek_next_token(parser);
        if (next.type == ',') {
            eat_token(parser);
            continue;
        }

        break;
    }

    // Parse the values
    next = peek_next_token(parser);

    // Infer. e.g. a := b
    if (next.type == TOKEN_COLON_EQUAL) {
        decl->flags |= DECLARATION_INFER;

        if (parser->current_scope->belongs_to_struct) {
            report_error_token(parser, LABEL_ERROR, next, "Default struct values are not yet supported");
            return NULL;
        }
        eat_token(parser);

        bool ok = parse_expression_list(parser, &decl->values);
        if (!ok) return NULL;
    }
    // Typed. 'a: int = b' or 'a: int'
    else if (next.type == ':') {
        eat_token(parser);

        Type *type = parse_type(parser);
        if (type == NULL) return NULL;
        decl->type = type;

        next = peek_next_token(parser);

        if (next.type == '=') {
            decl->flags |= DECLARATION_TYPED;
            if (parser->current_scope->belongs_to_struct) {
                report_error_token(parser, LABEL_ERROR, next, "Default struct values are not yet implemented!");
                return NULL;
            }
            
            eat_token(parser);

            bool ok = parse_expression_list(parser, &decl->values);
            if (!ok) return NULL;

        } 
        else {
            decl->flags |= DECLARATION_TYPED_NO_EXPR;
        }

        // Add the type to all of the identifiers
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            ident->type = type;
        }
    }

    // Constant. e.g. A :: 5;
    else if (next.type == TOKEN_DOUBLE_COLON) {
        decl->flags |= DECLARATION_CONSTANT;

        eat_token(parser);
        bool ok = parse_expression_list(parser, &decl->values);
        if (!ok) return NULL;

        // Mark all the identifiers as constant
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            ident->flags |= IDENTIFIER_IS_CONSTANT;
        }
    }
    else {
        report_error_token(parser, LABEL_ERROR, next, "Invalid declaration");
        return NULL;
    }

    // Add decl position info
    decl->head.start = ((AstIdentifier **)decl->idents.items)[0]->head.start;                                                        // Pos of first ident
    decl->head.end   = decl->values.count > 0 ? ((AstExpr **)decl->values.items)[decl->values.count - 1]->head.end : decl->type->head.end; // Pos of last value or the type

    if (parser->current_scope->belongs_to_struct) {
        decl->flags |= DECLARATION_IS_STRUCT_MEMBER;
    }

    // Parent each identifier to this declaration
    for (int i = 0; i < decl->idents.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
        ident->decl = decl;
    }

    // Add the identifiers to the current scope
    if (parser->current_scope) {
    
        AstIdentifier *existing = add_declaration_to_scope(parser, parser->current_scope, decl);
        if (existing != NULL) {
            return NULL;
        }
    
    } else {
        report_error_ast(parser, LABEL_ERROR, (Ast *)decl, "Compiler Error: No scope is present while parsing the following declaration");
        return NULL;
    }

    return decl;
}

AstIdentifier *generate_identifier(Parser *parser, char *ident_name, Type *type, IdentifierFlags flags) {
    assert(type);
    AstIdentifier *ident = make_identifier_from_string(parser, ident_name, type);
    ident->flags |= flags;
    ident->head.flags |= AST_FLAG_COMPILER_GENERATED | AST_FLAG_IS_TYPE_CHECKED;
    return ident;
}

int align_value(int value, int alignment) {
    if (alignment == 0) return value;

    int rem = value % alignment;
    if (rem == 0) return value;
    else {
        if (value < 0) return value - (alignment + rem);
        else           return value + (alignment - rem);
    }
}

AstIdentifier *make_identifier_from_string(Parser *parser, const char *name, Type *type) {
    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.kind = AST_IDENTIFIER;
    ident->head.file = parser->current_file;
    ident->type      = type;
    ident->name      = (char *)(name);

    return ident;
}

AstIdentifier *make_identifier_from_token(Parser *parser, Token ident_token, Type *type) {
    assert(ident_token.type == TOKEN_LITERAL_IDENTIFIER);

    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.kind  = AST_IDENTIFIER;
    ident->head.file  = parser->current_file;
    ident->head.start = ident_token.start;
    ident->head.end   = ident_token.end;
    ident->type       = type;
    ident->name       = ident_token.as_value.value.identifier.name;
    ident->stack_offset = 0;

    return ident;
}

void expect(Parser *parser, Token given, TokenType expected_type) {
    if (given.type != expected_type) {
        report_error_token(parser, LABEL_ERROR, given, "Expected '%s', but got '%s'", token_type_to_str(expected_type), token_type_to_str(given.type));
        exit(1);
    }
    // Everything's cool dude
}

int get_precedence(OperatorType op) {
    switch (op) {
        case OP_LOGICAL_OR: 
            return 1;
        case OP_LOGICAL_AND: 
            return 2;
        case OP_DOUBLE_EQUAL:
        case OP_NOT_EQUAL: 
            return 3;
        case OP_GREATER_THAN: 
        case OP_LESS_THAN:
        case OP_GREATER_THAN_EQUAL:
        case OP_LESS_THAN_EQUAL:
            return 5;
        case OP_MINUS:
        case OP_PLUS:
            return 6;
        case OP_TIMES:
        case OP_DIVIDE:
        case OP_MODULO:
            return 7;
        case OP_POWER:
            return 8;
        case OP_NOT:
            return 9;
        case OP_UNARY_MINUS:
        case OP_ADDRESS_OF:
        case OP_POINTER_DEREFERENCE:
        case OP_CAST:
            return 10;
        case OP_DOT:
        case OP_SUBSCRIPT:
            return 11;
        default:
            printf("%s:%d: compiler-error: Unexpected token type '%s'. Expected token to be an operator\n", __FILE__, __LINE__, token_type_to_str((TokenType)(op)));
            exit(1);
    }
}

bool ends_expression(Token token) {
    if (token.type == TOKEN_END) return true;
    if (token.type == ')') return true;
    if (token.type == ';') return true;
    if (token.type == ']') return true;
    if (token.type == '{') return true;
    if (token.type == '}') return true;
    if (token.type == ',') return true;
    if (token.type == TOKEN_DOUBLE_DOT) return true;
    if (token.type == TOKEN_TRIPLE_DOT) return true;
    if (is_assignment_operator(token)) return true;

    return false;
}

AstExpr *parse_expression(Parser *parser, int min_prec) {
    AstExpr *left = parse_leaf(parser);
    if (!left) return NULL;

    Token next = peek_next_token(parser);
    if (ends_expression(next)) return left;

    while (true) {
        Token next = peek_next_token(parser);

        if (ends_expression(next))          break;
        if (!is_binary_operator(next.type)) break;

        int next_prec = get_precedence((OperatorType)(next.type));
        if (next_prec <= min_prec) {
            break;
        } else {
            eat_token(parser);

            if (next.type == '.') {
                AstExpr *right = parse_leaf(parser);
                if (!right) return NULL;
                left = make_member_access(parser, next, left, right);
            } else if (next.type == '[') {
                left = parse_array_access(parser, next, left);
                if (!left) return NULL;
            } else {
                AstExpr *right = parse_expression(parser, next_prec);
                if (!right) return NULL;
                left = make_binary_node(parser, next, left, right);
            }
        }
    }

    return left;
}

AstExpr *parse_leaf(Parser *parser) {
    Token t = peek_next_token(parser);
    if (t.type == TOKEN_LITERAL_BOOLEAN ||
        t.type == TOKEN_LITERAL_INTEGER ||
        t.type == TOKEN_LITERAL_FLOAT   ||
        t.type == TOKEN_LITERAL_NULL    ||
        t.type == TOKEN_LITERAL_STRING
    ) {
        eat_token(parser);
        return make_literal_node(parser, t);
    }

    if (t.type == ';') {
        // We intentionally don't eat the semicolon because we want the expression to end right after this @Bug; "a := ;" currently doesn't produce a parse error

        AstSemicolonExpr *sce = ast_allocate(parser, sizeof(AstSemicolonExpr));
        sce->head.head.kind   = AST_SEMICOLON_EXPR;
        sce->head.head.file   = parser->current_file;
        sce->head.head.start  = t.start;
        sce->head.head.end    = t.end;
        return (AstExpr *) sce;
    }

    if (t.type == '(') {
        eat_token(parser);
        AstExpr *sub_expr = parse_expression(parser, MIN_PRECEDENCE);
        if (!sub_expr) return NULL;

        Token next = peek_next_token(parser);
        if (next.type != ')') {
            report_error_token(parser, LABEL_ERROR, t, "Expected a closing paranthesis for this parenthesis");
            exit(1);
        }
        eat_token(parser);
        return sub_expr;
    }

    if (t.type == '!') {
        eat_token(parser);
        int prec = get_precedence(OP_NOT);
        AstExpr *sub_expr = parse_expression(parser, prec); 
        if (!sub_expr) return NULL;
        return make_unary_node(parser, t, sub_expr, OP_NOT);
    }

    if (t.type == '-') {
        eat_token(parser);
        int prec = get_precedence(OP_UNARY_MINUS);
        AstExpr *sub_expr = parse_expression(parser, prec); 
        if (!sub_expr) return NULL;
        return make_unary_node(parser, t, sub_expr, OP_UNARY_MINUS);
    }

    if (t.type == '&') {
        eat_token(parser);
        int prec = get_precedence(OP_ADDRESS_OF);
        AstExpr *sub_expr = parse_expression(parser, prec); 
        if (!sub_expr) return NULL;
        return make_unary_node(parser, t, sub_expr, OP_ADDRESS_OF);
    }

    if (t.type == '*') {
        eat_token(parser);
        int prec = get_precedence(OP_POINTER_DEREFERENCE);
        AstExpr *sub_expr = parse_expression(parser, prec); 
        if (!sub_expr) return NULL;
        return make_unary_node(parser, t, sub_expr, OP_POINTER_DEREFERENCE);
    }

    if (starts_struct_literal(parser)) {
        AstStructLiteral *struct_literal = parse_struct_literal(parser);
        return (AstExpr *)(struct_literal);
    }

    if (starts_array_literal(parser))  {
        AstArrayLiteral *array_lit = parse_array_literal(parser);
        return (AstExpr *)(array_lit);
    }

    if (t.type == TOKEN_XX || t.type == TOKEN_CAST)  {
        AstCast *cast = parse_cast(parser);
        return (AstExpr *)(cast);
    }

    if (t.type == '.' && peek_token(parser, 1).type == TOKEN_LITERAL_IDENTIFIER) {
        Token ident = peek_token(parser, 1);

        eat_token(parser);
        eat_token(parser);

        AstEnumLiteral *eval  = ast_allocate(parser, sizeof(AstEnumLiteral));
        eval->head.head.kind  = AST_ENUM_LITERAL;
        eval->head.head.file  = parser->current_file;
        eval->head.head.start = t.start;
        eval->head.head.end   = ident.end;
        eval->identifier      = make_identifier_from_token(parser, ident, NULL);

        return (AstExpr *)(eval);
    }

    if (t.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == '(') {
        AstFunctionCall *call = parse_function_call(parser);
        return (AstExpr *)(call);
    }

    if (t.type == TOKEN_TYPEOF) {
        AstTypeof *ast_typeof = parse_typeof(parser);
        if (!ast_typeof) return NULL;
        return (AstExpr *)(ast_typeof);
    }

    if (t.type == TOKEN_SIZEOF) {
        AstSizeof *ast_sizeof = parse_sizeof(parser);
        if (!ast_sizeof) return NULL;
        return (AstExpr *)(ast_sizeof);
    }

    if (t.type == TOKEN_NEW) {
        AstNew *ast_new = parse_new(parser);
        if (!ast_new) return NULL;
        return (AstExpr *)(ast_new);
    }

    if (t.type == TOKEN_LITERAL_IDENTIFIER) {
        eat_token(parser);
        return make_literal_node(parser, t);
    }

    report_error_token(parser, LABEL_ERROR, t, "Syntax Error: Invalid start of expression");
    return NULL;
}

bool starts_array_literal(Parser *parser) {
    Token token = peek_next_token(parser);

    if (token.type == '[') return true;

    return false;
}

bool starts_struct_literal(Parser *parser) {
    // Prevent parsing an ending identifer and open bracket as a struct literal if we are in a statement header
    //
    // for a {...}
    //     ^^^
    // here the identifier 'a' and bracket would otherwise be parsed as a struct literal
    if (parser->inside_statement_header) return false;

    Token next = peek_next_token(parser);
    if (next.type == '{') return true;
    if (next.type == TOKEN_LITERAL_IDENTIFIER && peek_token(parser, 1).type == '{') return true;

    return false;
}

void *ast_allocate(Parser *parser, size_t size) {
    return arena_allocate(&parser->ast_nodes, size);
}

AstExpr *make_member_access(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs) {
    assert(token.type == '.');

    AstMemberAccess *ma = ast_allocate(parser, sizeof(AstMemberAccess));
    ma->head.head.kind  = AST_MEMBER_ACCESS;
    ma->head.head.file  = parser->current_file;
    ma->head.head.start = lhs->head.start;
    ma->head.head.end   = rhs->head.end;
    ma->left            = lhs;
    ma->right           = rhs;

    return (AstExpr *)(ma);
}

AstExpr *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type) {
    assert(is_unary_operator(token.type));

    AstUnary *unary_op = (AstUnary *) ast_allocate(parser, sizeof(AstUnary));
    unary_op->head.head.kind  = AST_UNARY;
    unary_op->head.head.file  = parser->current_file;
    unary_op->head.head.start = token.start;
    unary_op->head.head.end   = expr->head.end;
    unary_op->operator        = op_type;
    unary_op->expr            = expr;

    return (AstExpr *)(unary_op);
}

AstExpr *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs) {
    assert(is_binary_operator(token.type));

    AstBinary *bin_op = (AstBinary *) ast_allocate(parser, sizeof(AstBinary));
    bin_op->head.head.kind  = AST_BINARY;
    bin_op->head.head.file  = parser->current_file;
    bin_op->head.head.start = lhs->head.start;
    bin_op->head.head.end   = rhs->head.end;
    bin_op->operator        = token.type;
    bin_op->left            = lhs;
    bin_op->right           = rhs;

    return (AstExpr *)(bin_op);
}
 
AstExpr *make_literal_node(Parser *parser, Token token) {
    assert(is_literal(token));

    AstLiteral *literal = (AstLiteral *)(ast_allocate(parser, sizeof(AstLiteral)));
    literal->head.head.kind  = AST_LITERAL;
    literal->head.head.file  = parser->current_file;
    literal->head.head.start = token.start;
    literal->head.head.end   = token.end;
    literal->kind            = (LiteralKind)(token.type);
    literal->as              = token.as_value;

    return (AstExpr *)(literal);
}

void eat_token(Parser *parser) {
    parser->current_token_index++;
}

void vomit_token(Parser *parser) {
    parser->current_token_index--;
}

Token peek_token(Parser *parser, int lookahead) {
    return ((Token *)parser->lexer->tokens.items)[parser->current_token_index + lookahead];
}

Token peek_next_token(Parser *parser) {
    return ((Token *)parser->lexer->tokens.items)[parser->current_token_index];
}

bool is_literal(Token token) {
    if (token.type == TOKEN_LITERAL_BOOLEAN)    return true;
    if (token.type == TOKEN_LITERAL_INTEGER)    return true;
    if (token.type == TOKEN_LITERAL_FLOAT)      return true;
    if (token.type == TOKEN_LITERAL_STRING)     return true;
    if (token.type == TOKEN_LITERAL_NULL)       return true;
    if (token.type == TOKEN_LITERAL_IDENTIFIER) return true;
    return false;
}

bool is_type_specifier(Token token) {
    return is_primitive_type_token(token) || token.type == TOKEN_LITERAL_IDENTIFIER;
}

bool is_primitive_type_token(Token token) {
    return (token.type >= TOKEN_TYPE_UINT && token.type <= TOKEN_TYPE_VOID);
}

bool is_assignment_operator(Token token) {
    if (token.type == '=')                return true;
    if (token.type == TOKEN_PLUS_EQUAL)   return true;
    if (token.type == TOKEN_MINUS_EQUAL)  return true;
    if (token.type == TOKEN_TIMES_EQUAL)  return true;
    if (token.type == TOKEN_DIVIDE_EQUAL) return true;
    return false;
}

void report_error_range(Parser *parser, Pos start, Pos end, const char *message, ...) {
    va_list args;
    va_start(args, message);
    assert(parser->current_file);

    report_error_helper(parser->current_file->absolute_path, parser->current_file->text, LABEL_ERROR, start, end, message, args);

    va_end(args);
}

// Passing NULL to failing_ast omits reporting specific line info. Should mostly be for hints
void report_error_ast(Parser *parser, const char* label, Ast *failing_ast, const char *message, ...) {
    va_list args;
    va_start(args, message);
    assert(parser->current_file);

    if (failing_ast) {
        report_error_helper(failing_ast->file->absolute_path, failing_ast->file->text, label, failing_ast->start, failing_ast->end, message, args);
    } else {
        Pos ghost_pos = (Pos){-1, -1, -1};
        report_error_helper(parser->current_file->absolute_path, parser->current_file->text, label, ghost_pos, ghost_pos, message, args);
    }

    va_end(args);
}

void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...) {
    va_list args;
    va_start(args, message);
    assert(parser->current_file);

    report_error_helper(parser->current_file->absolute_path, parser->current_file->text, label, failing_token.start, failing_token.end, message, args);

    va_end(args);
}