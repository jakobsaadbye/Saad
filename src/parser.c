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
} Parser;

Parser           parser_init();
AstNode         *parse_statement(Parser *parser);
AstDeclaration  *parse_declaration(Parser *parser);
AstAssignment   *parse_assignment(Parser *parser);
AstFunctionDefn *parse_function_defn(Parser *parser);
AstFunctionCall *parse_function_call(Parser *parser);
AstIf           *parse_if(Parser *parser);
AstFor          *parse_for(Parser *parser);
AstPrint        *parse_print(Parser *parser);
AstAssert       *parse_assert(Parser *parser);
AstReturn       *parse_return(Parser *parser);
AstExpr         *parse_expression(Parser *parser, int min_prec);
AstExpr         *parse_range_or_normal_expression(Parser *parser);
AstExpr         *parse_leaf(Parser *parser);
AstDeclaration  *make_declaration(Parser *parser, Token ident_token, DeclarationType decl_type, AstExpr *expr, Token type_token, bool is_function_parameter);
AstExpr         *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
AstExpr         *make_unary_node(Parser *parser, Token token, AstExpr *expr, OperatorType op_type);
AstExpr         *make_literal_node(Parser *parser, Token token);
AstIdentifier   *make_identifier_from_token(Parser *parser, Token ident_token, TypeKind type);
AstIdentifier   *make_identifier_from_string(Parser *parser, const char *name, TypeKind type);
bool identifier_is_equal(const void *key, const void *item);
void *ast_allocate(Parser *parser, size_t size);
void expect(Parser *parser, Token given, TokenType expected_type);
void eat_token(Parser *parser);
void vomit_token(Parser *parser);
Token peek_token(Parser *parser, int lookahead);
Token peek_next_token(Parser *parser);
const char *label_color(const char *label);
bool is_literal(Token token);
bool is_primitive_type(Token token);
bool is_assignment_operator(Token token);
int get_precedence(OperatorType op);
TypeKind token_to_type_kind(Token token);
void report_error_ast(Parser *parser, const char* label, AstNode *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);
void report_error_position(Parser *parser, const char* label, Pos position, const char *message, ...);
unsigned int size_of_type(TypeKind type);


Parser parser_init(Lexer *lexer) {
    Parser parser = {0};
    parser.lexer           = lexer;
    parser.ast_nodes       = arena_make(AST_ALLOCATION_SIZE);
    parser.ident_table     = symbol_table_init();
    parser.function_table  = symbol_table_init();

    return parser;
}

AstCode *parse_top_level_code(Parser *parser) {
    AstCode *code = (AstCode *)(ast_allocate(parser, sizeof(AstCode)));
    code->statements = da_init(8, sizeof(AstNode *));

    Token next = peek_next_token(parser);
    while (true) {
        next = peek_next_token(parser);
        if (next.type == TOKEN_END) break;
        // @Incomplete - Need to limit what statements are allowed at top level scope
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
                report_error_token(parser, LABEL_NONE, start_token, "Here is the block with too many statements");
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
        else if (
            next.type == ':' ||
            next.type == TOKEN_COLON_EQUAL || 
            next.type == TOKEN_DOUBLE_COLON
        ) {
            stmt = (AstNode *)(parse_declaration(parser));
            statement_ends_with_semicolon = true;
        } else if (is_assignment_operator(next)) {
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
            report_error_position(parser, LABEL_ERROR, next.end, "Expected a ';'");
            exit(1);
        }
        eat_token(parser);
    }

    return stmt;
}

AstAssignment *parse_assignment(Parser *parser) {
    Token ident_token = peek_next_token(parser);
    assert(ident_token.type == TOKEN_IDENTIFIER);
    eat_token(parser);

    char *ident_name = ident_token.as_value.identifier.name;
    Symbol *ident_sym = symbol_lookup(&parser->ident_table, ident_name);
    if (ident_sym == NULL) {
        report_error_token(parser, LABEL_ERROR, ident_token, "Undeclared variable '%s'", ident_name);
        exit(1);
    }

    Token op_token = peek_next_token(parser);
    assert(is_assignment_operator(op_token));
    eat_token(parser);

    AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);

    AstAssignment *assign = (AstAssignment *)(ast_allocate(parser, sizeof(AstAssignment)));
    assign->head.type = AST_ASSIGNMENT;
    assign->head.start = ident_token.start;
    assign->head.end = expr->head.end;
    assign->identifier = ident_sym->as_value.identifier;
    assign->op = (AssignOp)(op_token.type);
    assign->expr = expr;

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
        iterator = make_identifier_from_token(parser, next, 0);

        eat_token(parser);
        eat_token(parser);

        iterable = parse_range_or_normal_expression(parser);
    } else {
        iterable = parse_range_or_normal_expression(parser);

        if (iterable->head.type == AST_RANGE_EXPR) {
            iterator = make_identifier_from_string(parser, "it", TYPE_INTEGER);
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
    call->identifer       = make_identifier_from_token(parser, ident_token, TYPE_VOID); // The type of the identifier will be set later in the typer, so here i just specify void

    return call;
}

AstFunctionDefn *parse_function_defn(Parser *parser) {
    AstFunctionDefn *func_defn = (AstFunctionDefn *)(ast_allocate(parser, sizeof(AstFunctionDefn)));
    func_defn->parameters      = da_init(2, sizeof(AstDeclaration *));
    func_defn->return_type     = TYPE_VOID;

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

        next = peek_next_token(parser);
        if (!is_primitive_type(next)) {
            report_error_token(parser, LABEL_ERROR, next, "Expected a type");
            exit(1);
        }
        eat_token(parser);

        Token type_token = next;

        // Tell 'make_declaration' that the paramters should not be sized into the scope as they live at the callee site
        AstDeclaration *param = make_declaration(parser, param_ident, DECLARATION_TYPED_NO_EXPR, NULL, type_token, true);
        da_append(&func_defn->parameters, param);
        first_parameter_seen = true;
    }

    next = peek_next_token(parser);
    if (next.type == TOKEN_RIGHT_ARROW) {
        //
        //  Parse return type
        //
        eat_token(parser);
        next = peek_next_token(parser);
        if (!is_primitive_type(next)) {
            report_error_token(parser, LABEL_ERROR, next, "Expected a return type");
            exit(1);
        }
        eat_token(parser);

        TypeKind return_type = token_to_type_kind(next);
        func_defn->return_type = return_type;
    }

    AstBlock *body = parse_block(parser, false); // Here we tell parse_block to explicitly not make a new lexical scope, as we are managing the scope manually in here
    close_scope(&parser->ident_table);

    func_defn->head.type  = AST_FUNCTION_DEFN;
    func_defn->head.start = ident_token.start;
    func_defn->head.end   = body->head.end;
    func_defn->identifier = make_identifier_from_token(parser, ident_token, func_defn->return_type);
    func_defn->body       = body;
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

AstDeclaration *parse_declaration(Parser *parser) {
    Token ident = peek_next_token(parser);
    expect(parser, ident, TOKEN_IDENTIFIER);
    eat_token(parser);

    Token next = peek_next_token(parser);
    if (next.type == TOKEN_COLON_EQUAL) {
        eat_token(parser);
        AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
        return make_declaration(parser, ident, DECLARATION_INFER, expr, (Token){0}, false); // The type of the declaration and identifier will be infered later
    }

    if (next.type == ':') {
        eat_token(parser);
        next = peek_next_token(parser);

        if (is_primitive_type(next) || next.type == TOKEN_IDENTIFIER) {
            Token type_token = next;

            eat_token(parser);
            next = peek_next_token(parser);
            expect(parser, next, '=');
            eat_token(parser);

            AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
            return make_declaration(parser, ident, DECLARATION_TYPED, expr, type_token, false);
        }

    }

    report_error_token(parser, LABEL_ERROR, next, "Invalid declaration");
    exit(1);
}

TypeKind token_to_type_kind(Token token) {
    assert(is_primitive_type(token) || token.type == TOKEN_IDENTIFIER);

    if (is_primitive_type(token)) {
        if (token.type == TOKEN_TYPE_BOOL)   return TYPE_BOOL;
        if (token.type == TOKEN_TYPE_INT)    return TYPE_INTEGER;
        if (token.type == TOKEN_TYPE_FLOAT)  return TYPE_FLOAT;
        if (token.type == TOKEN_TYPE_STRING) return TYPE_STRING;
    }
    if (token.type == TOKEN_IDENTIFIER) {
        XXX;
    }

    printf("%s:%d: compiler-error: Token '%s' could not be turned into a TypeKind.", __FILE__, __LINE__, token_type_to_str(token.type));
    exit(1);
}

AstDeclaration *make_declaration(Parser *parser, Token ident_token, DeclarationType decl_type, AstExpr *expr, Token type_token, bool is_function_parameter) {
    TypeKind type = TYPE_VOID;
    if (decl_type == DECLARATION_TYPED || decl_type == DECLARATION_TYPED_NO_EXPR) {
        type = token_to_type_kind(type_token);
    }
    AstIdentifier *ident = make_identifier_from_token(parser, ident_token, type);
    Symbol *existing = symbol_add_identifier(&parser->ident_table, ident);
    if (existing != NULL) {
        report_error_token(parser, LABEL_ERROR, ident_token, "Redeclaration of variable '%s'", ident_token.as_value.identifier.name);
        report_error_ast(parser, LABEL_NONE, (AstNode *)(existing), "Here is the previous declaration ...");
        exit(1);
    }

    AstDeclaration *decl = (AstDeclaration *) ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.type         = AST_DECLARATION;
    decl->head.start        = ident_token.start;
    decl->head.end          = type_token.end;
    decl->declaration_type  = decl_type;
    decl->identifier        = ident;
    decl->declared_type     = type;
    decl->is_function_param = is_function_parameter;
    decl->expr              = decl_type == DECLARATION_TYPED_NO_EXPR ? NULL : expr;

    return decl;
}

unsigned int size_of_type(TypeKind type) {
    switch (type) {
        case TYPE_VOID:      return 0;
        case TYPE_BOOL:      return 1;
        case TYPE_INTEGER:   return 4;
        case TYPE_FLOAT:     return 4;
        case TYPE_STRING:    return 8; // pointer to string
        case TYPE_IDENTIFER: XXX;
    }
    printf("%s:%d: compiler-error: Failed to size type with enum number '%d'", __FILE__, __LINE__, type);
    exit(1);
}

AstIdentifier *make_identifier_from_string(Parser *parser, const char *name, TypeKind type) {
    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.type = AST_IDENTIFIER;
    ident->type      = type;
    ident->name      = (char *)(name);
    ident->length    = strlen(name);

    return ident;
}

AstIdentifier *make_identifier_from_token(Parser *parser, Token ident_token, TypeKind type) {
    assert(ident_token.type == TOKEN_IDENTIFIER);

    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.type  = AST_IDENTIFIER;
    ident->head.start = ident_token.start;
    ident->head.end   = ident_token.end;
    ident->type       = type;
    ident->name       = ident_token.as_value.identifier.name;
    ident->length     = ident_token.as_value.identifier.length;

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
    if (token.type == ',') return true;
    if (token.type == TOKEN_DOUBLE_DOT) return true;
    if (token.type == TOKEN_TRIPLE_DOT) return true;

    return false;
}

AstExpr *parse_expression(Parser *parser, int min_prec) {
    AstExpr *left = parse_leaf(parser);

    Token next = peek_next_token(parser);
    if (ends_expression(next)) return left;
    if (is_binary_operator(next.type)) vomit_token(parser);

    eat_token(parser);
    while (true) {
        Token next = peek_next_token(parser);

        if (ends_expression(next)) break;
        if (!is_binary_operator(next.type)) {
            vomit_token(parser);
            break;
        }

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
    if (t.type == TOKEN_BOOLEAN)    return make_literal_node(parser, t);
    if (t.type == TOKEN_INTEGER)    return make_literal_node(parser, t);
    if (t.type == TOKEN_FLOAT)      return make_literal_node(parser, t);
    if (t.type == TOKEN_STRING)     return make_literal_node(parser, t);
    if (t.type == TOKEN_IDENTIFIER) {
        Token next = peek_token(parser, 1);
        if (next.type == '(') {
            AstFunctionCall *call = parse_function_call(parser);
            return (AstExpr *)(call);
        } else {
            return make_literal_node(parser, t);
        }
    }

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

    report_error_token(parser, LABEL_ERROR, t, "Invalid expression");
    exit(1);
}

const char *ast_type_name(AstType ast_type) {
    switch (ast_type) {
        case AST_ERR:                break;
        case AST_BLOCK:              return "AST_BLOCK";
        case AST_DECLARATION:        return "AST_DECLARATION";
        case AST_ASSIGNMENT:         return "AST_ASSIGNMENT";
        case AST_PRINT:              return "AST_PRINT";
        case AST_ASSERT:             return "AST_ASSERT";
        case AST_RETURN:             return "AST_RETURN";
        case AST_FUNCTION_DEFN:      return "AST_FUNCTION_DEFN";
        case AST_FUNCTION_CALL:      return "AST_FUNCTION_CALL";
        case AST_IF:                 return "AST_IF";
        case AST_FOR:                return "AST_FOR";
        case AST_EXPR:               return "AST_EXPR";
        case AST_RANGE_EXPR:         return "AST_RANGE_EXPR";
        case AST_BINARY:             return "AST_BINARY";
        case AST_UNARY:              return "AST_UNARY";
        case AST_LITERAL:            return "AST_LITERAL";
        case AST_IDENTIFIER:         return "AST_IDENTIFIER";
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
    literal->type            = token.type;
    literal->as_value        = token.as_value;

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

bool is_primitive_type(Token token) {
    if (token.type == TOKEN_TYPE_BOOL)   return true;
    if (token.type == TOKEN_TYPE_INT)    return true;
    if (token.type == TOKEN_TYPE_FLOAT)  return true;
    if (token.type == TOKEN_TYPE_STRING) return true;
    return false;
}

bool is_assignment_operator(Token token) {
    if (token.type == '=')                return true;
    if (token.type == TOKEN_PLUS_EQUAL)   return true;
    if (token.type == TOKEN_MINUS_EQUAL)  return true;
    if (token.type == TOKEN_TIMES_EQUAL)  return true;
    if (token.type == TOKEN_DIVIDE_EQUAL) return true;
    return false;
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

void report_error_position(Parser *parser, const char* label, Pos position, const char *message, ...) {
    va_list args;
    va_start(args, message);

    report_error_helper(parser->lexer, label, position, position, message, args);

    va_end(args);
}