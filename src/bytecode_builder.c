#include "parser.c"

//
// EXPERIMENTAL !!!
// This code is put on hold. Firstly we want to have an llvm converter, which i thought the bytecode builder could help
// with, but after more time thinking about, its probably better to convert directly from the ast.
// We might come back to the bytecode builder when we want our own backends to be easier to implement e.g when doing arm backend
// or running code at compile-time!
//    - jsaad 3 okt. 2024
// 

typedef enum Inst {
    INST_INVALID,

    INST_POP,
    INST_PUSH,

    INST_COUNT
} Inst;

typedef struct BC {
    Inst inst;
    int op1;
    int op2;
    int op3;
    int size;
} BC;

typedef struct BytecodeBuilder {
    Parser *parser;

    Arena instructions; // of BC
} BytecodeBuilder;

void bc_emit_binary(AstBinary *bin) {
    return;
}

void bc_emit_expression(AstExpr *expr) {
    switch (expr->head.type) {
    case AST_BINARY: bc_emit_binary((AstBinary *)expr); return;
    default:
        printf("Internal Compiler Error: Unknown case %s in build_expression()", ast_to_str((Ast *)expr));
    }
}

void bc_emit_statement(Ast *ast) {
    switch (ast->type) {
    case AST_EXPR: bc_emit_expression((AstExpr *)ast); return;
    default:
        printf("Internal Compiler Error: Unknown case %s in build_statement()", ast_to_str(ast));
    }
}