#include "parser.c"

typedef struct LLVMConverter {
    Parser *parser;
} LLVMConverter;

void llvm_emit_binary(AstBinary *bin) {
    return;
}

void llvm_emit_expression(AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_BINARY: bc_emit_binary((AstBinary *)expr); return;
    default:
        printf("Internal Compiler Error: Unknown case %s in llvm_emit_expression()", ast_to_str((Ast *)expr));
    }
}

void llvm_emit_statement(Ast *ast) {
    switch (ast->kind) {
    case AST_EXPR: bc_emit_expression((AstExpr *)ast); return;
    default:
        printf("Internal Compiler Error: Unknown case %s in llvm_emit_statement()", ast_to_str(ast));
    }
}