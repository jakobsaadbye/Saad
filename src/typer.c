#include "const_expr.c"

typedef struct Typer {
    Parser *parser;
    ConstEvaluater *ce;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
} Typer;

bool     check_block(Typer *typer, AstBlock *block, bool open_lexical_scope);
bool     check_statement(Typer *typer, AstNode *stmt);
TypeInfo *check_expression(Typer *typer, AstExpr *expr, TypeInfo *ctx_type);
TypeInfo *check_binary(Typer *typer, AstBinary *binary, TypeInfo *ctx_type);
TypeInfo *check_unary(Typer *typer, AstUnary *unary, TypeInfo *ctx_type);
TypeInfo *check_literal(Typer *typer, AstLiteral *literal);
TypeInfo *check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, TypeInfo *ctx_type);
TypeInfo *check_enum_literal(Typer *typer, AstEnumLiteral *enum_literal, TypeInfo *ctx_type);
TypeInfo *check_member_access(Typer *typer, AstMemberAccess * ma);
char *type_to_str(TypeInfo *type);
bool types_are_equal(TypeInfo *lhs, TypeInfo *rhs);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);
AstStruct      *get_struct(SymbolTable *type_table, char *name);
AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name);
DynamicArray    get_struct_members(AstStruct *struct_defn);

Typer typer_init(Parser *parser, ConstEvaluater *ce) {
    Typer typer = {0};
    typer.parser = parser;
    typer.ce = ce;
    typer.enclosing_function = NULL;

    return typer;
}

bool check_code(Typer *typer, AstCode *code) {
    bool ok;

    for (unsigned int i = 0; i < code->statements.count; i++) {
        AstNode *stmt = ((AstNode **)(code->statements.items))[i];
        ok = check_statement(typer, stmt);
        if (!ok) return false; // @Improvement - Maybe proceed with typechecking if we can still continue after seing the error
    }

    return true;
}

bool check_block(Typer *typer, AstBlock *block, bool open_lexical_scope) {
    bool ok;

    if (open_lexical_scope) enter_scope(&typer->parser->ident_table);
    for (int i = 0; i < block->num_of_statements; i++) {
        ok = check_statement(typer, block->statements[i]);
        if (!ok) return false;
    }
    if (open_lexical_scope) exit_scope(&typer->parser->ident_table);

    return true;
}

bool resolve_enum_or_struct_type(Typer *typer, AstDeclaration *decl) {
    if (decl->declared_type->kind == TYPE_NAME) {
        TypeInfo *found = type_lookup(&typer->parser->type_table, decl->declared_type->as.name);
        if (found == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl->declared_type), "Unknown type '%s'", type_to_str(decl->declared_type));
            return false;
        }

        decl->declared_type    = found;
        decl->identifier->type = found;
    }

    return true;
}

unsigned long long max_value_of_type(TypePrimitive *type) {
    switch (type->kind) {
        case PRIMITIVE_INVALID: XXX;
        case PRIMITIVE_INT:     return S32_MAX;
        case PRIMITIVE_U8:      return U8_MAX;
        case PRIMITIVE_U16:     return U16_MAX;
        case PRIMITIVE_U32:     return U32_MAX;
        case PRIMITIVE_U64:     return U64_MAX;
        case PRIMITIVE_S8:      return S8_MAX;
        case PRIMITIVE_S16:     return S16_MAX;
        case PRIMITIVE_S32:     return S32_MAX;
        case PRIMITIVE_S64:     return S64_MAX;
        case PRIMITIVE_FLOAT:   XXX;
        case PRIMITIVE_F32:     XXX;
        case PRIMITIVE_F64:     XXX;
        case PRIMITIVE_STRING:  XXX;
        case PRIMITIVE_BOOL:    XXX;
        case PRIMITIVE_VOID:    XXX;
        case PRIMITIVE_COUNT:   XXX;
    }

    printf("internal compiler error: Unknown type kind %d in max_value_of_type()", type->kind);
    exit(1);
}

bool check_for_integer_overflow(Typer *typer, TypeInfo *lhs_type, AstExpr *expr) {
    assert(lhs_type);
    if (expr == NULL) return true;

    if (lhs_type->kind == TYPE_ENUM) {
        // @Todo - Handle assignment to enum with integer that might be too large
        return true;
    }

    if (expr->head.type == AST_LITERAL) {
        AstLiteral *lit = (AstLiteral *)(expr);

        if (lit->kind == LITERAL_INTEGER) {
            assert(is_primitive_type(lhs_type->kind));
            assert(lit->as.value.integer >= 0);

            TypePrimitive *prim_type = (TypePrimitive *)(lhs_type);
            unsigned long long lit_value = (unsigned long long) lit->as.value.integer;
            unsigned long long max_value = max_value_of_type(prim_type);

            if (lit_value > max_value) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(lit), "Assignment produces an integer overflow. Max value of type '%s' is '%llu'", type_to_str(lhs_type), max_value);
                return false;
            }
        } 
    }
    // Expr is complex so we just let it slide. 
    // @Note - When we can evaluate expressions, we should
    // be able to handle checking overflow situations.
    return true;
}

bool check_declaration(Typer *typer, AstDeclaration *decl) {
    bool ok;

    if (decl->flags & DECLARATION_TYPED) {
        ok = resolve_enum_or_struct_type(typer, decl);
        if (!ok) return false;
        
        TypeInfo *expr_type = check_expression(typer, decl->expr, decl->declared_type);
        if (!expr_type) return false;
        if (!types_are_equal(decl->declared_type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl), "Variable was said to be of type %s, but expression is of type %s", type_to_str(decl->declared_type), type_to_str(expr_type));
            return false;
        }
    }
    else if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        ok = resolve_enum_or_struct_type(typer, decl);
        if (!ok) return false;
    }
    else if (decl->flags & DECLARATION_INFER) {
        TypeInfo *expr_type = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;
        
        decl->identifier->type = expr_type;
        decl->declared_type    = expr_type;
    }
    else if (decl->flags & DECLARATION_CONSTANT) {
        TypeInfo *expr_type  = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;

        AstExpr *const_expr = simplify_expression(typer->ce, decl->expr);
        if (const_expr->head.type == AST_LITERAL) {
            // Swap out the current expression for the simplified expression
            // @Speed? @Note - We might in the future cleanup the already allocated expression tree as its no longer needed after this swap.
            //                 We might be taking many cache misses if we leave big gaps in the allocated ast nodes, so something like packing the ast nodes could be
            //                 a thing?
            decl->expr = const_expr;
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl->expr), "Constant declaration with a non-constant expression");
            return false;
        }

        decl->identifier->type = expr_type;
        decl->declared_type    = expr_type;
    }

    ok = check_for_integer_overflow(typer, decl->declared_type, decl->expr);
    if (!ok) return false;
    
    if (decl->flags & (DECLARATION_IS_FUNCTION_PARAMETER | DECLARATION_IS_STRUCT_MEMBER)) {
        // Omit sizing the declaration as it is done at the call site
    } else {
        if (typer->enclosing_function != NULL) {
            typer->enclosing_function->bytes_allocated += decl->declared_type->size;
        } else {
            if (decl->flags & DECLARATION_CONSTANT) {
                // Constants don't need to be sized
            } else {
                // @TODO: Non-constant declarations in global scope should also be sized. Variables that live in global scope, should probably have some defined stack address of where they live, so they can be referenced locally. Still an open question on how i would do this.
                XXX;
            }
        }
    }

    return true;
}

bool types_are_equal(TypeInfo *lhs, TypeInfo *rhs) {
    assert(!(lhs->kind == TYPE_NAME || rhs->kind == TYPE_NAME)); // Type slots should have been resolved at this point

    if (is_primitive_type(lhs->kind) && is_primitive_type(rhs->kind)) {
        // Allow int to float implicit casting
        if (lhs->kind == TYPE_FLOAT && rhs->kind == TYPE_INTEGER) return true;
        else return lhs->kind == rhs->kind;
    } else if (lhs->kind == TYPE_ENUM && rhs->kind == TYPE_INTEGER) {
        return true;
    } else if (
        (lhs->kind == TYPE_STRUCT && rhs->kind == TYPE_STRUCT) || 
        (lhs->kind == TYPE_ENUM   && rhs->kind == TYPE_ENUM)
    ) {
        return strcmp(lhs->as.name, rhs->as.name) == 0;
    } else {
        return false;
    }
}

TypeInfo *check_function_call(Typer *typer, AstFunctionCall *call) {
    Symbol *func_symbol = symbol_lookup(&typer->parser->function_table, call->identifer->name);
    if (func_symbol == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Unknown function '%s'", call->identifer->name);
        return NULL;
    }

    AstFunctionDefn *func_defn = func_symbol->as.function_defn;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Mismatch in number of arguments. Function '%s' takes %d %s, but %d were supplied", func_defn->identifier->name, func_defn->parameters.count, func_defn->parameters.count == 1 ? "parameter" : "parameters", call->arguments.count);
        report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        return NULL;
    }

    for (unsigned int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        AstExpr *arg       = ((AstExpr **)(call->arguments.items))[i];
        TypeInfo *arg_type = check_expression(typer, arg, NULL);
        if (!arg_type) return NULL;

        if (!types_are_equal(arg->evaluated_type, param->declared_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(arg), "Type mismatch. Expected argument to be of type '%s', but argument is of type '%s'", type_to_str(param->declared_type), type_to_str(arg_type));
            report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
            return NULL;
        }
    }

    return func_defn->return_type;
}

bool check_assignment(Typer *typer, AstAssignment *assign) {
    TypeInfo *lhs_type  = check_expression(typer, assign->lhs, NULL);
    if (!lhs_type) return false;
    TypeInfo *expr_type = check_expression(typer, assign->expr, lhs_type);
    if (!expr_type) return false;

    bool is_member     = assign->lhs->head.type == AST_MEMBER_ACCESS;
    bool is_identifier = assign->lhs->head.type == AST_LITERAL && ((AstLiteral *)(assign->lhs))->kind == LITERAL_IDENTIFIER;

    AstDeclaration *member = NULL;
    AstIdentifier  *ident  = NULL;

    bool lhs_is_constant = false;
    if (is_member) {
        DynamicArray accessors = ((AstMemberAccess *)(assign->lhs))->chain;
        member = ((AstAccessor **)(accessors.items))[accessors.count - 1]->struct_member;
        lhs_is_constant = member->flags & DECLARATION_CONSTANT;
    } else if (is_identifier) {
        ident = symbol_lookup(&typer->parser->ident_table, ((AstLiteral *)(assign->lhs))->as.value.identifier.name)->as.identifier;
        assert(ident); // Is checked in check_expression so no need to check it here also
        lhs_is_constant = ident->flags & IDENTIFIER_IS_CONSTANT;
    } else {
        XXX;
    }

    if (lhs_is_constant) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Cannot assign new value to constant '%s'", is_member ? member->identifier->name : ident->name);
        return false;
    }

    if (!types_are_equal(lhs_type, expr_type)) {
        if (is_member) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Cannot assign value of '%s' to member '%s' of type '%s'", type_to_str(expr_type), member->identifier->name, type_to_str(lhs_type));
        } else if (is_identifier) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Cannot assign value of '%s' to variable '%s' of type '%s'", type_to_str(expr_type), ident->name, type_to_str(lhs_type));
        } else {
            XXX;
        }

        return false;
    }

    if (assign->op != '=') {
        if (expr_type->kind != TYPE_INTEGER && expr_type->kind != TYPE_FLOAT) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Compound operators are only allowed for int and float types. Given type was '%s'\n", type_to_str(expr_type));
            return false;
        }
    }

    bool ok = check_for_integer_overflow(typer, lhs_type, assign->expr);
    if (!ok) return false;

    return true;
}

bool check_struct(Typer *typer, AstStruct *ast_struct) {
    TypeStruct *type_struct = (TypeStruct *)(type_lookup(&typer->parser->type_table, ast_struct->identifier->name));
    assert(type_struct != NULL && type_struct->head.kind == TYPE_STRUCT);

    DynamicArray members = get_struct_members(ast_struct);
    bool ok;
    for (unsigned int i = 0; i < members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];
        ok = check_declaration(typer, member);
        if (!ok) return false;
    }
    
    // C-style struct alignment + padding
    int alignment = 0;
    int largest_alignment = 0;
    int offset  = 0;
    for (unsigned int i = 0; i < members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];
        
        int member_size = member->declared_type->size;
        if (member->declared_type->kind == TYPE_STRUCT) {
            alignment = ((TypeStruct *)(member->declared_type))->alignment;
        } else {
            alignment = member_size;
        }
        offset = align_value(offset, alignment);

        member->member_offset = offset;
        offset               += member_size;

        if (alignment > largest_alignment) largest_alignment = alignment;
    }

    type_struct->head.size  = align_value(offset, largest_alignment);
    type_struct->alignment  = largest_alignment;

    free(members.items);
    return true;
}

bool check_for(Typer *typer, AstFor *ast_for) {
    if (ast_for->iterable->head.type == AST_RANGE_EXPR) {
        AstRangeExpr *range = (AstRangeExpr *)(ast_for->iterable);
        
        TypeInfo* type_start = check_expression(typer, range->start, NULL);
        TypeInfo* type_end   = check_expression(typer, range->end, NULL);
        if (!type_start || !type_end) return NULL;

        if (type_start->kind != TYPE_INTEGER && type_start->kind != TYPE_ENUM) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(range->start), "Range expressions must have integer type as bounds. Got type %s", type_to_str(type_start));
            return NULL;
        }
        if (type_end->kind != TYPE_INTEGER && type_end->kind != TYPE_ENUM) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(range->end), "Range expressions must have integer type as bounds. Got type %s", type_to_str(type_end));
            return NULL;
        }

        ast_for->iterator->type = primitive_type(PRIMITIVE_INT);

        // allocate space for the iterator, start and end
        assert(typer->enclosing_function != NULL);
        typer->enclosing_function->bytes_allocated += ast_for->iterator->type->size;
        typer->enclosing_function->bytes_allocated += type_start->size;
        typer->enclosing_function->bytes_allocated += type_end->size;
    } else {
        // ToDo: Implement for unnamed for-statements
        XXX;
    }

    bool ok = check_block(typer, ast_for->body, true);
    if (!ok) return false;

    return true;
}

bool check_statement(Typer *typer, AstNode *stmt) {
    switch (stmt->type) {
    case AST_DECLARATION: return check_declaration(typer, (AstDeclaration *)(stmt));
    case AST_ASSIGNMENT:  return check_assignment(typer, (AstAssignment *)(stmt));
    case AST_PRINT: {
        AstPrint *print = (AstPrint *)(stmt);
        return check_expression(typer, print->expr, NULL);
    }
    case AST_ASSERT: {
        AstAssert *assertion = (AstAssert *)(stmt);
        TypeInfo *expr_type = check_expression(typer, assertion->expr, NULL);
        if (expr_type->kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assertion), "Type mismatch. Expected expression to be of type bool, but expression is of type '%s'", type_to_str(expr_type));
            return false;
        }

        return true;
    }
    case AST_TYPEOF: {
        AstTypeof *ast_typeof = (AstTypeof *)(stmt);
        return check_expression(typer, ast_typeof->expr, NULL);
    }
    case AST_IF: {
        AstIf *ast_if = (AstIf *)(stmt);
        TypeInfo *condition_type = check_expression(typer, ast_if->condition, NULL);
        if (condition_type->kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_if->condition), "Expression needs to be of type 'bool', but expression evaluated to type '%s'", type_to_str(condition_type));
            return false;
        }

        bool ok = check_block(typer, ast_if->block, true);
        if (!ok) return false;

        for (unsigned int i = 0; i < ast_if->else_ifs.count; i++) {
            AstIf *else_if = &(((AstIf *)(ast_if->else_ifs.items))[i]);
            ok = check_statement(typer, (AstNode *)(else_if));
            if (!ok) return false;
        }
        if (ast_if->has_else_block) {
            ok = check_block(typer, ast_if->else_block, true);
            if (!ok) return false;
        }

        return true;
    }
    case AST_BLOCK: {
        AstBlock *block = (AstBlock *)(stmt);
        return check_block(typer, block, true);
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(stmt);
        return check_function_call(typer, call);
    }
    case AST_RETURN: {
        AstReturn *ast_return = (AstReturn *)(stmt);
        AstFunctionDefn *ef = typer->enclosing_function;
        if (ef == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_return), "Attempting to return outside of a function");
            return false;
        }

        TypeInfo *ok = check_expression(typer, ast_return->expr, ef->return_type);
        if (!ok) return false;

        if (!types_are_equal(ast_return->expr->evaluated_type, ef->return_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ast_return), "Type mismatch. Type of expression in return is %s, but function '%s' has return type %s", type_to_str(ast_return->expr->evaluated_type), ef->identifier->name, type_to_str(ef->return_type));
            return false;
        }

        ast_return->enclosing_function = ef;
        return true;
    }
    case AST_FUNCTION_DEFN: {
        AstFunctionDefn *func_defn = (AstFunctionDefn *)(stmt);
        typer->enclosing_function = func_defn;
        bool ok = check_block(typer, func_defn->body, true);
        if (!ok) return false;

        typer->enclosing_function = NULL;
        return true;
    }
    case AST_STRUCT: return check_struct(typer, (AstStruct *)(stmt));
    case AST_ENUM: {
        AstEnum *ast_enum = (AstEnum *)(stmt);

        TypeEnum *enum_defn = (TypeEnum *)(type_lookup(&typer->parser->type_table, ast_enum->identifier->name));
        assert(enum_defn != NULL); // Was put in during parsing

        enum_defn->head.size = enum_defn->backing_type->size;
        return true;
    }
    case AST_FOR: return check_for(typer, (AstFor *)(stmt));
    default:
        XXX;
    }
}

TypeInfo *check_enum_literal(Typer *typer, AstEnumLiteral *literal, TypeInfo *lhs_type) {
    if (lhs_type == NULL || lhs_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Couldn't infer type of enum value from the context");
        return NULL;
    }

    TypeEnum *enum_defn = (TypeEnum *)(lhs_type);
    char     *enum_name = literal->identifier->name;
    AstEnumerator *found = hash_table_get(&enum_defn->node->enumerators, enum_name);
    if (!found) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "'%s' is not a member of enum '%s'", enum_name, enum_defn->identifier->name);
        report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
        return NULL;
    }

    literal->enum_member = found;

    return lhs_type;
}

TypeInfo *check_expression(Typer *typer, AstExpr *expr, TypeInfo *ctx_type) {
    TypeInfo *result = NULL;
    if      (expr->head.type == AST_FUNCTION_CALL)  result = check_function_call(typer, (AstFunctionCall *)(expr));
    else if (expr->head.type == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr), ctx_type);
    else if (expr->head.type == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr), ctx_type);
    else if (expr->head.type == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr));
    else if (expr->head.type == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_ENUM_LITERAL)   result = check_enum_literal(typer, (AstEnumLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_MEMBER_ACCESS)  result = check_member_access(typer, (AstMemberAccess *)(expr));
    else if (expr->head.type == AST_RANGE_EXPR)     result = primitive_type(PRIMITIVE_INT); // @Investigate - Shouldn't this be checked for both sides being integers???
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
        exit(1);
    }

    expr->evaluated_type = result;
    return result;
}

TypeInfo *check_member_access(Typer *typer, AstMemberAccess *ma) {  
    Symbol *ident_sym = symbol_lookup(&typer->parser->ident_table, ma->ident->name);
    if (ident_sym == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Undeclared variable '%s'", ma->ident->name);
        return NULL;
    }

    AstIdentifier *ident = ident_sym->as.identifier;
    if (ident->type->kind != TYPE_STRUCT && ident->type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Trying to access variable '%s' of type '%s'", ident->name, type_to_str(ident->type));
        return NULL;
    }

    if (ident->type->kind == TYPE_ENUM) {
        if (ident->flags & IDENTIFIER_IS_NAME_OF_ENUM) {
            assert(ma->chain.count > 0);
            AstAccessor *ac = ((AstAccessor **)(ma->chain.items))[0];
            if (ma->chain.count > 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not accessable", ac->name);
                return NULL;
            }

            // @Refactor - Find a way to refactor this. Looks similar to the way enum literals are type checked !
            TypeEnum *enum_defn = (TypeEnum *)(ident->type);
            char     *enum_name = ac->name;
            AstEnumerator *found = hash_table_get(&enum_defn->node->enumerators, enum_name);
            if (!found) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not a member of enum '%s'", enum_name, enum_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
                return NULL;
            }

            ac->kind        = ACCESSOR_ENUM;
            ac->enum_member = found;

            return (TypeInfo *)(enum_defn);
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Enum values are not accessable", ident->name, type_to_str(ident->type));
            return NULL;
        }
    }


    if (ident->type->kind == TYPE_STRUCT) {
        TypeStruct  *curr_struct = (TypeStruct *)(ident->type);
        AstAccessor *ac          = NULL;
        for (unsigned int i = 0; i < ma->chain.count; i++) {
            ac = ((AstAccessor **)(ma->chain.items))[i];

            AstDeclaration *member = get_struct_member(curr_struct->node, ac->name);
            if (member == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not a member of '%s'", ac->name, curr_struct->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(curr_struct), "Here is the definition of '%s'", curr_struct->identifier->name);
                return NULL;
            }

            ac->kind          = ACCESSOR_STRUCT;
            ac->struct_member = member;

            if (member->declared_type->kind == TYPE_STRUCT) {
                curr_struct = (TypeStruct *)(member->declared_type);
                continue;
            } else {
                bool has_next = (i + 1) < ma->chain.count;
                if (has_next) {
                    report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "Trying to access '%s' of type '%s'", ac->name, type_to_str(member->declared_type));
                    return NULL;
                } else {
                    // Everything is fine, we end on a member not a struct
                }
            }
        }

        return ac->struct_member->declared_type;
    }

    XXX;
}

TypeInfo *check_struct_literal(Typer *typer, AstStructLiteral *literal, TypeInfo *lhs_type) {
    TypeStruct *struct_defn = NULL;
    if (literal->explicit_type != NULL) {
        // Explicit type is used
        TypeInfo *type = literal->explicit_type;
        if (type->kind != TYPE_NAME) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(type));
            return NULL;
        }

        TypeInfo *found = type_lookup(&typer->parser->type_table, type->as.name);
        if (!found) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(&type), "Unknown type '%s'", type_to_str(type));
            return NULL;
        }

        if (found->kind != TYPE_STRUCT) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(&type), "'%s' is not the name of a struct. Type of '%s' is '%s'", found->as.name, found->as.name, type_to_str(type));
            return NULL;
        }
        
        literal->explicit_type = found;
        struct_defn = (TypeStruct *)(found);
    }
    else {
        // Type is inferred from the type on the declaration
        if (lhs_type == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type of struct literal could not be infered. Put atleast a type on the declaration or make an explict type on the struct literal");
            return NULL;
        }

        if (lhs_type->kind != TYPE_STRUCT) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(lhs_type));
            return NULL;
        }

        struct_defn = (TypeStruct *)(lhs_type);
    }

    DynamicArray members = get_struct_members(struct_defn->node);
    unsigned int curr_member_index = 0;
    for (unsigned int i = 0; i < literal->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(literal->initializers.items))[i];

        if (init->designator != NULL) {
            AstDeclaration *member = get_struct_member(struct_defn->node, init->designator->name);
            if (member == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->designator), "'%s' is not a member of '%s'", init->designator->name, struct_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            TypeInfo *member_type = member->declared_type;
            TypeInfo *value_type  = check_expression(typer, init->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            init->member = member;
            curr_member_index = member->member_index + 1;
        } else {
            if (curr_member_index > members.count - 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init), "Initializing to unknown member");
                return NULL;
            }
            
            AstDeclaration *member  = ((AstDeclaration **)(members.items))[i];
            TypeInfo *member_type   = member->declared_type;
            TypeInfo *value_type    = check_expression(typer, init->value, member_type);
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            init->member = member;
            curr_member_index += 1;
        }
    }

    if (literal->explicit_type != NULL) {
        return literal->explicit_type;
    } else {
        return lhs_type;
    }
}

TypeInfo *biggest_type(TypeInfo *lhs, TypeInfo *rhs) {
    if (lhs->size >= rhs->size) return lhs;
    else                        return rhs;
}

TypeInfo *check_binary(Typer *typer, AstBinary *binary, TypeInfo *ctx_type) {
    TypeInfo *ti_lhs = check_expression(typer, binary->left, ctx_type);
    TypeInfo *ti_rhs = check_expression(typer, binary->right, ctx_type);
    if (!ti_lhs || !ti_rhs) return NULL;

    TypeKind lhs = ti_lhs->kind;
    TypeKind rhs = ti_rhs->kind;

    if (lhs == TYPE_STRUCT && rhs == TYPE_STRUCT) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(binary), "Binary operation between two structs is not yet supported!");
        return NULL;
    }

    if (lhs == TYPE_ENUM) lhs = TYPE_INTEGER;
    if (rhs == TYPE_ENUM) rhs = TYPE_INTEGER;

    if (strchr("+-*/^", binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return ti_lhs;
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return ti_rhs;
    }
    if (binary->operator == '%') {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
    }
    if (is_boolean_operator(binary->operator)) {
        if (lhs == TYPE_BOOL && rhs == TYPE_BOOL)       return primitive_type(PRIMITIVE_BOOL);
    }
    if (is_comparison_operator(binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return primitive_type(PRIMITIVE_BOOL);
    }

    report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(binary), "Type '%s' and '%s' is not compatible with operator %s\n", type_to_str(ti_lhs), type_to_str(ti_rhs), token_type_to_str(binary->operator));
    return NULL;
}

TypeInfo *check_unary(Typer *typer, AstUnary *unary, TypeInfo *ctx_type) {
    TypeInfo *expr_type = check_expression(typer, unary->expr, ctx_type);
    if (!expr_type) return NULL;

    TypeKind type_kind = expr_type->kind;

    if (unary->operator == OP_NOT) {
        if (type_kind != TYPE_BOOL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(unary->expr), "Type mismatch. Operator '!' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            exit(1);
        }
    }
    else if (unary->operator == OP_UNARY_MINUS) {
        if (type_kind != TYPE_INTEGER && type_kind != TYPE_FLOAT && type_kind != TYPE_ENUM) { // Maybe give a warning when applying unary minus to an enum??? Seems kinda strange
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(unary->expr), "Type mismatch. Operator '-' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            return NULL;
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

bool is_a_before_b (AstNode *a, AstNode *b) {
    if ((b->start.line - a->start.line) > 0) return true;
    if ((b->start.line - a->start.line) < 0) return false;
    else {
        return (b->start.col - a->start.col) < 0;
    }
}

TypeInfo *check_literal(Typer *typer, AstLiteral *literal) {
    switch (literal->kind) {
    case LITERAL_INTEGER:   return primitive_type(PRIMITIVE_INT);
    case LITERAL_FLOAT:     return primitive_type(PRIMITIVE_FLOAT);
    case LITERAL_STRING:    return primitive_type(PRIMITIVE_STRING);
    case LITERAL_BOOLEAN:   return primitive_type(PRIMITIVE_BOOL);
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.value.identifier.name;
        Symbol *ident_symbol = symbol_lookup(&typer->parser->ident_table, ident_name);
        if (ident_symbol == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undeclared variable '%s'", ident_name);
            return NULL;
        }

        AstIdentifier *ident = ident_symbol->as.identifier;

        if (is_a_before_b((AstNode *)(literal), (AstNode *)(ident))) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undeclared variable '%s'", ident->name);
            return NULL;
        }

        // Type must have been resolved at this point
        return ident->type;
    }}

    XXX;
}

int cmp_members(const void *a, const void *b) {
    return (*(AstDeclaration **)(a))->member_index - (*(AstDeclaration **)(b))->member_index;
}

DynamicArray get_struct_members(AstStruct *struct_defn) {
    DynamicArray member_entries = symbol_get_symbols(&struct_defn->member_table);
    DynamicArray members        = da_init(member_entries.count, sizeof(AstDeclaration *));
    for (unsigned int i = 0; i < member_entries.count; i++) {
        da_append(&members, ((Symbol **)(member_entries.items))[i]->as.struct_member);
    }
    free(member_entries.items);

    qsort(members.items, members.count, members.item_size, cmp_members);
    return members;
}

AstStruct *get_struct(SymbolTable *type_table, char *name) {
    Symbol *struct_sym = symbol_lookup(type_table, name);
    if (struct_sym == NULL) return NULL;
    assert(struct_sym->type == AST_STRUCT);
    return struct_sym->as.struct_defn;
}

AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name) {
    Symbol *member_sym = symbol_lookup(&struct_defn->member_table, name);
    if (member_sym == NULL) return NULL;

    assert(member_sym->type == AST_DECLARATION);
    return member_sym->as.struct_member;
}