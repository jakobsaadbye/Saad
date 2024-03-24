#include "math.h"
#include "parser.c"

float evaluate_expression(AstExpr *expr) {
    if (expr->head.type == AST_BINARY) {
        AstBinary *bin_expr = (AstBinary *)(expr);
        float left  = evaluate_expression(bin_expr->left);
        float right = evaluate_expression(bin_expr->right);

        if (bin_expr->operator == '+') return left + right;
        if (bin_expr->operator == '-') return left - right;
        if (bin_expr->operator == '*') return left * right;
        if (bin_expr->operator == '/') return left / right;
        if (bin_expr->operator == '%') return (float)((int)(left) % (int)(right)); // @Cleanup - Should probably be handled correctly
        if (bin_expr->operator == '^') return powf(left, right);
    }
    else if (expr->head.type == AST_LITERAL) {
        AstLiteral *literal = (AstLiteral *)(expr);
        if (literal->type == TOKEN_INTEGER) return (float)(literal->as_value.integer); // @Cleanup - also not perfect
        if (literal->type == TOKEN_FLOAT)   return literal->as_value.floating;
    }

    printf("%s:%d: error: Got AST node which can not be evaluated as an expression \n", __FILE__, __LINE__); // @Cleanup - Make better error message
    exit(1);
}