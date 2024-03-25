#include "parser.c"

typedef struct Typer {
    Parser *parser;
} Typer;

TypeKind check_block(Typer *typer, AstBlock *block);
TypeKind check_statement(Typer *typer, AstNode *stmt);
TypeKind check_expression(Typer *typer, AstExpr *expr);
TypeKind check_binary(Typer *typer, AstBinary *binary);
TypeKind check_unary(Typer *typer, AstUnary *unary);
TypeKind check_literal(Typer *typer, AstLiteral *literal);
const char *type_kind_to_str(TypeKind kind);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);


TypeKind check_code(Typer *typer, AstCode *code) {
    for (unsigned int i = 0; i < code->statements.count; i++) {
        AstNode *stmt = ((AstNode **)(code->statements.items))[i];
        check_statement(typer, stmt);
    }
    return TYPE_VOID;
}

TypeKind check_block(Typer *typer, AstBlock *block) {
    for (int i = 0; i < block->num_of_statements; i++) {
        check_statement(typer, block->statements[i]);
    }
    return TYPE_VOID;
}

TypeKind check_statement(Typer *typer, AstNode *stmt) {
    switch (stmt->type) {
        case AST_DECLARATION: {
            AstDeclaration *decl   = (AstDeclaration *)(stmt);
            TypeKind expr_type     = check_expression(typer, (decl->expr));
            decl->identifier->type = expr_type;
            decl->evaluated_type   = expr_type;

            typer->parser->block_byte_size += size_of_type(expr_type);

            return expr_type;
        }
        case AST_PRINT: {
            AstPrint *print = (AstPrint *)(stmt);
            check_expression(typer, print->expr);
            return TYPE_VOID;
        }
        case AST_IF: {
            AstIf *ast_if = (AstIf *)(stmt);
            TypeKind type_of_expr = check_expression(typer, ast_if->condition);
            if (type_of_expr != TYPE_BOOL) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_if->condition), "Expression needs to be of type 'bool', but expression evaluated to type '%s'", type_kind_to_str(type_of_expr));
                exit(1);
            }

            check_block(typer, ast_if->block);

            for (unsigned int i = 0; i < ast_if->else_ifs.count; i++) {
                AstIf *else_if = &(((AstIf *)(ast_if->else_ifs.items))[i]);
                check_statement(typer, (AstNode *)(else_if));
            }
            if (ast_if->has_else_block) {
                check_block(typer, ast_if->else_block);
            }
            
            return TYPE_VOID;
        }
        case AST_BLOCK: {
            AstBlock *block = (AstBlock *)(stmt);

            check_block(typer, block);

            return TYPE_VOID;
        }
        default:
            XXX;
    }
}

TypeKind check_expression(Typer *typer, AstExpr *expr) {
    TypeKind result;
    if (expr->head.type == AST_BINARY)  result = check_binary(typer,  (AstBinary *)(expr));
    if (expr->head.type == AST_UNARY)   result = check_unary(typer,   (AstUnary *)(expr));
    if (expr->head.type == AST_LITERAL) result = check_literal(typer, (AstLiteral *)(expr));

    if (result) {
        expr->evaluated_type = result;
        return result;
    }

    printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
    exit(1);
}

TypeKind check_binary(Typer *typer, AstBinary *binary) {
    TypeKind lhs = check_expression(typer, binary->left);
    TypeKind rhs = check_expression(typer, binary->right);

    if (strchr("+-/*^", binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return TYPE_INTEGER;
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return TYPE_FLOAT;
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return TYPE_FLOAT;
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return TYPE_FLOAT;
    }
    if (binary->operator == '%') {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return TYPE_INTEGER;
    }
    if (is_boolean_operator(binary->operator)) {
        if (lhs == TYPE_BOOL && rhs == TYPE_BOOL)       return TYPE_BOOL;
    }
    if (is_comparison_operator(binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return TYPE_BOOL;
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return TYPE_BOOL;
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return TYPE_BOOL;
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return TYPE_BOOL;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(binary), "Type '%s' and '%s' is not compatible with operator %s\n", type_kind_to_str(lhs), type_kind_to_str(rhs), token_type_to_str(binary->operator));
    exit(1);
}

TypeKind check_unary(Typer *typer, AstUnary *unary) {
    TypeKind expr_type = check_expression(typer, unary->expr);

    if (unary->operator == '!') {
        if (expr_type == TYPE_BOOL) return TYPE_BOOL;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(unary), "Type '%s' is not compatible with operator %s\n", type_kind_to_str(expr_type), token_type_to_str(unary->operator));
    exit(1);
}

bool is_comparison_operator(TokenType op) {
    if (op == '<')                 return true;
    if (op == '>')                 return true;
    if (op == TOKEN_GREATER_EQUAL) return true;
    if (op == TOKEN_LESS_EQUAL)    return true;
    if (op == TOKEN_DOUBLE_EQUAL)  return true;
    if (op == TOKEN_NOT_EQUAL)     return true;
    return false;
}

bool is_boolean_operator(TokenType op) {
    if (op == TOKEN_LOGICAL_AND)   return true;
    if (op == TOKEN_LOGICAL_OR)    return true;
    return false;
}

TypeKind check_literal(Typer *typer, AstLiteral *literal) {
    if (literal->type == TOKEN_INTEGER) return TYPE_INTEGER;
    if (literal->type == TOKEN_FLOAT)   return TYPE_FLOAT;
    if (literal->type == TOKEN_STRING)  return TYPE_STRING;
    if (literal->type == TOKEN_BOOLEAN) return TYPE_BOOL;
    if (literal->type == TOKEN_IDENTIFIER) {
        char *ident_name     = literal->as_value.identifier.name;
        Symbol *ident_symbol = symbol_lookup(&typer->parser->ident_table, ident_name);
        if (ident_symbol == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undefined variable '%s'", ident_name);
            exit(1);
        }

        // Type must have been resolved at this point
        return ident_symbol->as_value.identifier->type;
    };

    printf("%s:%d: compiler-error: Literal '%s' could not be turned into a TypeKind.", __FILE__, __LINE__, token_type_to_str(literal->type));
    exit(1);
}