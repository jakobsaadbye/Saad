#include "parser.c"

typedef struct Typer {
    Parser *parser;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
} Typer;

void check_block(Typer *typer, AstBlock *block, bool open_lexical_scope);
void check_statement(Typer *typer, AstNode *stmt);
TypeInfo check_expression(Typer *typer, AstExpr *expr, TypeInfo lhs_type);
TypeInfo check_binary(Typer *typer, AstBinary *binary);
TypeInfo check_unary(Typer *typer, AstUnary *unary);
TypeInfo check_literal(Typer *typer, AstLiteral *literal);
TypeInfo check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, TypeInfo lhs_type);
char *type_to_str(TypeInfo type);
bool types_match(TypeInfo a, TypeInfo b);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);

Typer typer_init(Parser *parser) {
    Typer typer = {0};
    typer.parser = parser;
    typer.enclosing_function = NULL;

    return typer;
}

void check_code(Typer *typer, AstCode *code) {
    for (unsigned int i = 0; i < code->statements.count; i++) {
        AstNode *stmt = ((AstNode **)(code->statements.items))[i];
        check_statement(typer, stmt);
    }
}

void check_block(Typer *typer, AstBlock *block, bool open_lexical_scope) {

    if (open_lexical_scope) enter_scope(&typer->parser->ident_table);
    for (int i = 0; i < block->num_of_statements; i++) {
        check_statement(typer, block->statements[i]);
    }
    if (open_lexical_scope) exit_scope(&typer->parser->ident_table);
}

void check_declaration(Typer *typer, AstDeclaration *decl) {
    switch (decl->declaration_type) {
        case DECLARATION_TYPED: {
            check_expression(typer, decl->expr, decl->declared_type);
            if (!types_match(decl->expr->evaluated_type, decl->declared_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl), "Variable was said to be of type %s, but expression is of type %s", type_to_str(decl->declared_type), type_to_str(decl->expr->evaluated_type));
                exit(1);
            }

            // Identifier should already be typed at the parsing stage.
            break;
        }
        case DECLARATION_TYPED_NO_EXPR: {
            // @Incomplete - We still gotta check that the type declared exists
            break;
        }
        case DECLARATION_INFER: {
            TypeInfo expr_type = check_expression(typer, decl->expr, type(TYPE_UNTYPED));
            
            decl->identifier->type = expr_type;
            decl->declared_type    = expr_type;
            break;
        }
        case DECLARATION_CONSTANT: {
            XXX;
        }
    }
    
    if (decl->flags & DF_IS_FUNCTION_PARAMETER || decl->flags & DF_IS_STRUCT_MEMBER) {
        // Omit sizing the declaration as it is already done at this point
    } else {
        typer->parser->ident_table.current_scope->bytes_allocated += size_of_type(decl->identifier->type);
    }
}

bool types_match(TypeInfo a, TypeInfo b) {
    if (is_primitive_type(a.kind) && is_primitive_type(b.kind)) {
        return a.kind == b.kind;
    } else {
        XXX;
    }
 
    return false;
}

TypeInfo check_function_call(Typer *typer, AstFunctionCall *call) {
    Symbol *func_symbol   = symbol_lookup(&typer->parser->function_table, call->identifer->name);
    if (func_symbol == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Unknown function '%s'", call->identifer->name);
        exit(1);
    }

    AstFunctionDefn *func_defn = func_symbol->as_value.function_defn;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Mismatch in number of arguments. Function '%s' takes %d %s, but %d were supplied", func_defn->identifier->name, func_defn->parameters.count, func_defn->parameters.count == 1 ? "parameter" : "parameters", call->arguments.count);
        report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        exit(1);
    }

    for (unsigned int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        AstExpr *arg = ((AstExpr **)(call->arguments.items))[i];
        check_expression(typer, arg, type(TYPE_UNTYPED));

        if (!types_match(arg->evaluated_type, param->declared_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(arg), "Type mismatch. Expected argument to be of type '%s', but argument is of type '%s'", type_to_str(param->declared_type), type_to_str(arg->evaluated_type));
            report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
            exit(1);
        }
    }

    return func_defn->return_type;
}

void check_statement(Typer *typer, AstNode *stmt) {
    switch (stmt->type) {
    case AST_DECLARATION: {
        AstDeclaration *decl = (AstDeclaration *)(stmt);
        check_declaration(typer, decl);
        return;
    }
    case AST_ASSIGNMENT: {
        AstAssignment *assign = (AstAssignment *)(stmt);

        TypeInfo expr_type = check_expression(typer, assign->expr, assign->identifier->type);

        if (!types_match(assign->identifier->type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Type mismatch. Trying to assign expression of type '%s' to variable of type '%s'", type_to_str(expr_type), type_to_str(assign->identifier->type));
            exit(1);
        }

        if (assign->op != '=') {
            if (expr_type.kind != TYPE_INTEGER && expr_type.kind != TYPE_FLOAT) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Compound operators are only allowed for int and float types. Given type was '%s'\n", type_to_str(expr_type));
                exit(1);
            }
        }

        return;
    }
    case AST_PRINT: {
        AstPrint *print = (AstPrint *)(stmt);
        check_expression(typer, print->expr, type(TYPE_UNTYPED));
        return;
    }
    case AST_ASSERT: {
        AstAssert *assertion = (AstAssert *)(stmt);
        TypeInfo expr_type = check_expression(typer, assertion->expr, type(TYPE_UNTYPED));
        if (expr_type.kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assertion), "Type mismatch. Expected expression to be of type bool, but expression is of type '%s'", type_to_str(expr_type));
            exit(1);
        }

        return;
    }
    case AST_IF: {
        AstIf *ast_if = (AstIf *)(stmt);
        TypeInfo condition_type = check_expression(typer, ast_if->condition, type(TYPE_UNTYPED));
        if (condition_type.kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_if->condition), "Expression needs to be of type 'bool', but expression evaluated to type '%s'", type_to_str(condition_type));
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
        return;
    }
    case AST_BLOCK: {
        AstBlock *block = (AstBlock *)(stmt);
        check_block(typer, block, true);
        return;
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(stmt);
        check_function_call(typer, call);
        return;
    }
    case AST_RETURN: {
        AstReturn *ast_return = (AstReturn *)(stmt);
        AstFunctionDefn *ef = typer->enclosing_function;
        if (ef == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_return), "Attempting to return outside of a function");
            exit(1);
        }

        check_expression(typer, ast_return->expr, ef->return_type);

        if (!types_match(ast_return->expr->evaluated_type, ef->return_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_return), "Type mismatch. Type of expression in return is %s, but function '%s' has return type %s", type_to_str(ast_return->expr->evaluated_type), ef->identifier->name, type_to_str(ef->return_type));
            exit(1);
        }

        ast_return->enclosing_function = ef;
        return;
    }
    case AST_FUNCTION_DEFN: {
        AstFunctionDefn *func_defn = (AstFunctionDefn *)(stmt);
        typer->enclosing_function = func_defn;
        check_block(typer, func_defn->body, true);
        typer->enclosing_function = NULL;
        return;
    }
    case AST_STRUCT: {
        AstStruct *ast_struct = (AstStruct *)(stmt);

        // @Incomplete - Check all the entries in the symbol table 
        // for (unsigned int i = 0; i < ast_struct->members.count; i++) {
        //     AstDeclaration *member = ((AstDeclaration **)(ast_struct->members.items))[i];
        //     check_declaration(typer, member);
        // }
        return;
    }
    case AST_FOR: {
        AstFor *ast_for = (AstFor *)(stmt);

        if (ast_for->iterable->head.type == AST_RANGE_EXPR) {
            AstRangeExpr *range = (AstRangeExpr *)(ast_for->iterable);
            
            TypeInfo type_start = check_expression(typer, range->start, type(TYPE_UNTYPED));
            TypeInfo type_end = check_expression(typer, range->end, type(TYPE_UNTYPED));

            if (type_start.kind != TYPE_INTEGER) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(range->start), "Range expressions must have integer type as bounds. Got a %s", type_to_str(type_start));
                exit(1);
            }
            if (type_end.kind != TYPE_INTEGER) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(range->end), "Range expressions must have integer type as bounds. Got a %s", type_to_str(type_end));
                exit(1);
            }

            ast_for->iterator->type = type(TYPE_INTEGER);

            // allocate space for the iterator, start and end
            typer->parser->ident_table.current_scope->bytes_allocated += size_of_type(ast_for->iterator->type) * 3;
        } else {
            // ToDo: Implement for unnamed for-statements
            XXX;
        }

        check_block(typer, ast_for->body, true);

        return;
    }

    default:
        XXX;
    }
}

TypeInfo check_expression(Typer *typer, AstExpr *expr, TypeInfo lhs_type) {
    TypeInfo result = {0};
    if (expr->head.type == AST_FUNCTION_CALL)       result = check_function_call(typer,  (AstFunctionCall *)(expr));
    else if (expr->head.type == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr));
    else if (expr->head.type == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr));
    else if (expr->head.type == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr));
    else if (expr->head.type == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), lhs_type);
    else if (expr->head.type == AST_RANGE_EXPR)     result = type(TYPE_INTEGER);
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
        exit(1);
    }

    expr->evaluated_type = result;
    return result;
}

TypeInfo check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, TypeInfo lhs_type) {
    AstStruct *struct_defn = NULL;
    if (struct_literal->explicit_type.kind != TYPE_UNTYPED) {
        // Explicit type is used
        if (is_primitive_type(struct_literal->explicit_type.kind)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(struct_literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(struct_literal->explicit_type));
            exit(1);
        }
        Symbol *existing = symbol_lookup(&typer->parser->type_table, struct_literal->explicit_type.as.identifier);
        if (existing == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(struct_literal), "Unknown type '%s'", type_to_str(struct_literal->explicit_type));
            exit(1);
        }

        struct_defn = existing->as_value.struct_defn;
    }
    else {
        // Type is inferred from the type on the declaration
        if (lhs_type.kind == TYPE_UNTYPED) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(struct_literal), "Type of struct literal could not be infered. Put atleast a type on the declaration or make an explict type on the struct literal");
            exit(1);
        }
        if (is_primitive_type(lhs_type.kind)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(struct_literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(lhs_type));
            exit(1);
        }

        Symbol *existing = symbol_lookup(&typer->parser->type_table, lhs_type.as.identifier);
        if (existing == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(struct_literal), "Unknown type '%s'", type_to_str(struct_literal->explicit_type));
            exit(1);
        }

        struct_defn = existing->as_value.struct_defn;
    }

    for (unsigned int i = 0; i < struct_literal->designators.count; i++) {
        AstStructDesignator *sd = ((AstStructDesignator **)(struct_literal->designators.items))[i];

        Symbol *member_sym = symbol_lookup(&struct_defn->members, sd->member->name);
        if (member_sym == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(sd->member), "'%s' is not a member of '%s'", sd->member->name, struct_defn->identifier->name);
            report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn), "Here is the definition of '%s'", struct_defn->identifier->name);
            exit(1);
        }

        AstDeclaration *struct_member = member_sym->as_value.struct_member;
        TypeInfo member_type = struct_member->declared_type;
        TypeInfo value_type = check_expression(typer, sd->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
        if (!types_match(member_type, value_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(sd->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", struct_member->identifier->name, type_to_str(struct_member->declared_type), type_to_str(value_type));
            exit(1);
        }
    }

    if (struct_literal->explicit_type.kind != TYPE_UNTYPED) {
        return struct_literal->explicit_type;
    } else {
        return lhs_type;
    }
}

TypeInfo check_binary(Typer *typer, AstBinary *binary) {
    TypeInfo ti_lhs = check_expression(typer, binary->left, type(TYPE_UNTYPED));
    TypeInfo ti_rhs = check_expression(typer, binary->right, type(TYPE_UNTYPED));

    TypeKind lhs = ti_lhs.kind;
    TypeKind rhs = ti_rhs.kind;
    if (!is_primitive_type(lhs) || !is_primitive_type(rhs)) {
        assert(false && "Binary operation between two user-defined types (structs and enums) is not yet supported!");
        exit(1);
    }
    if (strchr("+-/*^", binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return type(TYPE_INTEGER);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return type(TYPE_FLOAT);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return type(TYPE_FLOAT);
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return type(TYPE_FLOAT);
    }
    if (binary->operator == '%') {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return type(TYPE_INTEGER);
    }
    if (is_boolean_operator(binary->operator)) {
        if (lhs == TYPE_BOOL && rhs == TYPE_BOOL)       return type(TYPE_BOOL);
    }
    if (is_comparison_operator(binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return type(TYPE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return type(TYPE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return type(TYPE_BOOL);
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return type(TYPE_BOOL);
    }

    report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(binary), "Type '%s' and '%s' is not compatible with operator %s\n", type_to_str(ti_lhs), type_to_str(ti_rhs), token_type_to_str(binary->operator));
    exit(1);
}

TypeInfo check_unary(Typer *typer, AstUnary *unary) {
    TypeInfo expr_type = check_expression(typer, unary->expr, type(TYPE_UNTYPED));

    if (unary->operator == OP_NOT) {
        if (expr_type.kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(unary->expr), "Type mismatch. Operator '!' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            exit(1);
        }
    }
    else if (unary->operator == OP_UNARY_MINUS) {
        if (expr_type.kind != TYPE_INTEGER && expr_type.kind != TYPE_FLOAT) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(unary->expr), "Type mismatch. Operator '-' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            exit(1);
        };
    } else {
        XXX;
    }

    return expr_type;
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

TypeInfo check_literal(Typer *typer, AstLiteral *literal) {
    if (literal->type == TOKEN_INTEGER) return type(TYPE_INTEGER);
    if (literal->type == TOKEN_FLOAT)   return type(TYPE_FLOAT);
    if (literal->type == TOKEN_STRING)  return type(TYPE_STRING);
    if (literal->type == TOKEN_BOOLEAN) return type(TYPE_BOOL);
    if (literal->type == TOKEN_IDENTIFIER) {
        char *ident_name     = literal->as.identifier.name;
        Symbol *ident_symbol = symbol_lookup(&typer->parser->ident_table, ident_name);
        if (ident_symbol == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undeclared variable '%s'", ident_name);
            exit(1);
        }

        // Type must have been resolved at this point
        return ident_symbol->as_value.identifier->type;
    }
    else {
        XXX;
    }
}