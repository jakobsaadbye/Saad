#include "symbol_table.c"

#define AST_ALLOCATION_SIZE 16384
#define IDENT_TABLE_SIZE    128

#define MAX_PRECEDENCE 99
#define MIN_PRECEDENCE -99

typedef struct Parser {
    Lexer *lexer;

    Arena ast_nodes;

    int current_token_index;
    int current_ast_node_index;

    SymbolTable ident_table;
    SymbolTable function_table;
    TypeTable   type_table;
} Parser;

Parser           parser_init();
AstCode         *parse_top_level_code(Parser *parser);
AstNode         *parse_statement(Parser *parser);
AstDeclaration  *parse_declaration(Parser *parser, DeclarationFlags flags);
AstAssignment   *parse_assignment(Parser *parser);
AstFunctionDefn *parse_function_defn(Parser *parser);
AstFunctionCall *parse_function_call(Parser *parser);
AstStruct       *parse_struct(Parser *parser);
AstEnum         *parse_enum(Parser *parser);
AstIf           *parse_if(Parser *parser);
AstFor          *parse_for(Parser *parser);
AstPrint        *parse_print(Parser *parser);
AstAssert       *parse_assert(Parser *parser);
AstTypeof       *parse_typeof(Parser *parser);
AstReturn       *parse_return(Parser *parser);
AstExpr         *parse_expression(Parser *parser, int min_prec);
AstExpr         *parse_range_or_normal_expression(Parser *parser);
AstExpr         *parse_leaf(Parser *parser);

AstDeclaration *make_declaration(Parser *parser, Token ident_token, AstExpr *expr, TypeInfo *type, DeclarationFlags flags);
AstExpr        *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
AstExpr        *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type);
AstExpr        *make_literal_node(Parser *parser, Token token);
AstIdentifier  *make_identifier_from_token(Parser *parser, Token ident_token, TypeInfo *type);
AstIdentifier  *make_identifier_from_string(Parser *parser, const char *name, TypeInfo *type);

bool identifier_is_equal(const void *key, const void *item);
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
bool starts_struct_literal(Parser *parser);
int get_precedence(OperatorType op);
TypeInfo token_to_type(Token token);
int align_value(int value, int alignment);

void report_error(Parser *parser, Pos start, Pos end, const char *message, ...);
void report_error_ast(Parser *parser, const char* label, AstNode *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);


Parser parser_init(Lexer *lexer) {
    Parser parser = {0};
    parser.lexer           = lexer;
    parser.ast_nodes       = arena_init(AST_ALLOCATION_SIZE);
    parser.ident_table     = symbol_table_init();
    parser.function_table  = symbol_table_init();
    parser.type_table      = type_table_init();

    return parser;
}

AstCode *parse_top_level_code(Parser *parser) {
    AstCode *code = (AstCode *)(ast_allocate(parser, sizeof(AstCode)));
    code->statements = da_init(8, sizeof(AstNode *));

    Token next = peek_next_token(parser);
    while (true) {
        next = peek_next_token(parser);
        if (next.type == TOKEN_END) break;
        // @Improvement - Need to limit what statements are allowed at top level scope
        AstNode *stmt = parse_statement(parser);
        da_append(&code->statements, stmt);
    }

    return code;
}

AstBlock *parse_block(Parser *parser, bool open_lexical_scope) {
    AstBlock *block = (AstBlock *)(ast_allocate(parser, sizeof(AstBlock)));

    Token start_token = peek_next_token(parser);
    expect(parser, start_token, '{');
    eat_token(parser);

    if (open_lexical_scope) open_scope(&parser->ident_table);

    int i = 0;
    Token next;
    while (true) {
        next = peek_next_token(parser);
        if (next.type == '}') break;
        else {
            if (i >= MAX_STATEMENTS_WITHIN_BLOCK) {
                printf("%s:%d: compiler-error: Reached maximum number of statements allowed within a single block which is %d", __FILE__, __LINE__, MAX_STATEMENTS_WITHIN_BLOCK);
                report_error_token(parser, LABEL_ERROR, start_token, "Here is the block with too many statements");
                exit(1);
            }

            block->statements[i] = (AstNode *)(parse_statement(parser));
            block->num_of_statements += 1;
            i++;
        }
    }

    expect(parser, next, '}');  // Should not be able to fail
    eat_token(parser);

    if (open_lexical_scope) close_scope(&parser->ident_table);

    block->head.type         = AST_BLOCK;  
    block->head.start        = start_token.start;
    block->head.end          = next.end;

    return block;
}

AstNode *parse_statement(Parser *parser) {
    bool statement_ends_with_semicolon = false;
    AstNode *stmt = NULL;
    
    Token token = peek_next_token(parser);

    if (token.type == TOKEN_IDENTIFIER) {
        Token next      = peek_token(parser, 1);
        Token next_next = peek_token(parser, 2);
        if (next.type == TOKEN_DOUBLE_COLON && next_next.type == '(') {
            stmt = (AstNode *)(parse_function_defn(parser));
            statement_ends_with_semicolon = false;
        }
        else if (next.type == '(') {
            stmt = (AstNode *)(parse_function_call(parser));
            statement_ends_with_semicolon = true;
        }
        // @Note - Structs should probably be parsed at the top level code instead of as a statement 
        else if (next.type == TOKEN_DOUBLE_COLON && next_next.type == TOKEN_STRUCT) {
            stmt = (AstNode *)(parse_struct(parser));
            statement_ends_with_semicolon = false;
        }
        else if (next.type == TOKEN_DOUBLE_COLON && next_next.type == TOKEN_ENUM) {
            stmt = (AstNode *)(parse_enum(parser));
            statement_ends_with_semicolon = false;
        }
        else if (
            next.type == ':' ||
            next.type == TOKEN_COLON_EQUAL || 
            next.type == TOKEN_DOUBLE_COLON
        ) {
            stmt = (AstNode *)(parse_declaration(parser, 0));
            statement_ends_with_semicolon = true;
        } else if (
            next.type == '.' ||
            is_assignment_operator(next)
        ) {
            stmt = (AstNode *)(parse_assignment(parser));
            statement_ends_with_semicolon = true;
        } else {
            // Fallthrough
        }
    }
    else if (token.type == TOKEN_RETURN) {
        stmt = (AstNode *)(parse_return(parser));
        statement_ends_with_semicolon = true;
    }
    else if (token.type == TOKEN_PRINT) {
        stmt = (AstNode *)(parse_print(parser));
        statement_ends_with_semicolon = true;
    }
    else if (token.type == TOKEN_ASSERT) {
        stmt = (AstNode *)(parse_assert(parser));
        statement_ends_with_semicolon = true;
    }
    else if (token.type == TOKEN_TYPEOF) {
        stmt = (AstNode *)(parse_typeof(parser));
        statement_ends_with_semicolon = true;
    }
    else if (token.type == TOKEN_IF) {
        stmt = (AstNode *)(parse_if(parser));
        statement_ends_with_semicolon = false;
    }
    else if (token.type == TOKEN_FOR) {
        stmt = (AstNode *)(parse_for(parser));
        statement_ends_with_semicolon = false;
    }
    else if (token.type == '{') {
        stmt = (AstNode *)(parse_block(parser, true));
        statement_ends_with_semicolon = false;
    }

    if (stmt == NULL) {
        report_error_token(parser, LABEL_ERROR, token, "Invalid statement");
        exit(1);
    }

    if (statement_ends_with_semicolon) {
        Token next = peek_next_token(parser);
        if (next.type != ';') {
            Token prev = peek_token(parser, -1);
            report_error(parser, prev.end, prev.end, "Syntax Error: Expected a semi-colon");
            exit(1);
        }
        eat_token(parser);
    }

    return stmt;
}

AstLiteral *int_literal_from_value(Parser *parser, int value) {
    AstLiteral *lit     = (AstLiteral *)(ast_allocate(parser, sizeof(AstLiteral)));
    lit->head.head.type = AST_LITERAL;
    lit->kind           = (LiteralKind)(TOKEN_INTEGER);
    lit->as.value.integer = value;
    lit->as.flags         = 0;
    return lit;
}

bool compare_enumerator(const void *key, const void *item) {
    AstEnumerator *etor = (AstEnumerator *)(item);
    return strcmp(key, etor->name) == 0;
}

TypeInfo *parse_type(Parser *parser) {
    Token next = peek_next_token(parser);
    if (is_primitive_type_token(next)) {
        eat_token(parser);
        TypePrimitive *ti = type_alloc(&parser->type_table, sizeof(TypePrimitive));
        TypePrimitive *primitive = &primitive_types[(next.type - TOKEN_TYPE_INT) + 1]; // +1 to skip over invalid kind
        memcpy(ti, primitive, sizeof(TypePrimitive));
        ti->head.head.type  = AST_TYPE_INFO;
        ti->head.head.start = next.start;
        ti->head.head.end   = next.end;
        return (TypeInfo *)(ti);
    }
    if (next.type == TOKEN_IDENTIFIER) {
        // Struct or enum
        eat_token(parser);
        TypeInfo *ti   = type_alloc(&parser->type_table, sizeof(TypeInfo));
        ti->head.type  = AST_TYPE_INFO;
        ti->head.start = next.start;
        ti->head.end   = next.end;
        ti->kind       = TYPE_NAME;
        ti->as.name    = next.as_value.value.identifier.name;
        return ti;
    }

    report_error_token(parser, LABEL_ERROR, next, "Expected a type");
    exit(1);
}

AstEnum *parse_enum(Parser *parser) {
    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_IDENTIFIER);
    eat_token(parser);
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL); // Type of ident is set later down to refer to the enum it self

    // Expose the enum as an identifier so enum values can be referenced explicitly using ENUM.value
    {
        ident->flags |= IDENTIFIER_IS_NAME_OF_ENUM;
        Symbol *exists = symbol_add_identifier(&parser->ident_table, ident);
        if (exists) {
            AstIdentifier *found = exists->as.identifier;
            report_error_ast(parser, LABEL_ERROR, (AstNode *)(ident), "Type '%s' already defined", found->name);
            report_error_ast(parser, LABEL_NOTE, (AstNode *)(found), "Here is the previous definition of '%s'", found->name);
            exit(1);
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
        if (next.type == TOKEN_IDENTIFIER && peek_token(parser, 1).type == '=') {
            // Explicit value
            eat_token(parser);
            eat_token(parser);

            AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
            if (!expr) return NULL;

            AstEnumerator *etor = (AstEnumerator *)(ast_allocate(parser, sizeof(AstEnumerator)));
            etor->head.type  = AST_ENUMERATOR;
            etor->head.start = next.start;
            etor->head.end   = next.end;
            etor->parent     = ast_enum;
            etor->name       = next.as_value.value.identifier.name; // @Robustness - Should probably do a copy here so that we are not tied to the token array
            etor->expr       = expr;
            etor->index      = enum_index;

            da_append(&ast_enum->enumerators, etor);

            enum_index++;
        } else if (next.type == TOKEN_IDENTIFIER) {
            // Auto-increment
            eat_token(parser);

            AstEnumerator *etor = (AstEnumerator *)(ast_allocate(parser, sizeof(AstEnumerator)));
            etor->head.type  = AST_ENUMERATOR;
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
            report_error_token(parser, LABEL_ERROR, next, "Expected a comma here");
            exit(1);
        }
    }
    expect(parser, next, '}');
    eat_token(parser);

    ast_enum->head.type  = AST_ENUM;
    ast_enum->head.start = ident->head.start;
    ast_enum->head.end   = peek_token(parser, -1).end;
    ast_enum->identifier = ident;

    TypeEnum *type_enum       = type_alloc(&parser->type_table, sizeof(TypeEnum));
    type_enum->head.head.type = AST_TYPE_INFO;
    type_enum->head.head.start = ast_enum->head.start;
    type_enum->head.head.end   = ast_enum->head.end;
    type_enum->head.kind      = TYPE_ENUM;
    type_enum->head.as.name   = ident->name;
    type_enum->node           = ast_enum; // @Improvement - Probably also need to copy over the hashtable of enumerators
    type_enum->identifier     = ast_enum->identifier;
    type_enum->backing_type   = primitive_type(PRIMITIVE_INT); // @Improvement - Support for having a backing integer type

    TypeInfo *exists = type_add_user_defined(&parser->type_table, (TypeInfo *)(type_enum));
    assert(!exists);

    ident->type = (TypeInfo *)(type_enum);

    return ast_enum;
}

AstStruct *parse_struct(Parser *parser) {
    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_IDENTIFIER);
    eat_token(parser);
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL);

    Token next = peek_next_token(parser);
    expect(parser, next, TOKEN_DOUBLE_COLON);
    eat_token(parser);

    Token struct_token = peek_next_token(parser);
    expect(parser, struct_token, TOKEN_STRUCT);
    eat_token(parser);

    next = peek_next_token(parser);
    expect(parser, next, '{');
    eat_token(parser);

    AstStruct *ast_struct    = (AstStruct *)(ast_allocate(parser, sizeof(AstStruct)));
    ast_struct->member_table = symbol_table_init();

    // Parse members
    int member_index = 0;
    while (next.type != '}' && next.type != TOKEN_END) {
        AstDeclaration *member = parse_declaration(parser, DECLARATION_IS_STRUCT_MEMBER);
        member->member_index = member_index;

        next = peek_next_token(parser);
        expect(parser, next, ';');
        eat_token(parser);

        Symbol *existing = symbol_add_struct_member(&ast_struct->member_table, member);
        if (existing != NULL) {
            report_error_ast(parser, LABEL_ERROR, (AstNode *)(member->identifier), "Redeclaration of member variable '%s'", member->identifier->name);
            report_error_ast(parser, LABEL_NOTE, (AstNode *)(existing->as.identifier), "Here is the previous declaration ...");
            exit(1);
        }

        next = peek_next_token(parser);
        member_index++;
    }
    expect(parser, next, '}');
    eat_token(parser);

    ast_struct->head.type  = AST_STRUCT;
    ast_struct->head.start = ident_token.start;
    ast_struct->head.end   = next.end;
    ast_struct->identifier = ident;

    TypeStruct *type_struct     = type_alloc(&parser->type_table, sizeof(TypeStruct));
    type_struct->head.head.type = AST_TYPE_INFO;
    type_struct->head.head.start = ast_struct->head.start;
    type_struct->head.head.end   = ast_struct->head.end;
    type_struct->head.kind      = TYPE_STRUCT;
    type_struct->head.as.name   = ident->name;
    type_struct->identifier     = ident;
    type_struct->node           = ast_struct;
    TypeInfo *existing          = type_add_user_defined(&parser->type_table, (TypeInfo *)(type_struct));
    if (existing) {
        if (existing->kind == TYPE_STRUCT) {
            TypeStruct *existing_struct = (TypeStruct *)(existing);
            report_error_ast(parser, LABEL_ERROR, (AstNode *)(ast_struct->identifier), "Struct '%s' is already defined", ast_struct->identifier->name);
            report_error_ast(parser, LABEL_NOTE, (AstNode *)(existing_struct->node), "Here is the previously defined struct");
            exit(1);
        } else {
            TypeStruct *existing_enum = (TypeStruct *)(existing);
            report_error_ast(parser, LABEL_ERROR, (AstNode *)(ast_struct->identifier), "Struct '%s' is already defined as an enum", ast_struct->identifier->name);
            report_error_ast(parser, LABEL_NOTE, (AstNode *)(existing_enum->node), "Here is the previously defined enum");
        }
    }

    return ast_struct;
}

AstAssignment *parse_assignment(Parser *parser) {
    AstExpr *lhs = parse_expression(parser, MIN_PRECEDENCE);
    bool valid_lhs = false;
    if (lhs->head.type == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) 
        valid_lhs = true;
    if (lhs->head.type == AST_MEMBER_ACCESS) 
        valid_lhs = true;
    if (!valid_lhs) {
        report_error_ast(parser, LABEL_ERROR, (AstNode *)(lhs), "Invalid expression as left-hand side of assignment");
        exit(1);
    }

    Token op_token = peek_next_token(parser);
    if (!is_assignment_operator(op_token)) {
        report_error_token(parser, LABEL_ERROR, op_token, "Expected an assignment operator");
        exit(1);
    }
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    AstAssignment *assign = (AstAssignment *)(ast_allocate(parser, sizeof(AstAssignment)));
    assign->head.type     = AST_ASSIGNMENT;
    assign->head.start    = lhs->head.start;
    assign->head.end      = expr->head.end;
    assign->lhs           = lhs;
    assign->op            = (AssignOp)(op_token.type);
    assign->expr          = expr;

    return assign;
}

AstFor *parse_for(Parser *parser) {
    AstIdentifier *iterator = NULL;
    AstExpr *iterable = NULL;

    Token for_token = peek_next_token(parser);
    eat_token(parser);

    Token next      = peek_next_token(parser);
    Token next_next = peek_token(parser, 1);

    if (next.type == TOKEN_IDENTIFIER && next_next.type == TOKEN_IN) {
        iterator = make_identifier_from_token(parser, next, NULL);

        eat_token(parser);
        eat_token(parser);

        iterable = parse_range_or_normal_expression(parser);
    } else {
        iterable = parse_range_or_normal_expression(parser);

        Token next = peek_next_token(parser);
        if (next.type != '{' ) {
            report_error(parser, iterable->head.start, next.end, "Syntax Error: Invalid for-expression");
            exit(1);
        }

        if (iterable->head.type == AST_RANGE_EXPR) {
            iterator = make_identifier_from_string(parser, "it", primitive_type(PRIMITIVE_INT));
        } else {
            // ToDo: Implement array type
            XXX;
        }
    }

    // Push down the iterator into the body scope
    open_scope(&parser->ident_table);
    symbol_add_identifier(&parser->ident_table, iterator);
    AstBlock *body = parse_block(parser, false);
    close_scope(&parser->ident_table);

    AstFor *ast_for = (AstFor *)(ast_allocate(parser, sizeof(AstFor)));
    ast_for->head.type = AST_FOR;
    ast_for->head.start = for_token.start;
    ast_for->head.end = body->head.end;
    ast_for->iterator = iterator;
    ast_for->iterable = iterable;
    ast_for->body = body;

    return ast_for;
}

AstExpr *parse_range_or_normal_expression(Parser *parser) {
    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    Token next = peek_next_token(parser);
    if (next.type == TOKEN_DOUBLE_DOT || next.type == TOKEN_TRIPLE_DOT) {
        eat_token(parser);
        AstExpr *end = parse_expression(parser, MIN_PRECEDENCE);

        AstRangeExpr *range_expr = (AstRangeExpr *)(ast_allocate(parser, sizeof(AstRangeExpr)));            
        range_expr->head.head.type = AST_RANGE_EXPR;
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
    Token return_token = peek_next_token(parser);
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    AstReturn *ast_return = (AstReturn *)(ast_allocate(parser, sizeof(AstReturn)));
    ast_return->head.type  = AST_RETURN;
    ast_return->head.start = return_token.start;
    ast_return->head.end   = expr->head.end;
    ast_return->expr       = expr;
    ast_return->enclosing_function = NULL;

    return ast_return;
}

AstStructLiteral *parse_struct_literal(Parser *parser) {
    TypeInfo *explicit_type = NULL;
    Token start_token = peek_next_token(parser);
    if (start_token.type == TOKEN_IDENTIFIER) {
        explicit_type = parse_type(parser);
    }

    Token next = peek_next_token(parser);
    expect(parser, next, '.');
    eat_token(parser);

    next = peek_next_token(parser);
    expect(parser, next, '{');
    eat_token(parser);

    AstStructLiteral *struct_literal = ast_allocate(parser, sizeof(AstStructLiteral));
    struct_literal->initializers     = da_init(2, sizeof(AstStructInitializer*));

    // Parse intializers
    next = peek_next_token(parser);
    AstIdentifier *designator = NULL;
    AstExpr *value            = NULL;
    while (next.type != '}' && next.type != TOKEN_END) {
        if (next.type == TOKEN_IDENTIFIER && peek_token(parser, 1).type == '=') {
            eat_token(parser);
            eat_token(parser);

            designator = make_identifier_from_token(parser, next, NULL);
            value      = parse_expression(parser, MIN_PRECEDENCE);
        } else {
            designator = NULL;
            value      = parse_expression(parser, MIN_PRECEDENCE);
        }

        AstStructInitializer *init = ast_allocate(parser, sizeof(AstStructInitializer));
        init->head.type            = AST_STRUCT_INITIALIZER;
        init->head.start           = designator != NULL ? designator->head.start : value->head.start;
        init->head.end             = value->head.end;
        init->designator           = designator;
        init->value                = value;
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

    struct_literal->head.head.type  = AST_STRUCT_LITERAL;
    struct_literal->head.head.start = start_token.start;
    struct_literal->head.head.end   = next.end;
    struct_literal->explicit_type   = explicit_type;

    return struct_literal;
}

AstAccessor *make_accessor(Parser *parser, Token tok) {
    AstAccessor *ac = ast_allocate(parser, sizeof(AstAccessor));
    ac->head.type   = AST_ACCESSOR;
    ac->head.start  = tok.start;
    ac->head.end    = tok.end;
    ac->name        = tok.as_value.value.identifier.name;
    ac->struct_member      = NULL;
    return ac;
}

AstMemberAccess *parse_member_access(Parser *parser) {
    AstMemberAccess *ma = (AstMemberAccess *)(ast_allocate(parser, sizeof(AstMemberAccess)));
    ma->chain = da_init(2, sizeof(AstAccessor *));

    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_IDENTIFIER);
    eat_token(parser);
    Token dot = peek_next_token(parser);
    expect(parser, dot, '.');
    eat_token(parser);
    Token first = peek_next_token(parser);
    expect(parser, first, TOKEN_IDENTIFIER);
    eat_token(parser);

    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, NULL);
    AstAccessor *ac      = make_accessor(parser, first);
    da_append(&ma->chain, ac);

    Token next = peek_next_token(parser);
    while (next.type == '.' || next.type == TOKEN_END) {
        eat_token(parser);

        next = peek_next_token(parser);
        expect(parser, next, TOKEN_IDENTIFIER);
        eat_token(parser);

        AstAccessor *nth = make_accessor(parser, next);
        da_append(&ma->chain, nth);

        next = peek_next_token(parser);
    }

    ma->head.head.type  = AST_MEMBER_ACCESS;
    ma->head.head.start = ident_token.start;
    ma->head.head.end   = peek_token(parser, -1).end;
    ma->ident           = ident;
    return ma;
}

AstFunctionCall *parse_function_call(Parser *parser) {
    AstFunctionCall *call = (AstFunctionCall *)(ast_allocate(parser, sizeof(AstFunctionCall)));
    call->arguments       = da_init(2, sizeof(AstExpr *));

    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_IDENTIFIER); // Should be impossible to fail
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '('); // Should also be impossible to fail
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

        da_append(&call->arguments, arg);
        first_argument_seen = true;
    }

    call->head.head.type  = AST_FUNCTION_CALL;
    call->head.head.start = ident_token.start;
    call->head.head.end   = next.end;
    call->identifer       = make_identifier_from_token(parser, ident_token, NULL); // The type of the identifier will be set later in the typer, so here i just specify void

    return call;
}

AstFunctionDefn *parse_function_defn(Parser *parser) {
    AstFunctionDefn *func_defn = (AstFunctionDefn *)(ast_allocate(parser, sizeof(AstFunctionDefn)));
    func_defn->parameters      = da_init(2, sizeof(AstDeclaration *));

    Token ident_token = peek_next_token(parser);
    expect(parser, ident_token, TOKEN_IDENTIFIER); // Should be impossible to fail
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, TOKEN_DOUBLE_COLON); // Should also be impossible to fail
    eat_token(parser);

    next = peek_next_token(parser);
    expect(parser, next, '('); // Should also be impossible to fail
    eat_token(parser);

    open_scope(&parser->ident_table); // Open a scope, so that the parameters will be pushed down into the scope of the body
    bool first_parameter_seen = false;
    while (true) {
        //
        //  Parse parameter list
        //
        next = peek_next_token(parser);
        if (next.type == ')') {
            eat_token(parser); 
            break;
        }

        // @Cleanup - This should probably use parse_declaration with a flag telling it that its a function parameter
        if (first_parameter_seen) {
            if (next.type != ',') {
                report_error_token(parser, LABEL_ERROR, next, "Expected a ',' between parameters");
                exit(1);    
            }
            eat_token(parser);
            next = peek_next_token(parser);
        }

        if (next.type != TOKEN_IDENTIFIER) {
            report_error_token(parser, LABEL_ERROR, next, "Expected an identifier");
            exit(1);
        }
        Token param_ident = next;
        eat_token(parser);

        next = peek_next_token(parser);
        if (next.type != ':') {
            report_error_token(parser, LABEL_ERROR, next, "Expected a : before the type");
            exit(1);
        }
        eat_token(parser);

        TypeInfo *type = parse_type(parser);

        // Tell 'make_declaration' that the paramters should not be sized into the scope as they live at the callee site
        AstDeclaration *param = make_declaration(parser, param_ident, NULL, type, DECLARATION_TYPED_NO_EXPR | DECLARATION_IS_FUNCTION_PARAMETER);
        da_append(&func_defn->parameters, param);
        first_parameter_seen = true;
    }

    TypeInfo *return_type = primitive_type(PRIMITIVE_VOID);
    
    next = peek_next_token(parser);
    if (next.type == TOKEN_RIGHT_ARROW) {
        eat_token(parser);
        return_type = parse_type(parser);;
    }

    AstBlock *body = parse_block(parser, false); // Here we tell parse_block to explicitly not make a new lexical scope, as we are managing the scope manually in here
    close_scope(&parser->ident_table);

    func_defn->head.type   = AST_FUNCTION_DEFN;
    func_defn->head.start  = ident_token.start;
    func_defn->head.end    = body->head.end;
    func_defn->identifier  = make_identifier_from_token(parser, ident_token, func_defn->return_type);
    func_defn->body        = body;
    func_defn->return_type = return_type;
    symbol_add_function_defn(&parser->function_table, func_defn);

    return func_defn;
}

AstIf *parse_if(Parser *parser) {
    Token if_token = peek_next_token(parser);
    eat_token(parser);

    AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);

    AstBlock *block = parse_block(parser, true);

    AstIf *ast_if = (AstIf *)(ast_allocate(parser, sizeof(AstIf)));
    ast_if->head.type = AST_IF;
    ast_if->head.start = if_token.start;
    ast_if->head.end = block->head.end;
    ast_if->block = block;
    ast_if->condition = condition;
    ast_if->else_ifs = da_init(2, sizeof(AstIf));

    while (true) {
        Token next = peek_next_token(parser);
        if (next.type == TOKEN_ELSE) {
            Token else_token = next;
            eat_token(parser);
            next = peek_next_token(parser);
            if (next.type == TOKEN_IF) {
                eat_token(parser);
                
                AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);
                AstBlock *block    = parse_block(parser, true);

                AstIf else_if = {0};
                else_if.head.type = AST_IF;
                else_if.head.start = else_token.start;
                else_if.head.end = block->head.end;
                else_if.block = block;
                else_if.condition = condition;

                da_append(&ast_if->else_ifs, else_if);

                continue; // Look for more else ifs
            }

            AstBlock *else_block = parse_block(parser, true);
            ast_if->has_else_block = true;
            ast_if->else_block = else_block;
            break;
        }
        break;
    }

    return ast_if;
}

AstTypeof *parse_typeof(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_TYPEOF);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstTypeof *ast_typeof  = (AstTypeof *)(ast_allocate(parser, sizeof(AstTypeof)));
    ast_typeof->head.type  = AST_TYPEOF;
    ast_typeof->head.start = start_token.start;
    ast_typeof->head.end   = next.end;
    ast_typeof->expr       = expr;

    return ast_typeof;
}

AstAssert *parse_assert(Parser *parser) {
    Token start_token = peek_next_token(parser);
    expect(parser, start_token, TOKEN_ASSERT);
    eat_token(parser);

    Token next = peek_next_token(parser);
    expect(parser, next, '(');
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstAssert *assertion = (AstAssert *)(ast_allocate(parser, sizeof(AstAssert)));
    assertion->head.type  = AST_ASSERT;
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


    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);


    next = peek_next_token(parser);
    expect(parser, next, ')');
    eat_token(parser);

    AstPrint *print_stmt = (AstPrint *)(ast_allocate(parser, sizeof(AstPrint)));
    print_stmt->head.type  = AST_PRINT;
    print_stmt->head.start = start_token.start;
    print_stmt->head.end   = next.end;
    print_stmt->expr       = expr;

    return print_stmt;
}

AstDeclaration *parse_declaration(Parser *parser, DeclarationFlags flags) {
    Token ident = peek_next_token(parser);
    expect(parser, ident, TOKEN_IDENTIFIER);
    eat_token(parser);

    Token next = peek_next_token(parser);

    // Infer. e.g. a := b
    if (next.type == TOKEN_COLON_EQUAL) {
        if (flags & DECLARATION_IS_STRUCT_MEMBER) {
            assert(false && "Default parameters not implemented for structs yet");
        }
        eat_token(parser);
        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
        return make_declaration(parser, ident, expr, NULL, flags | DECLARATION_INFER);
    }

    // Typed. 'a : int = b' or 'a : int'
    if (next.type == ':') {
        eat_token(parser);

        TypeInfo *type = parse_type(parser);
        next = peek_next_token(parser);

        if (next.type == '=') {
            if (flags & DECLARATION_IS_STRUCT_MEMBER)      assert(false && "Default struct member values not implemented yet");
            if (flags & DECLARATION_IS_FUNCTION_PARAMETER) assert(false && "Default function arguments not implemented yet");
            
            eat_token(parser);
            AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
            return make_declaration(parser, ident, expr, type, flags | DECLARATION_TYPED);
        } else {
            return make_declaration(parser, ident, NULL, type, flags | DECLARATION_TYPED_NO_EXPR);
        }
    }

    // Constant. e.g. A :: 5;
    if (next.type == TOKEN_DOUBLE_COLON) {
        eat_token(parser);

        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

        return make_declaration(parser, ident, expr, NULL, flags | DECLARATION_CONSTANT);
    }   

    report_error_token(parser, LABEL_ERROR, next, "Invalid declaration");
    exit(1);
}

AstDeclaration *make_declaration(Parser *parser, Token ident_token, AstExpr *expr, TypeInfo *type, DeclarationFlags flags) {
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, type);

    if (flags & DECLARATION_CONSTANT) {
        ident->flags |= IDENTIFIER_IS_CONSTANT;
    }
    if (flags & DECLARATION_IS_STRUCT_MEMBER) {
        // Skip putting identifier into identifier table as structs keeps their own small symbol table of members
    } else {
        Symbol *existing = symbol_add_identifier(&parser->ident_table, ident);
        if (existing != NULL) {
            report_error_token(parser, LABEL_ERROR, ident_token, "Redeclaration of variable '%s'", ident_token.as_value.value.identifier.name);
            report_error_ast(parser, LABEL_NOTE, (AstNode *)(existing->as.identifier), "Here is the previous declaration ...");
            exit(1);
        }
    }

    AstDeclaration *decl    = (AstDeclaration *) ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.type         = AST_DECLARATION;
    decl->head.start        = ident_token.start;
    decl->head.end          = expr != NULL ? expr->head.end : type->head.end;
    decl->identifier        = ident;
    decl->declared_type     = type;
    decl->flags             = flags;
    decl->expr              = expr;

    ident->belongs_to_decl = decl;

    return decl;
}

int align_value(int value, int alignment) {
    int rem = value % alignment;
    if (rem == 0) return value;
    else {
        if (value < 0) return value - (alignment + rem);
        else           return value + (alignment - rem);
    }
}

AstIdentifier *make_identifier_from_string(Parser *parser, const char *name, TypeInfo *type) {
    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.type = AST_IDENTIFIER;
    ident->type      = type;
    ident->name      = (char *)(name);
    ident->length    = strlen(name);

    return ident;
}

AstIdentifier *make_identifier_from_token(Parser *parser, Token ident_token, TypeInfo *type) {
    assert(ident_token.type == TOKEN_IDENTIFIER);

    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.type  = AST_IDENTIFIER;
    ident->head.start = ident_token.start;
    ident->head.end   = ident_token.end;
    ident->type       = type;
    ident->name       = ident_token.as_value.value.identifier.name;
    ident->length     = ident_token.as_value.value.identifier.length;
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
            return 10;
        default:
            printf("%s:%d: compiler-error: Unexpected token type '%s'. Expected token to be an operator\n", __FILE__, __LINE__, token_type_to_str((TokenType)(op)));
            exit(1);
    }
}

bool ends_expression(Token token) {
    if (token.type == TOKEN_END) return true;
    if (token.type == ')') return true;
    if (token.type == ';') return true;
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

    Token next = peek_next_token(parser);
    if (ends_expression(next))         return left;
    if (is_binary_operator(next.type)) vomit_token(parser);

    eat_token(parser);
    while (true) {
        Token next = peek_next_token(parser);

        if (ends_expression(next))          break;
        if (!is_binary_operator(next.type)) break;

        int next_prec = get_precedence((OperatorType)(next.type));
        if (next_prec <= min_prec) {
            break;
        } else {
            eat_token(parser);
            AstExpr *right = parse_expression(parser, next_prec);
            left = make_binary_node(parser, next, left, right);
        }
    }

    return left;
}

AstExpr *parse_leaf(Parser *parser) {
    Token t = peek_next_token(parser);
    if (t.type == TOKEN_BOOLEAN) return make_literal_node(parser, t);
    if (t.type == TOKEN_INTEGER) return make_literal_node(parser, t);
    if (t.type == TOKEN_FLOAT)   return make_literal_node(parser, t);
    if (t.type == TOKEN_STRING)  return make_literal_node(parser, t);

    if (t.type == '(')  {
        eat_token(parser);
        AstExpr *sub_expr = parse_expression(parser, MIN_PRECEDENCE); 
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
        return make_unary_node(parser, t, sub_expr, OP_NOT);
    }

    if (t.type == '-') {
        eat_token(parser);
        int prec = get_precedence(OP_UNARY_MINUS);
        AstExpr *sub_expr = parse_expression(parser, prec); 
        return make_unary_node(parser, t, sub_expr, OP_UNARY_MINUS);
    }

    if (starts_struct_literal(parser)) {
        AstStructLiteral *struct_literal = parse_struct_literal(parser);
        return (AstExpr *)(struct_literal);
    }

    if (t.type == '.' && peek_token(parser, 1).type == TOKEN_IDENTIFIER) {
        Token ident = peek_token(parser, 1);

        eat_token(parser);
        eat_token(parser);

        AstEnumLiteral *eval  = ast_allocate(parser, sizeof(AstEnumLiteral));
        eval->head.head.type  = AST_ENUM_LITERAL;
        eval->head.head.start = t.start;
        eval->head.head.end   = ident.end;
        eval->identifier      = make_identifier_from_token(parser, ident, NULL);

        return (AstExpr *)(eval);
    }

    if (t.type == TOKEN_IDENTIFIER && peek_token(parser, 1).type == '.') {
        AstMemberAccess *ma = parse_member_access(parser);
        return (AstExpr *)(ma);
    }

    if (t.type == TOKEN_IDENTIFIER && peek_token(parser, 1).type == '(') {
        AstFunctionCall *call = parse_function_call(parser);
        return (AstExpr *)(call);
    }

    if (t.type == TOKEN_IDENTIFIER) {
        return make_literal_node(parser, t);
    }

    // @Improvement - Instead of reporting the error here, it would be better to "bubble up" the error
    // so that the full expression that couldn't be parsed can be shown instead of the last token that couldn't
    // be parsed.
    report_error_token(parser, LABEL_ERROR, t, "Invalid expression");
    exit(1);
}

bool starts_struct_literal(Parser *parser) {
    Token next = peek_next_token(parser);
    if (next.type == '.' && peek_token(parser, 1).type == '{') return true;
    if (next.type == TOKEN_IDENTIFIER && peek_token(parser, 1).type == '.' && peek_token(parser, 2).type == '{') return true;
    return false;
}

const char *ast_type_name(AstType ast_type) {
    switch (ast_type) {
        case AST_ERR:                return "ERROR";
        case AST_BLOCK:              return "AST_BLOCK";
        case AST_DECLARATION:        return "AST_DECLARATION";
        case AST_ASSIGNMENT:         return "AST_ASSIGNMENT";
        case AST_PRINT:              return "AST_PRINT";
        case AST_ASSERT:             return "AST_ASSERT";
        case AST_TYPEOF:             return "AST_TYPEOF";
        case AST_RETURN:             return "AST_RETURN";
        case AST_STRUCT:             return "AST_STRUCT";
        case AST_STRUCT_LITERAL:     return "AST_STRUCT_LITERAL";
        case AST_STRUCT_INITIALIZER: return "AST_STRUCT_DESIGNATOR";
        case AST_ENUM:               return "AST_ENUM";
        case AST_ENUMERATOR:         return "AST_ENUMERATOR";
        case AST_ENUM_LITERAL:       return "AST_ENUM_LITERAL";
        case AST_FUNCTION_DEFN:      return "AST_FUNCTION_DEFN";
        case AST_FUNCTION_CALL:      return "AST_FUNCTION_CALL";
        case AST_IF:                 return "AST_IF";
        case AST_FOR:                return "AST_FOR";
        case AST_EXPR:               return "AST_EXPR";
        case AST_RANGE_EXPR:         return "AST_RANGE_EXPR";
        case AST_BINARY:             return "AST_BINARY";
        case AST_UNARY:              return "AST_UNARY";
        case AST_LITERAL:            return "AST_LITERAL";
        case AST_ACCESSOR:           return "AST_ACCESSOR";
        case AST_MEMBER_ACCESS:      return "AST_MEMBER_ACCESS";
        case AST_IDENTIFIER:         return "AST_IDENTIFIER";
        case AST_TYPE_INFO:          return "AST_TYPE_INFO";
    }
    printf("%s:%d: compiler-error: Could not give the name of AST node with type id %d\n", __FILE__, __LINE__, ast_type);
    exit(1);
}

void *ast_allocate(Parser *parser, size_t size) {
    return arena_allocate(&parser->ast_nodes, size);
}

AstExpr *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type) {
    assert(is_unary_operator(token.type));

    AstUnary *unary_op = (AstUnary *) ast_allocate(parser, sizeof(AstUnary));
    unary_op->head.head.type  = AST_UNARY;
    unary_op->head.head.start = token.start;
    unary_op->head.head.end   = token.end;
    unary_op->operator        = op_type;
    unary_op->expr            = expr;

    return (AstExpr *)(unary_op);
}

AstExpr *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs) {
    assert(is_binary_operator(token.type));

    AstBinary *bin_op = (AstBinary *) ast_allocate(parser, sizeof(AstBinary));
    bin_op->head.head.type  = AST_BINARY;
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
    literal->head.head.type  = AST_LITERAL;
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
    return parser->lexer->tokens[parser->current_token_index + lookahead];
}

Token peek_next_token(Parser *parser) {
    return parser->lexer->tokens[parser->current_token_index];
}

bool is_literal(Token token) {
    if (token.type == TOKEN_BOOLEAN)    return true;
    if (token.type == TOKEN_INTEGER)    return true;
    if (token.type == TOKEN_FLOAT)      return true;
    if (token.type == TOKEN_STRING)     return true;
    if (token.type == TOKEN_IDENTIFIER) return true;
    return false;
}

bool is_type_specifier(Token token) {
    return is_primitive_type_token(token) || token.type == TOKEN_IDENTIFIER;
}

bool is_primitive_type_token(Token token) {
    return (token.type >= TOKEN_TYPE_INT && token.type <= TOKEN_TYPE_VOID);
}

bool is_assignment_operator(Token token) {
    if (token.type == '=')                return true;
    if (token.type == TOKEN_PLUS_EQUAL)   return true;
    if (token.type == TOKEN_MINUS_EQUAL)  return true;
    if (token.type == TOKEN_TIMES_EQUAL)  return true;
    if (token.type == TOKEN_DIVIDE_EQUAL) return true;
    return false;
}

void report_error(Parser *parser, Pos start, Pos end, const char *message, ...) {
    va_list args;
    va_start(args, message);

    report_error_helper(parser->lexer, LABEL_ERROR, start, end, message, args);

    va_end(args);
}

void report_error_ast(Parser *parser, const char* label, AstNode *failing_ast, const char *message, ...) {
    va_list args;
    va_start(args, message);

    report_error_helper(parser->lexer, label, failing_ast->start, failing_ast->end, message, args);

    va_end(args);
}

void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...) {
    va_list args;
    va_start(args, message);

    report_error_helper(parser->lexer, label, failing_token.start, failing_token.end, message, args);

    va_end(args);
}