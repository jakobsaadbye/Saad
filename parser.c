#include "assert.h"
#include "lexer.c"
#include "lib/table.c"
#include "lib/array.c"

#define AST_ALLOCATION_SIZE 16384
#define IDENT_TABLE_SIZE    128
#define MAX_STATEMENTS_WITHIN_BLOCK 64

#define MAX_PRECEDENCE 99
#define MIN_PRECEDENCE -99

typedef struct Parser {
    Lexer *lexer;

    Arena ast_nodes;

    int current_token_index;
    int current_ast_node_index;

    HashTable ident_table;

    unsigned int block_byte_size;

} Parser;

typedef struct AstNode AstNode;
typedef struct AstExpr AstExpr;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstBinary AstBinary;
typedef struct AstUnary AstUnary;
typedef struct AstLiteral AstLiteral;
typedef struct AstIdentifier AstIdentifier;

typedef enum AstType {
    AST_ERR,
    AST_BLOCK,
    AST_DECLARATION,
    AST_PRINT,
    AST_IF,
    AST_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_IDENTIFIER,
    AST_LITERAL,
} AstType;

typedef struct AstNode {
    AstType type;

    Pos start;
    Pos end;
} AstNode;

typedef enum TypeKind {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_IDENTIFER,
} TypeKind;

typedef enum DeclarationType {
    DECLARATION_TYPED,          // ex. a : int = b
    DECLARATION_INFER,          // ex. a := b
    DECLARATION_CONSTANT        // ex. a :: 3 @ToDo
} DeclarationType;

typedef struct AstDeclaration {
    AstNode head;

    DeclarationType declaration_type;
    AstIdentifier  *identifier;
    TypeKind        evaluated_type;
    AstExpr        *expr;

} AstDeclaration;

typedef struct AstBlock {
    AstNode head;

    AstNode  *statements[MAX_STATEMENTS_WITHIN_BLOCK];
    int       num_of_statements;
} AstBlock;

typedef struct AstPrint {
    AstNode head;

    AstExpr *expr;
} AstPrint;

typedef struct AstIf {
    AstNode head;

    AstExpr *condition;
    AstBlock *block;
    AstBlock *else_block; // Optional
    DynamicArray else_ifs;
    bool has_else_block;
} AstIf;

typedef struct AstExpr {
    AstNode head;
    TypeKind evaluated_type;
} AstExpr;

typedef struct AstBinary {
    AstExpr head;

    TokenType operator;
    AstExpr  *left;
    AstExpr  *right;
} AstBinary;

typedef struct AstUnary {
    AstExpr head;

    TokenType operator;
    AstExpr  *expr;
} AstUnary;

typedef struct AstLiteral {
    AstExpr head;

    TokenType type;
    As_value as_value;
} AstLiteral;

typedef struct AstIdentifier {
    AstNode head;

    TypeKind type;
    char *name;
    int   length;

    int stack_offset;
} AstIdentifier;

Parser          parser_init();
AstExpr        *parse_expression(Parser *parser, int min_prec);
AstExpr        *parse_leaf(Parser *parser);
AstNode        *parse_statement(Parser *parser);
AstDeclaration *parse_declaration(Parser *parser);
AstIf          *parse_if(Parser *parser);
AstPrint       *parse_print(Parser *parser);
AstDeclaration *make_declaration(Parser *parser, Token ident_token, DeclarationType decl_type, AstExpr *expr, TypeKind type);
AstExpr        *make_binary_node(Parser *parser, Token token, AstExpr *lhs, AstExpr *rhs);
AstExpr        *make_unary_node(Parser *parser, Token token, AstExpr *expr);
AstExpr        *make_literal_node(Parser *parser, Token token);
AstIdentifier  *make_identifier_node(Parser *parser, Token ident_token, TypeKind type);
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
int get_precedence(TokenType op);
TypeKind token_to_type_kind(Token token);
void report_error_ast(Parser *parser, const char* label, AstNode *failing_ast, const char *message, ...);
void report_error_token(Parser *parser, const char* label, Token failing_token, const char *message, ...);
void report_error_position(Parser *parser, const char* label, Pos position, const char *message, ...);
unsigned int size_of_type(TypeKind type);


Parser parser_init(Lexer *lexer) {
    Parser parser = {0};
    parser.lexer           = lexer;
    parser.ast_nodes       = arena_make(AST_ALLOCATION_SIZE);
    parser.ident_table     = hash_table_make(IDENT_TABLE_SIZE, identifier_is_equal);
    parser.block_byte_size = 0;

    return parser;
}

bool identifier_is_equal(const void *key, const void *item) {
    const char    *name  = (const char *)   (key);
    AstIdentifier *ident = (AstIdentifier *)(item);
    return strcmp(name, ident->name) == 0;
}

AstBlock *parse_block(Parser *parser) {
    AstBlock *block = (AstBlock *)(ast_allocate(parser, sizeof(AstBlock)));

    Token start_token = peek_next_token(parser);
    expect(parser, start_token, '{');
    eat_token(parser);

    block->statements[0]     = (AstNode *)(parse_statement(parser));
    block->num_of_statements = 1;

    int i = 1;
    Token next_token = peek_next_token(parser);
    while (true) {
        next_token = peek_next_token(parser);
        if (next_token.type == '}') break;
        else {
            if (i >= MAX_STATEMENTS_WITHIN_BLOCK) {
                printf("%s:%d: compiler-error: Reached maximum number of statements allowed within a single block which is %d", __FILE__, __LINE__, MAX_STATEMENTS_WITHIN_BLOCK);
                report_error_token(parser, LABEL_NONE, start_token, "Here is the block with too many statements");
                return NULL;
            }

            block->statements[i] = (AstNode *)(parse_statement(parser));
            i++;
        }
    }

    expect(parser, next_token, '}');
    eat_token(parser);


    block->head.type         = AST_BLOCK;  
    block->head.start        = start_token.start;
    block->head.end          = next_token.end;
    block->num_of_statements = i;

    return block;
}

AstNode *parse_statement(Parser *parser) {
    bool statement_ends_with_semicolon = false;
    AstNode *stmt = NULL;
    
    Token token = peek_next_token(parser);

    if (token.type == TOKEN_IDENTIFIER) {
        Token next = peek_token(parser, 1);
        if (
            next.type == ':' ||
            next.type == TOKEN_COLON_EQUAL || 
            next.type == TOKEN_DOUBLE_COLON
        ) {
            stmt = (AstNode *)(parse_declaration(parser));
            statement_ends_with_semicolon = true;
        } 
    }
    else if (token.type == TOKEN_PRINT) {
        stmt = (AstNode *)(parse_print(parser));
        statement_ends_with_semicolon = true;
    }
    else if (token.type == TOKEN_IF) {
        stmt = (AstNode *)(parse_if(parser));
        statement_ends_with_semicolon = false;
    }
    else {
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

AstIf *parse_if(Parser *parser) {
    Token if_token = peek_next_token(parser);
    eat_token(parser);

    AstExpr *condition = parse_expression(parser, MIN_PRECEDENCE);
    AstBlock *block    = parse_block(parser);

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
                AstBlock *block    = parse_block(parser);

                AstIf else_if = {0};
                else_if.head.type = AST_IF;
                else_if.head.start = else_token.start;
                else_if.head.end = block->head.end;
                else_if.block = block;
                else_if.condition = condition;

                da_append(&ast_if->else_ifs, else_if);

                continue; // Look for more else ifs
            }

            AstBlock *else_block = parse_block(parser);
            ast_if->has_else_block = true;
            ast_if->else_block = else_block;
            break;
        }
        break;
    }

    return ast_if;
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
        return make_declaration(parser, ident, DECLARATION_INFER, expr, TYPE_VOID);
    }

    if (next.type == ':') {
        eat_token(parser);
        next = peek_next_token(parser);

        if (is_primitive_type(next) || next.type == TOKEN_IDENTIFIER) {
            TypeKind type = token_to_type_kind(next);

            eat_token(parser);
            next = peek_next_token(parser);
            expect(parser, next, '=');
            eat_token(parser);

            AstExpr *expr = parse_expression(parser, MIN_PRECEDENCE);
            return make_declaration(parser, ident, DECLARATION_TYPED, expr, type);
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

AstDeclaration *make_declaration(Parser *parser, Token ident_token, DeclarationType decl_type, AstExpr *expr, TypeKind type) {
    AstIdentifier *ident = make_identifier_node(parser, ident_token, type);

    AstDeclaration *decl = (AstDeclaration *) ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.type        = AST_DECLARATION;
    decl->head.start       = ident_token.start;
    decl->head.end         = expr->head.end;
    decl->declaration_type = decl_type;
    decl->identifier       = ident;
    decl->evaluated_type   = type;
    decl->expr             = expr;

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

AstIdentifier *make_identifier_node(Parser *parser, Token ident_token, TypeKind type) {
    assert(ident_token.type == TOKEN_IDENTIFIER);

    AstIdentifier *existing = (AstIdentifier *)(hash_table_get(&parser->ident_table, ident_token.as_value.identifier.name));
    if (existing != NULL) {
        report_error_token(parser, LABEL_ERROR, ident_token, "Variable '%s' is already defined", ident_token.as_value.identifier.name);
        report_error_ast(parser, LABEL_NONE, (AstNode *)(existing), "Here is the previous declaration ...");
        exit(1);
    }

    AstIdentifier *ident = (AstIdentifier *) ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.type  = AST_IDENTIFIER;
    ident->head.start = ident_token.start;
    ident->head.end   = ident_token.end;
    ident->type       = type;
    ident->name       = ident_token.as_value.identifier.name;
    ident->length     = ident_token.as_value.identifier.length;

    hash_table_add(&parser->ident_table, ident->name, ident);

    return ident;
}

void expect(Parser *parser, Token given, TokenType expected_type) {
    if (given.type != expected_type) {
        report_error_token(parser, LABEL_ERROR, given, "Expected '%s', but got '%s'", token_type_to_str(expected_type), token_type_to_str(given.type));
        exit(1);
    }
    // Everything's cool dude
}

int get_precedence(TokenType op) {
    switch ((int)(op)) {
        case TOKEN_LOGICAL_OR: 
            return 1;
        case TOKEN_LOGICAL_AND: 
            return 2;
        case TOKEN_DOUBLE_EQUAL:
        case TOKEN_NOT_EQUAL: 
            return 3;
        case (TokenType)('>'): 
        case (TokenType)('<'):
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS_EQUAL:
            return 5;
        case (TokenType)('-'):
        case (TokenType)('+'):
            return 6;
        case (TokenType)('*'):
        case (TokenType)('/'):
        case (TokenType)('%'):
            return 7;
        case (TokenType)('^'):
            return 8;
        case (TokenType)('!'):
            return 9;
        default:
            printf("%s:%d: compiler-error: Unexpected token type '%s'. Expected token to be an operator\n", __FILE__, __LINE__, token_type_to_str(op));
            exit(1);
    }
}

AstExpr *parse_expression(Parser *parser, int min_prec) {
    AstExpr *left = parse_leaf(parser);

    Token next = peek_next_token(parser);
    if (next.type == TOKEN_END) return left;
    if (next.type == ')')       return left;
    if (next.type == ';')       return left;
    if (next.type == '{')       return left;
    if (is_binary_operator(next.type)) vomit_token(parser);

    eat_token(parser);
    while (true) {
        Token next = peek_next_token(parser);

        if (next.type == TOKEN_END) break;
        if (next.type == ')')  break;
        if (next.type == ';')  break;
        if (next.type == '{')  break;
        if (!is_binary_operator(next.type)) {
            vomit_token(parser);
            break;
        }

        int next_prec = get_precedence(next.type);
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
    if (t.type == TOKEN_IDENTIFIER) return make_literal_node(parser, t);

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
        int prec          = get_precedence('!');
        AstExpr *sub_expr = parse_expression(parser, prec); 
        return make_unary_node(parser, t, sub_expr);
    }

    report_error_token(parser, LABEL_ERROR, t, "Invalid expression");
    exit(1);
}

const char *ast_type_name(AstType ast_type) {
    switch (ast_type) {
        case AST_ERR:         break;
        case AST_BLOCK:       return "AST_BLOCK";
        case AST_DECLARATION: return "AST_DECLARATION";
        case AST_PRINT:       return "AST_PRINT";
        case AST_IF:          return "AST_IF";
        case AST_EXPR:        return "AST_EXPR";
        case AST_BINARY:      return "AST_BINARY";
        case AST_UNARY:       return "AST_UNARY";
        case AST_LITERAL:     return "AST_LITERAL";
        case AST_IDENTIFIER:  return "AST_IDENTIFIER";
    }
    printf("%s:%d: compiler-error: Could not give the name of AST node with type id %d\n", __FILE__, __LINE__, ast_type);
    exit(1);
}

void *ast_allocate(Parser *parser, size_t size) {
    return arena_allocate(&parser->ast_nodes, size);
}

AstExpr *make_unary_node(Parser *parser, Token token, AstExpr *expr) {
    assert(is_unary_operator(token.type));

    AstUnary *unary_op = (AstUnary *) ast_allocate(parser, sizeof(AstUnary));
    unary_op->head.head.type  = AST_UNARY;
    unary_op->head.head.start = token.start;
    unary_op->head.head.end   = token.end;
    unary_op->operator        = token.type;
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