// @ErrorImprovement:
//     Right now i am returning a type from statement, but it is never used. I think the only
//     time it makes sense to return the type is in check_expression. I would like to change it
//     so that all check functions just returns a bool for success instead of the current solution
//     of exiting!

#include "parser.c"

typedef struct Typer {
    Parser *parser;
} Typer;

TypeKind check_block(Typer *typer, AstBlock *block, bool open_lexical_scope);
TypeKind check_statement(Typer *typer, AstNode *stmt);
TypeKind check_expression(Typer *typer, AstExpr *expr);
TypeKind check_binary(Typer *typer, AstBinary *binary);
TypeKind check_unary(Typer *typer, AstUnary *unary);
TypeKind check_literal(Typer *typer, AstLiteral *literal);
const char *type_kind_to_str(TypeKind kind);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);

Typer typer_init(Parser *parser) {
    Typer typer = {0};
    typer.parser = parser;

    return typer;
}

TypeKind check_code(Typer *typer, AstCode *code) {
    for (unsigned int i = 0; i < code->statements.count; i++) {
        AstNode *stmt = ((AstNode **)(code->statements.items))[i];
        check_statement(typer, stmt);
    }
    return TYPE_VOID;
}

TypeKind check_block(Typer *typer, AstBlock *block, bool open_lexical_scope) {

    if (open_lexical_scope) enter_scope(&typer->parser->ident_table);
    for (int i = 0; i < block->num_of_statements; i++) {
        check_statement(typer, block->statements[i]);
    }
    if (open_lexical_scope) exit_scope(&typer->parser->ident_table);

    return TYPE_VOID;
}

TypeKind check_declaration(Typer *typer, AstDeclaration *decl) {
    if (decl->declaration_type == DECLARATION_TYPED) {
        check_expression(typer, decl->expr);
        if (decl->expr->evaluated_type != decl->declared_type) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl), "Variable was said to be of type %s, but expression is of type %s", type_kind_to_str(decl->declared_type), type_kind_to_str(decl->expr->evaluated_type));
            exit(1);
        }

        // Identifier should already be typed at the parsing stage.
    }
    if (decl->declaration_type == DECLARATION_INFER) {
        check_expression(typer, decl->expr);
        
        decl->identifier->type = decl->expr->evaluated_type;
        decl->declared_type    = decl->expr->evaluated_type;
    }
    if (decl->declaration_type == DECLARATION_CONSTANT) {
        XXX;
    }
    
    if (decl->is_function_param) {
        // Omit sizing the declaration as it is already done at the call site
    } else {
        typer->parser->ident_table.current_scope->bytes_allocated += size_of_type(decl->identifier->type);
    }

    return TYPE_VOID;
}

TypeKind check_function_call(Typer *typer, AstFunctionCall *call) {
    Symbol *func_symbol   = symbol_lookup(&typer->parser->function_table, call->identifer->name);
    if (func_symbol == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Unknown function '%s'", call->identifer->name);
        exit(1);
    }

    AstFunctionDefn *func_defn = func_symbol->as_value.function_defn;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Mismatch in arguments to function. Function '%s' takes %d parameters, but got %d", func_defn->identifier->name, func_defn->parameters.count, call->arguments.count);
        report_error_ast(typer->parser, LABEL_NONE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        exit(1);
    }

    for (unsigned int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        AstExpr *arg = ((AstExpr **)(call->arguments.items))[i];
        check_expression(typer, arg);

        if (arg->evaluated_type != param->declared_type) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(arg), "Expected argument to be of type '%s', but argument is of type '%s'", type_kind_to_str(param->declared_type), type_kind_to_str(arg->evaluated_type));
            report_error_ast(typer->parser, LABEL_NONE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
            exit(1);
        }
    }

    return func_defn->return_type;
}

TypeKind check_statement(Typer *typer, AstNode *stmt) {
    switch (stmt->type) {
    case AST_DECLARATION: {
        AstDeclaration *decl = (AstDeclaration *)(stmt);
        return check_declaration(typer, decl);
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

        check_block(typer, ast_if->block, true);

        for (unsigned int i = 0; i < ast_if->else_ifs.count; i++) {
            AstIf *else_if = &(((AstIf *)(ast_if->else_ifs.items))[i]);
            check_statement(typer, (AstNode *)(else_if));
        }
        if (ast_if->has_else_block) {
            check_block(typer, ast_if->else_block, true);
        }
        
        return TYPE_VOID;
    }
    case AST_BLOCK: {
        AstBlock *block = (AstBlock *)(stmt);

        check_block(typer, block, true);

        return TYPE_VOID;
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(stmt);
        return check_function_call(typer, call);
    }
    case AST_RETURN: {
        // @Incomplete - Should check if the type of the expression matches the function definition that the return statements resides in.
        AstReturn *ast_return = (AstReturn *)(stmt);
        check_expression(typer, ast_return->expr);

        return TYPE_VOID;
    }
    case AST_FUNCTION_DEFN: {
        AstFunctionDefn *func_defn = (AstFunctionDefn *)(stmt);
        check_block(typer, func_defn->body, true);

        return TYPE_VOID;
    }
    default:
        XXX;
}
}

TypeKind check_expression(Typer *typer, AstExpr *expr) {
    TypeKind result;
    if (expr->head.type == AST_FUNCTION_CALL) result = check_function_call(typer,  (AstFunctionCall *)(expr));
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
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undeclared variable '%s'", ident_name);
            exit(1);
        }

        // Type must have been resolved at this point
        return ident_symbol->as_value.identifier->type;
    };

    printf("%s:%d: compiler-error: Literal '%s' could not be turned into a TypeKind.", __FILE__, __LINE__, token_type_to_str(literal->type));
    exit(1);
}