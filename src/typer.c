#include "parser.c"

typedef struct Typer {
    Parser *parser;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
} Typer;

void     check_block(Typer *typer, AstBlock *block, bool open_lexical_scope);
void     check_statement(Typer *typer, AstNode *stmt);
TypeInfo check_expression(Typer *typer, AstExpr *expr, TypeInfo lhs_type);
TypeInfo check_binary(Typer *typer, AstBinary *binary);
TypeInfo check_unary(Typer *typer, AstUnary *unary);
TypeInfo check_literal(Typer *typer, AstLiteral *literal);
TypeInfo check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, TypeInfo lhs_type);
TypeInfo check_member_access(Typer *typer, AstMemberAccess * ma);
char *type_to_str(TypeInfo type);
bool types_are_equal(Typer *typer, TypeInfo lhs, TypeInfo rhs);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);
AstStruct      *get_struct(SymbolTable *type_table, char *name);
AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name);
DynamicArray    get_struct_members(AstStruct *struct_defn);

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

void resolve_enum_or_struct_type(Typer *typer, AstDeclaration *decl) {
    if (decl->declared_type.kind == TYPE_VAR) {
        Symbol *type_sym = symbol_lookup(&typer->parser->type_table, decl->declared_type.as.identifier);
        if (type_sym == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(&decl->declared_type), "Unknown type '%s'", type_to_str(decl->declared_type));
            exit(1);
        }

        if (type_sym->type == AST_STRUCT) {
            decl->declared_type.kind    = TYPE_STRUCT;
            decl->identifier->type.kind = TYPE_STRUCT;
        } else {
            decl->declared_type.kind    = TYPE_ENUM;
            decl->identifier->type.kind = TYPE_ENUM;
        }
    }
}

void check_declaration(Typer *typer, AstDeclaration *decl) {
    switch (decl->declaration_type) {
    case DECLARATION_TYPED: {
        resolve_enum_or_struct_type(typer, decl);
        
        TypeInfo expr_type = check_expression(typer, decl->expr, decl->declared_type);
        if (!types_are_equal(typer, decl->declared_type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(decl), "Variable was said to be of type %s, but expression is of type %s", type_to_str(decl->declared_type), type_to_str(expr_type));
            exit(1);
        }

        break;
    }
    case DECLARATION_TYPED_NO_EXPR: {
        resolve_enum_or_struct_type(typer, decl);
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
    
    if (decl->flags & (DECL_IS_FUNCTION_PARAMETER | DECL_IS_STRUCT_MEMBER)) {
        // Omit sizing the declaration as it is done at the call site
    } else {
        typer->parser->ident_table.current_scope->bytes_allocated += size_of_type(typer->parser, decl->declared_type);
    }
}

bool types_are_equal(Typer *typer, TypeInfo lhs, TypeInfo rhs) {
    assert(!(lhs.kind == TYPE_VAR || rhs.kind == TYPE_VAR)); // Type slots should have been resolved at this point

    if (is_primitive_type(lhs.kind) && is_primitive_type(rhs.kind)) {
        // Allow int to float implicit casting
        if (lhs.kind == TYPE_FLOAT && rhs.kind == TYPE_INTEGER) return true;
        else return lhs.kind == rhs.kind;
    } else if (lhs.kind == TYPE_ENUM && rhs.kind == TYPE_INTEGER) {
        // report_error_ast(typer->parser, LABEL_WARNING, (AstNode *)(&lhs.head), "Assigning to enum without explicit cast");
        return true;
    } else if (
        (lhs.kind == TYPE_STRUCT && rhs.kind == TYPE_STRUCT) || 
        (lhs.kind == TYPE_ENUM   && rhs.kind == TYPE_ENUM)
    ) {
        return strcmp(lhs.as.identifier, rhs.as.identifier) == 0;
    } else {
        return false;
    }
}

TypeInfo check_function_call(Typer *typer, AstFunctionCall *call) {
    Symbol *func_symbol   = symbol_lookup(&typer->parser->function_table, call->identifer->name);
    if (func_symbol == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Unknown function '%s'", call->identifer->name);
        exit(1);
    }

    AstFunctionDefn *func_defn = func_symbol->as.function_defn;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(call), "Mismatch in number of arguments. Function '%s' takes %d %s, but %d were supplied", func_defn->identifier->name, func_defn->parameters.count, func_defn->parameters.count == 1 ? "parameter" : "parameters", call->arguments.count);
        report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        exit(1);
    }

    for (unsigned int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        AstExpr *arg = ((AstExpr **)(call->arguments.items))[i];
        check_expression(typer, arg, type(TYPE_UNTYPED));

        if (!types_are_equal(typer, arg->evaluated_type, param->declared_type)) {
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

        TypeInfo lhs_type  = check_expression(typer, assign->lhs, type(TYPE_UNTYPED));
        TypeInfo expr_type = check_expression(typer, assign->expr, lhs_type);

        if (!types_are_equal(typer, lhs_type, expr_type)) {
            if (assign->lhs->head.type == AST_MEMBER_ACCESS) {
                DynamicArray accessors = ((AstMemberAccess *)(assign->lhs))->chain;
                AstDeclaration *member = ((AstAccessor **)(accessors.items))[accessors.count - 1]->struct_member;
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Type mismatch. Trying to assign value of type '%s' to member '%s' of type '%s'", type_to_str(expr_type), member->identifier->name, type_to_str(lhs_type));
            } else if (assign->lhs->head.type == AST_LITERAL) {
                char *ident_name = ((AstLiteral *)(assign->lhs))->as.identifier.name;
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(assign), "Type mismatch. Trying to assign value of type '%s' to variable '%s' of type '%s'", type_to_str(expr_type), ident_name, type_to_str(lhs_type));
            }
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

        if (!types_are_equal(typer, ast_return->expr->evaluated_type, ef->return_type)) {
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
        AstStruct *struct_defn = (AstStruct *)(stmt);

        DynamicArray members = get_struct_members(struct_defn);
        for (unsigned int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)members.items)[i];
            check_declaration(typer, member);
        }
        
        // C-style struct alignment + padding
        int alignment = 0;
        int largest_alignment = 0;
        int offset  = 0;
        for (unsigned int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)members.items)[i];
            
            int member_size = size_of_type(typer->parser, member->declared_type);
            if (member->declared_type.kind == TYPE_STRUCT) {
                AstStruct *struct_defn = get_struct(&typer->parser->type_table, member->declared_type.as.identifier);
                assert(struct_defn != NULL);
                alignment = struct_defn->alignment;
            } else {
                alignment = member_size;
            }
            offset = align_value(offset, alignment);

            member->member_offset = offset;
            offset               += member_size;

            if (alignment > largest_alignment) largest_alignment = alignment;
        }
        struct_defn->size_bytes = align_value(offset, largest_alignment);
        struct_defn->alignment    = largest_alignment;

        free(members.items);
        return;
    }
    case AST_ENUM: {
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
            typer->parser->ident_table.current_scope->bytes_allocated += size_of_type(typer->parser, ast_for->iterator->type) * 3;
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

TypeInfo check_enum_literal(Typer *typer, AstEnumLiteral *literal, TypeInfo lhs_type) {
    if (lhs_type.kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Couldn't infer type of enum value from the context");
        exit(1);
    }

    Symbol *symbol = symbol_lookup(&typer->parser->type_table, lhs_type.as.identifier);
    assert(symbol && symbol->type == AST_ENUM);

    AstEnum *enum_defn = symbol->as.enum_defn;
    char    *enum_name = literal->identifier->name;
    AstEnumerator *exists = hash_table_get(&enum_defn->enumerators, enum_name);
    if (!exists) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "'%s' is not an enumeration of '%s'", enum_name, enum_defn->identifier->name);
        report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
        exit(1);
    }

    literal->enum_member = exists;

    return lhs_type;
}

TypeInfo check_expression(Typer *typer, AstExpr *expr, TypeInfo lhs_type) {
    TypeInfo result = {0};
    if      (expr->head.type == AST_FUNCTION_CALL)  result = check_function_call(typer,  (AstFunctionCall *)(expr));
    else if (expr->head.type == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr));
    else if (expr->head.type == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr));
    else if (expr->head.type == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr));
    else if (expr->head.type == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), lhs_type);
    else if (expr->head.type == AST_ENUM_LITERAL)   result = check_enum_literal(typer, (AstEnumLiteral *)(expr), lhs_type);
    else if (expr->head.type == AST_MEMBER_ACCESS)  result = check_member_access(typer, (AstMemberAccess *)(expr));
    else if (expr->head.type == AST_RANGE_EXPR)     result = type(TYPE_INTEGER);
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
        exit(1);
    }

    expr->evaluated_type = result;
    return result;
}

TypeInfo check_member_access(Typer *typer, AstMemberAccess *ma) {  
    Symbol *ident_sym = symbol_lookup(&typer->parser->ident_table, ma->ident->name);
    if (ident_sym == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Undeclared variable '%s'", ma->ident->name);
        exit(1);
    }

    AstIdentifier *ident = ident_sym->as.identifier;
    if (ident->type.kind != TYPE_STRUCT && ident->type.kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Trying to access variable '%s' of type '%s'", ident->name, type_to_str(ident->type));
        exit(1);
    }

    if (ident->type.kind == TYPE_ENUM) {
        if (ident->type.flags & TypeFlag_IS_NAME_OF_ENUM) {
            Symbol *sym = symbol_lookup(&typer->parser->type_table, ident->type.as.identifier);
            assert(sym && sym->type == AST_ENUM);

            assert(ma->chain.count > 0);
            AstAccessor *ac = ((AstAccessor **)(ma->chain.items))[0];
            if (ma->chain.count > 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not accessable", ac->name);
                exit(1);
            }

            // @Refactor - Find a way to refactor this. Looks similar to the way enum literals are type checked !
            AstEnum *enum_defn = sym->as.enum_defn;
            char    *enum_name = ac->name;
            AstEnumerator *enumerator = hash_table_get(&enum_defn->enumerators, enum_name);
            if (!enumerator) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not an enumeration of '%s'", enum_name, enum_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
                exit(1);
            }

            ac->kind        = Accessor_Enum;
            ac->enum_member = enumerator;

            return type_var(TYPE_ENUM, ident->type.as.identifier);
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ma->ident), "Enum values are not accessable", ident->name, type_to_str(ident->type));
            exit(1);
        }
    }

    AstStruct *curr_struct = symbol_lookup(&typer->parser->type_table, ident->type.as.identifier)->as.struct_defn;
    AstAccessor *ac        = NULL;
    for (unsigned int i = 0; i < ma->chain.count; i++) {
        ac = ((AstAccessor **)(ma->chain.items))[i];

        AstDeclaration *member = get_struct_member(curr_struct, ac->name);
        if (member == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "'%s' is not a member of '%s'", ac->name, curr_struct->identifier->name);
            report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(curr_struct), "Here is the definition of '%s'", curr_struct->identifier->name);
            exit(1);
        }

        ac->kind          = Accessor_Struct;
        ac->struct_member = member;

        if (member->declared_type.kind == TYPE_STRUCT) {
            // Change the type we are looking at to that of the member in the next iteration of the loop
            curr_struct = symbol_lookup(&typer->parser->type_table, member->declared_type.as.identifier)->as.struct_defn;
            continue;
        }


        bool has_next = (i + 1) < ma->chain.count;
        if (has_next) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(ac), "Trying to access '%s' of type '%s'", ac->name, type_to_str(member->declared_type));
            exit(1);
        }
    }

    return ac->struct_member->declared_type;
}

TypeInfo check_struct_literal(Typer *typer, AstStructLiteral *literal, TypeInfo lhs_type) {
    AstStruct *struct_defn = NULL;
    if (literal->explicit_type.kind != TYPE_UNTYPED) {
        // Explicit type is used
        if (is_primitive_type(literal->explicit_type.kind)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(literal->explicit_type));
            exit(1);
        }
        Symbol *existing = symbol_lookup(&typer->parser->type_table, literal->explicit_type.as.identifier);
        if (existing == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(&literal->explicit_type), "Unknown type '%s'", type_to_str(literal->explicit_type));
            exit(1);
        }

        struct_defn = existing->as.struct_defn;
    }
    else {
        // Type is inferred from the type on the declaration
        if (lhs_type.kind == TYPE_UNTYPED) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type of struct literal could not be infered. Put atleast a type on the declaration or make an explict type on the struct literal");
            exit(1);
        }
        if (is_primitive_type(lhs_type.kind)) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Type mistmatch. Struct literal cannot conform to type '%s'", type_to_str(lhs_type));
            exit(1);
        }

        Symbol *existing = symbol_lookup(&typer->parser->type_table, lhs_type.as.identifier);
        if (existing == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(&literal->explicit_type), "Unknown type '%s'", type_to_str(literal->explicit_type));
            exit(1);
        }

        struct_defn = existing->as.struct_defn;
    }

    DynamicArray members  = get_struct_members(struct_defn);
    unsigned int curr_member_index = 0;
    for (unsigned int i = 0; i < literal->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(literal->initializers.items))[i];

        if (init->designator != NULL) {
            AstDeclaration *member = get_struct_member(struct_defn, init->designator->name);
            if (member == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->designator), "'%s' is not a member of '%s'", init->designator->name, struct_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn), "Here is the definition of '%s'", struct_defn->identifier->name);
                exit(1);
            }

            TypeInfo member_type = member->declared_type;
            TypeInfo value_type  = check_expression(typer, init->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
            if (!types_are_equal(typer, member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn), "Here is the definition of '%s'", struct_defn->identifier->name);
                exit(1);
            }

            init->member = member;
            curr_member_index = member->member_index + 1;
        } else {
            if (curr_member_index > members.count - 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init), "Initializing to unknown member");
                exit(1);
            }
            
            AstDeclaration *member = ((AstDeclaration **)(members.items))[i];
            TypeInfo member_type   = member->declared_type;
            TypeInfo value_type    = check_expression(typer, init->value, member_type);
            if (!types_are_equal(typer, member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (AstNode *)(struct_defn), "Here is the definition of '%s'", struct_defn->identifier->name);
                exit(1);
            }

            init->member = member;
            curr_member_index += 1;
        }
    }

    if (literal->explicit_type.kind != TYPE_UNTYPED) {
        literal->explicit_type.kind = TYPE_STRUCT;
        return literal->explicit_type;
    } else {
        lhs_type.kind = TYPE_STRUCT;
        return lhs_type;
    }
}

TypeInfo check_binary(Typer *typer, AstBinary *binary) {
    TypeInfo ti_lhs = check_expression(typer, binary->left, type(TYPE_UNTYPED));
    TypeInfo ti_rhs = check_expression(typer, binary->right, type(TYPE_UNTYPED));

    TypeKind lhs = ti_lhs.kind;
    TypeKind rhs = ti_rhs.kind;
    if (!is_primitive_type(lhs) && !is_primitive_type(rhs)) {
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
    switch (literal->kind) {
    case LITERAL_INTEGER:   return type(TYPE_INTEGER);
    case LITERAL_FLOAT:     return type(TYPE_FLOAT);
    case LITERAL_STRING:    return type(TYPE_STRING);
    case LITERAL_BOOLEAN:   return type(TYPE_BOOL);
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.identifier.name;
        Symbol *ident_symbol = symbol_lookup(&typer->parser->ident_table, ident_name);
        if (ident_symbol == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (AstNode *)(literal), "Undeclared variable '%s'", ident_name);
            exit(1);
        }

        // Type must have been resolved at this point
        return ident_symbol->as.identifier->type;
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