#include "parser.c"

typedef struct ConstEvaluater {
    Parser *parser;
} ConstEvaluater;


ConstEvaluater const_evaluator_init(Parser *parser) {
    ConstEvaluater ce = {0};
    ce.parser  = parser;
    return ce;
}


AstExpr *simplify_expression(ConstEvaluater *ce, AstExpr *expr);


AstExpr *make_literal_integer(ConstEvaluater *ce, long long value) {
    AstLiteral *lit          = ast_allocate(ce->parser, sizeof(AstLiteral));
    lit->head.head.type      = AST_LITERAL;
    lit->head.evaluated_type = primitive_type(PRIMITIVE_INT);
    lit->kind                = LITERAL_INTEGER;
    lit->as.value.integer    = value;
    return (AstExpr *)(lit);
}

AstExpr *make_literal_float(ConstEvaluater *ce, double value) {
    AstLiteral *lit          = ast_allocate(ce->parser, sizeof(AstLiteral));
    lit->head.head.type      = AST_LITERAL;
    lit->head.evaluated_type = primitive_type(PRIMITIVE_FLOAT);
    lit->kind                = LITERAL_FLOAT;
    lit->as.value.floating   = value;
    return (AstExpr *)(lit);
}

AstExpr *make_literal_boolean(ConstEvaluater *ce, bool value) {
    AstLiteral *lit          = ast_allocate(ce->parser, sizeof(AstLiteral));
    lit->head.head.type      = AST_LITERAL;
    lit->head.evaluated_type = primitive_type(PRIMITIVE_BOOL);
    lit->kind                = LITERAL_BOOLEAN;
    lit->as.value.boolean    = value;
    return (AstExpr *)(lit);
}

AstExpr *simplify_binary(ConstEvaluater *ce, AstBinary *bin) {
    AstExpr *left_expr  = simplify_expression(ce, bin->left);
    AstExpr *right_expr = simplify_expression(ce, bin->right);

    if (left_expr->head.type != AST_LITERAL)  return (AstExpr *)(bin);
    if (right_expr->head.type != AST_LITERAL) return (AstExpr *)(bin);

    AstLiteral *left =  (AstLiteral *)(left_expr);
    AstLiteral *right = (AstLiteral *)(right_expr);

    if (left->kind == LITERAL_IDENTIFIER || right->kind == LITERAL_IDENTIFIER) return (AstExpr *)(bin);

    TokenType op = bin->operator;
    if (op == '+') {
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_INTEGER) return make_literal_integer(ce, left->as.value.integer + right->as.value.integer);
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_FLOAT)     return make_literal_float(ce,   left->as.value.floating + right->as.value.floating);
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_FLOAT)   return make_literal_float(ce,   (double)(left->as.value.integer) + right->as.value.floating); // @Incompelte - double casting here might overflow. Check for it??? 
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_INTEGER)   return make_literal_float(ce,   left->as.value.floating + (double)(right->as.value.integer)); // @Incompelte - double casting here might overflow. Check for it??? 
    }
    if (op == '-') {
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_INTEGER) return make_literal_integer(ce, left->as.value.integer - right->as.value.integer);
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_FLOAT)     return make_literal_float(ce,   left->as.value.floating - right->as.value.floating);
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_FLOAT)   return make_literal_float(ce,   (double)(left->as.value.integer) - right->as.value.floating); // @Incompelte - double casting here might overflow. Check for it??? 
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_INTEGER)   return make_literal_float(ce,   left->as.value.floating - (double)(right->as.value.integer)); // @Incompelte - double casting here might overflow. Check for it??? 
    }
    if (op == '*') {
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_INTEGER) return make_literal_integer(ce, left->as.value.integer * right->as.value.integer);
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_FLOAT)     return make_literal_float(ce,   left->as.value.floating * right->as.value.floating);
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_FLOAT)   return make_literal_float(ce,   (double)(left->as.value.integer) * right->as.value.floating); // @Incompelte - double casting here might overflow. Check for it??? 
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_INTEGER)   return make_literal_float(ce,   left->as.value.floating * (double)(right->as.value.integer)); // @Incompelte - double casting here might overflow. Check for it??? 
    }
    if (op == '/') {
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_INTEGER) return make_literal_integer(ce, left->as.value.integer / right->as.value.integer);
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_FLOAT)     return make_literal_float(ce,   left->as.value.floating / right->as.value.floating);
        if (left->kind == LITERAL_INTEGER && right->kind == LITERAL_FLOAT)   return make_literal_float(ce,   (double)(left->as.value.integer) / right->as.value.floating); // @Incompelte - double casting here might overflow. Check for it??? 
        if (left->kind == LITERAL_FLOAT && right->kind == LITERAL_INTEGER)   return make_literal_float(ce,   left->as.value.floating / (double)(right->as.value.integer)); // @Incompelte - double casting here might overflow. Check for it??? 
    }

    if (op == TOKEN_LOGICAL_OR)  return make_literal_boolean(ce, left->as.value.boolean || right->as.value.boolean);
    if (op == TOKEN_LOGICAL_AND) return make_literal_boolean(ce, left->as.value.boolean && right->as.value.boolean);
    
    XXX;
}

AstExpr *simplify_unary(ConstEvaluater *ce, AstUnary *unary) {
    AstExpr *sub_expr = simplify_expression(ce, unary->expr);

    if (sub_expr->head.type != AST_LITERAL) return (AstExpr *)(sub_expr);

    AstLiteral *lit = (AstLiteral *)(sub_expr);
    if (lit->kind == LITERAL_IDENTIFIER) return (AstExpr *)(sub_expr);

    if (unary->operator == OP_UNARY_MINUS) {
        if (lit->kind == LITERAL_INTEGER) {
            if (lit->as.flags & VALUE_IS_BIGGER_THAN_SIGNED_64_BIT_INTEGER) {
                XXX;
            }
            return make_literal_integer(ce, -lit->as.value.integer);
        }
        if (lit->kind == LITERAL_FLOAT) {
            return make_literal_float(ce, -lit->as.value.floating);
        }

        XXX; // Shouldn't happen
    }
    if (unary->operator == OP_NOT) {
        if (lit->kind == LITERAL_BOOLEAN) {
            return make_literal_boolean(ce, !lit->as.value.boolean);
        }

        XXX; // Shouldn't happen
    }

    printf("Unhandled cases in simplify_unary()\n");
    exit(1);
}

AstExpr *simplify_expression(ConstEvaluater *ce, AstExpr *expr) {
    switch (expr->head.type) {
        case AST_FUNCTION_CALL: return expr; // don't evaluate
        case AST_BINARY:        return simplify_binary(ce, (AstBinary *)(expr));
        case AST_UNARY:         return simplify_unary(ce, (AstUnary *)(expr));
        case AST_LITERAL: {
            AstLiteral *lit = (AstLiteral *)(expr);

            if (lit->kind == LITERAL_IDENTIFIER) {
                AstIdentifier *ident = symbol_lookup(&ce->parser->ident_table, lit->as.value.identifier.name)->as.identifier;
                if (ident->flags & IDENTIFIER_IS_CONSTANT) {
                    return simplify_expression(ce, ident->belongs_to_decl->expr);
                }
            }

            return expr;
        }
        case AST_STRUCT_LITERAL:
        case AST_ENUM_LITERAL: {
            AstEnumLiteral *lit = (AstEnumLiteral *)(expr);
            return make_literal_integer(ce, lit->enum_member->value); // @Note - Potential loss of enum information
        }
        case AST_MEMBER_ACCESS: {
            AstMemberAccess *ma = (AstMemberAccess *)(expr);

            if (ma->access_kind == MEMBER_ACCESS_ENUM) {
                return make_literal_integer(ce, ma->enum_member->value); // @Note - Potential loss of enum information
            }

            return expr;
        }
        case AST_RANGE_EXPR:
        default:
            printf("%s:%d: compiler-error: Unhandled cases in 'simplify_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
            return expr;
    }
}