#include "const_expr.c"

typedef struct Typer {
    Parser *parser;
    ConstEvaluater *ce;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
    AstFor          *enclosing_for; // Used by break or continue statements to know where to branch to

    DynamicArray flattened_array; // of *DynamicArray of *TypeArray. Used to infer size of array literals
    int array_literal_depth;
} Typer;

bool  check_block(Typer *typer, AstBlock *block, bool open_lexical_scope);
bool  check_statement(Typer *typer, Ast *stmt);
Type *check_expression(Typer *typer, AstExpr *expr, Type *ctx_type);
Type *check_binary(Typer *typer, AstBinary *binary, Type *ctx_type);
Type *check_unary(Typer *typer, AstUnary *unary, Type *ctx_type);
Type *check_literal(Typer *typer, AstLiteral *literal, Type *ctx_type);
Type *check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, Type *ctx_type);
Type *check_enum_literal(Typer *typer, AstEnumLiteral *enum_literal, Type *ctx_type);
Type *check_member_access(Typer *typer, AstMemberAccess * ma);

char *type_to_str(Type *type);
bool types_are_equal(Type *lhs, Type *rhs);
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

    typer.array_literal_depth = 0;

    return typer;
}

bool check_code(Typer *typer, AstCode *code) {
    bool ok;

    for (unsigned int i = 0; i < code->statements.count; i++) {
        Ast *stmt = ((Ast **)(code->statements.items))[i];
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

Type *resolve_type(Typer *typer, Type *type, AstDeclaration *decl) {
    if (is_primitive_type(type->kind)) {
        return type;
    } 

    if (type->kind == TYPE_NAME) {
        Type *found = type_lookup(&typer->parser->type_table, type->as.name);
        if (found == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "Unknown type '%s'", type_to_str(type));
            return NULL;
        }

        return found;
    }

    if (type->kind == TYPE_POINTER) {
        TypePointer *ptr = (TypePointer *)(type);

        Type *points_to = resolve_type(typer, ptr->pointer_to, decl);
        if (!points_to) return NULL;

        ptr->head.size = 8;
        ptr->pointer_to = points_to;
        return (Type *)(ptr);
    }

    if (type->kind == TYPE_ARRAY) {
        TypeArray *array = (TypeArray *)(type);

        if (decl) {
            if (array->capacity_expr) {
                AstExpr *constexpr = simplify_expression(typer->ce, array->capacity_expr);
                if (constexpr->head.type != AST_LITERAL && ((AstLiteral *)(constexpr))->kind != LITERAL_INTEGER) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array->capacity_expr), "Size of the array must be an integer constant");
                    return NULL;
                }

                long long capacity = ((AstLiteral *)(constexpr))->as.value.integer;
                if (capacity < 0) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array->capacity_expr), "Size of the array must be non-negative. Value is %lld", capacity);
                    return NULL;
                }

                array->capicity = capacity;
            }
            if (array->flags & ARRAY_IS_STATIC_WITH_INFERRED_CAPACITY && decl->expr == NULL) { // @Note - This might also be the case for function parameters that also don't allow an expression to be present
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array), "Missing array initializer to determine size of array");
                return NULL;
            }
        }

        Type *elem_type = resolve_type(typer, array->elem_type, decl);
        if (!elem_type) return NULL;

        array->elem_type  = elem_type;
        array->head.size  = array->capicity * elem_type->size; 

        return (Type *)(array);
    }

    return NULL;
}

unsigned long long max_integer_value(TypePrimitive *type) {
    switch (type->kind) {
    case PRIMITIVE_INT: return S32_MAX;
    case PRIMITIVE_U8:  return U8_MAX;
    case PRIMITIVE_U16: return U16_MAX;
    case PRIMITIVE_U32: return U32_MAX;
    case PRIMITIVE_U64: return U64_MAX;
    case PRIMITIVE_S8:  return S8_MAX;
    case PRIMITIVE_S16: return S16_MAX;
    case PRIMITIVE_S32: return S32_MAX;
    case PRIMITIVE_S64: return S64_MAX;
    default:
        printf("Internal compiler error: Unknown type kind %d in max_integer_value()", type->kind);
        exit(1);
    }
}

bool check_for_integer_overflow(Typer *typer, Type *lhs_type, AstExpr *expr) {
    assert(lhs_type);
    if (expr == NULL) return true;
    if (lhs_type->kind != TYPE_INTEGER && lhs_type->kind != TYPE_ENUM) return true;

    if (lhs_type->kind == TYPE_ENUM) {
        // @TODO - Handle assignment to enum with integer that might be too large
        return true;
    }

    if (expr->head.type == AST_LITERAL) {
        AstLiteral *lit = (AstLiteral *)(expr);

        if (lit->kind == LITERAL_INTEGER) {
            assert(is_primitive_type(lhs_type->kind));
            assert(lit->as.value.integer >= 0);

            TypePrimitive *prim_type = (TypePrimitive *)(lhs_type);
            unsigned long long lit_value = (unsigned long long) lit->as.value.integer;
            unsigned long long max_value = max_integer_value(prim_type);

            if (lit_value > max_value) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(lit), "Assignment produces an integer overflow. Max value of type '%s' is '%llu'", type_to_str(lhs_type), max_value);
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
    if (decl->flags & DECLARATION_TYPED) {
        Type *resolved_type = resolve_type(typer, decl->declared_type, decl);
        if (!resolved_type) return false;

        decl->identifier->type = resolved_type;
        decl->declared_type    = resolved_type;
        
        Type *expr_type = check_expression(typer, decl->expr, decl->declared_type);
        if (!expr_type) return false;
        if (!types_are_equal(decl->declared_type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(decl->expr), "'%s' is said to be of type %s, but expression is of type %s", decl->identifier->name, type_to_str(decl->declared_type), type_to_str(expr_type));
            return false;
        }

        if (expr_type->kind == TYPE_ARRAY) {
            decl->identifier->type = expr_type;
            decl->declared_type    = expr_type;
        }
    }
    else if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        Type *resolved_type = resolve_type(typer, decl->declared_type, decl);
        if (!resolved_type) return false;

        decl->identifier->type = resolved_type;
        decl->declared_type    = resolved_type;
    }
    else if (decl->flags & DECLARATION_INFER) {
        Type *expr_type = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;

        decl->identifier->type = expr_type;
        decl->declared_type    = expr_type;
    }
    else if (decl->flags & DECLARATION_CONSTANT) {
        Type *expr_type  = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;

        AstExpr *const_expr = simplify_expression(typer->ce, decl->expr);
        if (const_expr->head.type == AST_LITERAL) {
            // Swap out the current expression for the simplified expression
            // @Speed? @Note - We might in the future cleanup the already allocated expression tree as its no longer needed after this swap.
            //                 We might be taking many cache misses if we leave big gaps in the allocated ast nodes, so something like packing the ast nodes could be
            //                 a thing?
            decl->expr = const_expr;
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(decl->expr), "Constant declaration with a non-constant expression");
            return false;
        }

        decl->identifier->type = expr_type;
        decl->declared_type    = expr_type;
    }

    bool ok = check_for_integer_overflow(typer, decl->declared_type, decl->expr);
    if (!ok) return false;
    
    if (decl->flags & (DECLARATION_IS_FUNCTION_PARAMETER | DECLARATION_IS_STRUCT_MEMBER)) {
        // Omit sizing the declaration as it is done at the call site
    } else {
        if (typer->enclosing_function != NULL) {
            typer->enclosing_function->bytes_allocated += decl->declared_type->size;

            if (decl->expr && decl->expr->head.type == AST_ARRAY_LITERAL) {
                // A little iffy, if we wanna keep it this way
                typer->enclosing_function->bytes_allocated += 16; // data + count
            }
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

bool types_are_equal(Type *lhs, Type *rhs) {
    assert(!(lhs->kind == TYPE_NAME || rhs->kind == TYPE_NAME)); // Type slots should have been resolved at this point

    if (is_primitive_type(lhs->kind) && is_primitive_type(rhs->kind)) {
        // Allow int to float implicit casting
        if (lhs->kind == TYPE_FLOAT && rhs->kind == TYPE_INTEGER) return true;
        else return lhs->kind == rhs->kind;
    } else if (lhs->kind == TYPE_ENUM && rhs->kind == TYPE_INTEGER) {
        return true;
    } else if ((lhs->kind == TYPE_STRUCT && rhs->kind == TYPE_STRUCT) || (lhs->kind == TYPE_ENUM   && rhs->kind == TYPE_ENUM)) {
        return strcmp(lhs->as.name, rhs->as.name) == 0;
    } else if (lhs->kind == TYPE_ARRAY && rhs->kind == TYPE_ARRAY) {
        Type *left_elem_type  = ((TypeArray *)(lhs))->elem_type;
        Type *right_elem_type = ((TypeArray *)(rhs))->elem_type;
        return types_are_equal(left_elem_type, right_elem_type);
    } else if (lhs->kind == TYPE_POINTER && rhs->kind == TYPE_POINTER) {
        Type *left_points_to  = ((TypePointer *)(lhs))->pointer_to;
        Type *right_points_to = ((TypePointer *)(rhs))->pointer_to;
        if (right_points_to->kind == TYPE_VOID) {
            return true;
        }
        return types_are_equal(left_points_to, right_points_to);
    }
    else {
        return false;
    }
}

Type *check_function_call(Typer *typer, AstFunctionCall *call) {
    Symbol *func_symbol = symbol_lookup(&typer->parser->function_table, call->identifer->name, (Ast *)call);
    if (func_symbol == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Unknown function '%s'", call->identifer->name);
        return NULL;
    }

    AstFunctionDefn *func_defn = func_symbol->as.function_defn;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Mismatch in number of arguments. Function '%s' takes %d %s, but %d were supplied", func_defn->identifier->name, func_defn->parameters.count, func_defn->parameters.count == 1 ? "parameter" : "parameters", call->arguments.count);
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        return NULL;
    }

    for (unsigned int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        AstExpr *arg       = ((AstExpr **)(call->arguments.items))[i];
        Type *arg_type = check_expression(typer, arg, NULL);
        if (!arg_type) return NULL;

        if (!types_are_equal(arg->evaluated_type, param->declared_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Expected argument to be of type '%s', but argument is of type '%s'", type_to_str(param->declared_type), type_to_str(arg_type));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
            return NULL;
        }
    }

    return func_defn->return_type;
}

bool check_assignment(Typer *typer, AstAssignment *assign) {
    Type *lhs_type  = check_expression(typer, assign->lhs, NULL);
    if (!lhs_type) return false;
    Type *expr_type = check_expression(typer, assign->expr, lhs_type);
    if (!expr_type) return false;

    bool is_member     = assign->lhs->head.type == AST_MEMBER_ACCESS;
    bool is_array_ac   = assign->lhs->head.type == AST_ARRAY_ACCESS;
    bool is_identifier = assign->lhs->head.type == AST_LITERAL && ((AstLiteral *)(assign->lhs))->kind == LITERAL_IDENTIFIER;

    AstDeclaration *member = NULL;
    AstIdentifier  *ident  = NULL;

    bool lhs_is_constant = false;
    if (is_member) {
        member = ((AstMemberAccess *)(assign->lhs))->struct_member;
        lhs_is_constant = member->flags & DECLARATION_CONSTANT;
    } 
    else if (is_array_ac) {
        lhs_is_constant = false;
    }
    else if (is_identifier) {
        ident = symbol_lookup(&typer->parser->ident_table, ((AstLiteral *)(assign->lhs))->as.value.identifier.name, (Ast *)assign->lhs)->as.identifier;
        assert(ident); // Is checked in check_expression so no need to check it here also
        lhs_is_constant = ident->flags & IDENTIFIER_IS_CONSTANT;
    } else {
        XXX;
    }

    if (lhs_is_constant) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign new value to constant '%s'", is_member ? member->identifier->name : ident->name);
        return false;
    }

    if (assign->op != '=') {
        if (expr_type->kind != TYPE_INTEGER && expr_type->kind != TYPE_FLOAT) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Compound operators are only allowed for int and float types. Given type was '%s'\n", type_to_str(expr_type));
            return false;
        }
    }

    if (lhs_type->kind == TYPE_POINTER) {
        if (expr_type->kind != TYPE_POINTER) {
            Type *points_to = ((TypePointer *)(lhs_type))->pointer_to;
            while (points_to->kind == TYPE_POINTER) {
                points_to = ((TypePointer *)(points_to))->pointer_to;
            }

            if (!types_are_equal(points_to, expr_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign->expr), "Expression of type %s cannot be assigned to pointer with innermost type %s", type_to_str(expr_type), type_to_str(points_to));
                return false;
            }

            bool ok = check_for_integer_overflow(typer, points_to, assign->expr);
            if (!ok) return false;

            return true;
        }
        /* Fallthrough */
    } 

    if (!types_are_equal(lhs_type, expr_type)) {
        if (is_member) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to member '%s' of type '%s'", type_to_str(expr_type), member->identifier->name, type_to_str(lhs_type));
        } else if (is_identifier) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to variable '%s' of type '%s'", type_to_str(expr_type), ident->name, type_to_str(lhs_type));
        } else if (is_array_ac) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to array index '%s' of type '%s'", type_to_str(expr_type), ident->name, type_to_str(lhs_type));
        } else {
            XXX;
        }

        return false;
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
    typer->enclosing_for = ast_for;

    if (ast_for->kind == FOR_INFINITY_AND_BEYOND) {
        // Nothing to check!
    }
    else if (ast_for->iterable->head.type == AST_RANGE_EXPR) {
        // @Incomplete - Need to set type of index if being used!
        AstRangeExpr *range = (AstRangeExpr *)(ast_for->iterable);
        
        Type* type_start = check_expression(typer, range->start, NULL);
        Type* type_end   = check_expression(typer, range->end, NULL);
        if (!type_start || !type_end) return NULL;

        if (type_start->kind != TYPE_INTEGER && type_start->kind != TYPE_ENUM) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(range->start), "Range expressions must have integer type as bounds. Got type %s", type_to_str(type_start));
            return NULL;
        }
        if (type_end->kind != TYPE_INTEGER && type_end->kind != TYPE_ENUM) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(range->end), "Range expressions must have integer type as bounds. Got type %s", type_to_str(type_end));
            return NULL;
        }

        ast_for->iterator->type = primitive_type(PRIMITIVE_S64);
        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_S64);
        }

        // allocate space for the iterator, start, end and optionally for the index
        assert(typer->enclosing_function != NULL);
        typer->enclosing_function->bytes_allocated += 24;
        if (ast_for->index) {
            typer->enclosing_function->bytes_allocated += 8;
        }
    } 
    else {
        Type *iterable_type = check_expression(typer, (AstExpr *)ast_for->iterable, NULL);
        if (!iterable_type) return NULL;
        if (iterable_type->kind != TYPE_ARRAY) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_for->iterable), "Cannot iterate through expression of type %s", type_to_str(iterable_type));
            return NULL;
        }

        ast_for->iterator->type = ((TypeArray *)(iterable_type))->elem_type;
        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_S64);
        }

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index
        assert(typer->enclosing_function != NULL);
        typer->enclosing_function->bytes_allocated += align_value(ast_for->iterator->type->size, 8) + 24;
    }

    bool ok = check_block(typer, ast_for->body, true);
    if (!ok) return false;

    typer->enclosing_for = NULL;
    return true;
}

AstEnumerator *enum_value_is_unique(AstEnum *ast_enum, int value) {
    for (unsigned int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        if (!etor->is_typechecked) continue;

        if (etor->value == value) return etor;
    }
    
    return NULL;
}

char *generate_c_printf_string(Typer *typer, AstPrint *print) {
    assert(print->arguments.count >= 1);

    AstExpr *arg0 = ((AstExpr **)print->arguments.items)[0];
    assert(arg0->head.type == AST_LITERAL && ((AstLiteral *)arg0)->kind == LITERAL_STRING);

    StringBuilder sb = sb_init(((AstLiteral *)arg0)->as.value.string.length);
    char *head = ((AstLiteral *)arg0)->as.value.string.data;
    char *c = head;
    int num_specifiers = 0;

    // First scan to get the number of specifiers
    while (*c != '\0') {
        if (*c == '%') {
            num_specifiers += 1;
        }
        c += 1;
    }

    if ((unsigned int )num_specifiers != print->arguments.count - 1) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg0, "Mismatch in the number of specifiers and arguments. Got %d specifiers and %d arguments", num_specifiers, print->arguments.count - 1);
        return NULL;
    }

    // Second scan to generate the string
    c = head;
    char *copied = c;
    int chars_to_copy = 0;
    int arg_index = 1; // Starts at 1 to skip the constant string
    while (*c != '\0') {
        if (*c == '%') {
            Type *arg_type = ((AstExpr **)print->arguments.items)[arg_index]->evaluated_type;
            char *format_specifier = "";
            switch (arg_type->kind) {
                case TYPE_BOOL:    format_specifier = "%s"; break;
                case TYPE_INTEGER: format_specifier = "%lld"; break;
                case TYPE_FLOAT:   format_specifier = "%lf"; break;
                case TYPE_STRING:  format_specifier = "%s"; break;
                default:
                    printf("Internal Compiler Error: Got unknown argument type %s in generate_c_printf_string()", type_to_str(arg_type));
                    return NULL;
            }

            // Copy everything upto this point in the src string
            sb_copy(&sb, copied, chars_to_copy);
            copied = c + 1;

            // Copy the format specifier
            sb_copy_string(&sb, format_specifier);

            arg_index += 1;
            chars_to_copy = 0;
            c += 1;
        }

        chars_to_copy += 1;
        c += 1;
    }

    // Copy the tail
    sb_copy(&sb, copied, chars_to_copy);

    return sb_to_string(&sb);
}

bool check_statement(Typer *typer, Ast *stmt) {
    switch (stmt->type) {
    case AST_DECLARATION: return check_declaration(typer, (AstDeclaration *)(stmt));
    case AST_ASSIGNMENT:  return check_assignment(typer, (AstAssignment *)(stmt));
    case AST_PRINT: {
        AstPrint *print = (AstPrint *)(stmt);
        if (print->arguments.count < 1) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)print, "Missing first argument to print");
            return NULL;
        }

        for (unsigned int i = 0; i < print->arguments.count; i++) {
            AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
            Type *arg_type = check_expression(typer, arg, NULL);
            if (!arg_type) return NULL;
            if (i == 0) {
                if (arg_type->kind != TYPE_STRING) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "Expected argument to have type string");
                    return NULL;
                }
                if (arg->head.type != AST_LITERAL && ((AstLiteral *)arg)->kind != LITERAL_STRING) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "First argument to printf must be a constant string");
                    return NULL;
                }

            }
        }

        char *c_string = generate_c_printf_string(typer, print);
        if (!c_string) return NULL;

        print->c_string = c_string;

        printf("Generated c string = %s\n", c_string);
        return true;
    }
    case AST_ASSERT: {
        AstAssert *assertion = (AstAssert *)(stmt);
        Type *expr_type = check_expression(typer, assertion->expr, NULL);
        if (expr_type->kind != TYPE_BOOL && expr_type->kind != TYPE_POINTER) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assertion), "Expression needs to be of type 'bool' or pointer, but expression evaluated to type '%s'", type_to_str(expr_type));
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
        Type *condition_type = check_expression(typer, ast_if->condition, NULL);
        if (!condition_type) return NULL;

        if (condition_type->kind != TYPE_BOOL && condition_type->kind != TYPE_POINTER) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_if->condition), "Expression needs to be of type 'bool' or pointer, but expression evaluated to type '%s'", type_to_str(condition_type));
            return false;
        }

        bool ok = check_block(typer, ast_if->block, true);
        if (!ok) return false;

        for (unsigned int i = 0; i < ast_if->else_ifs.count; i++) {
            AstIf *else_if = &(((AstIf *)(ast_if->else_ifs.items))[i]);
            ok = check_statement(typer, (Ast *)(else_if));
            if (!ok) return false;
        }
        if (ast_if->else_block) {
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
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_return), "Attempting to return outside of a function");
            return false;
        }

        Type *ok = check_expression(typer, ast_return->expr, ef->return_type);
        if (!ok) return false;

        if (!types_are_equal(ast_return->expr->evaluated_type, ef->return_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_return), "Type mismatch. Returning type %s from function '%s' with return type %s", type_to_str(ast_return->expr->evaluated_type), ef->identifier->name, type_to_str(ef->return_type));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(ef), "... Here is the function signature for '%s'", ef->identifier->name);
            return false;
        }

        ast_return->enclosing_function = ef;
        return true;
    }
    case AST_BREAK_OR_CONTINUE: {
        AstBreakOrContinue *boc = (AstBreakOrContinue *)(stmt);
        AstFor *enclosing_for = typer->enclosing_for;
        if (!enclosing_for) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(boc), "%s is only allowed inside for-loop's", boc->token.type ==TOKEN_BREAK ? "break" : "continue");
            return false;
        }

        boc->enclosing_for = enclosing_for;

        return true;
    }
    case AST_FUNCTION_DEFN: {
        AstFunctionDefn *func_defn = (AstFunctionDefn *)(stmt);
        typer->enclosing_function = func_defn;
        bool ok = check_block(typer, func_defn->body, true);
        if (!ok) return false;

        func_defn->return_type = resolve_type(typer, func_defn->return_type, NULL);

        typer->enclosing_function = NULL;
        return true;
    }
    case AST_STRUCT: return check_struct(typer, (AstStruct *)(stmt));
    case AST_ENUM: {
        AstEnum *ast_enum = (AstEnum *)(stmt);

        TypeEnum *enum_defn = (TypeEnum *)(type_lookup(&typer->parser->type_table, ast_enum->identifier->name));
        assert(enum_defn != NULL); // Was put in during parsing

        int auto_increment_value = 0;
        for (unsigned int i = 0; i < ast_enum->enumerators.count; i++) {
            AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
            if (etor->expr) {
                Type *ok = check_expression(typer, etor->expr, (Type *)(enum_defn)); // Type definition of the enum gets to be the ctx type so we can refer to enum members inside the enum with the shorthand syntax .ENUM_MEMBER
                if (!ok) return NULL;

                AstExpr *constexpr = simplify_expression(typer->ce, etor->expr);
                if (constexpr->head.type != AST_LITERAL) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Value must be a constant expression");
                    return false;
                }
                if (constexpr->evaluated_type->kind != TYPE_INTEGER) { // @Robustness - Check that the value of the integer does not overflow the backing type if made explicit. Maybe if the value goes beyond the default int we promote the integer type to fit the largest value???
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Enum value must have type int, but value is of type %s", type_to_str(etor->expr->evaluated_type));
                    return false;
                }

                etor->value = ((AstLiteral *)(constexpr))->as.value.integer;
                auto_increment_value = etor->value + 1;
            } else {
                etor->value = auto_increment_value;
                auto_increment_value++;
            }
            
            AstEnumerator *enumerator_with_same_value = enum_value_is_unique(ast_enum, etor->value);
            if (enumerator_with_same_value) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Enum values must be unique. Both '%s' and '%s' have value %d", etor->name, enumerator_with_same_value->name, etor->value);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enumerator_with_same_value), "Here is the enum member with the same value ...");
                return false;
            }

            etor->is_typechecked = true;
        }


        enum_defn->head.size = enum_defn->backing_type->size;
        return true;
    }
    case AST_FOR: return check_for(typer, (AstFor *)(stmt));
    default:
        XXX;
    }
}

AstEnumerator *find_enum_member(AstEnum *enum_defn, char *name) {
    for (unsigned int i = 0; i < enum_defn->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(enum_defn->enumerators.items))[i];
        if (strcmp(etor->name, name) == 0) {
            return etor;
        }
    }

    return NULL;
}

Type *check_enum_literal(Typer *typer, AstEnumLiteral *literal, Type *lhs_type) {
    if (lhs_type == NULL || lhs_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Couldn't infer type of enum value from the context");
        return NULL;
    }

    TypeEnum *enum_defn = (TypeEnum *)(lhs_type);
    char     *enum_name = literal->identifier->name;
    AstEnumerator *found = find_enum_member(enum_defn->node, enum_name);
    if (!found) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "'%s' is not a member of enum '%s'", enum_name, enum_defn->identifier->name);
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
        return NULL;
    }

    if (!found->is_typechecked) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Enum member used before being declared", enum_name, enum_defn->identifier->name);
        return NULL;
    }

    literal->enum_member = found;

    return lhs_type;
}

void queue_array_to_be_sized(DynamicArray *flattened_array, TypeArray *array, int depth) {
    if (depth >= (int)flattened_array->count) {
        for (int i = 0; i < depth + 1; i++) {
            DynamicArray elements = da_init(2, sizeof(TypeArray *));
            da_append(flattened_array, elements);
        }
    }

    DynamicArray *elements = &((DynamicArray *)(flattened_array->items))[depth];
    da_append(elements, array);
}

void free_flattened_array(DynamicArray *flattened_array) {
    for (unsigned int i = 0; i < flattened_array->count; i++) {
        DynamicArray *elements = &((DynamicArray *)(flattened_array->items))[i];
        free(elements->items);
    }
}

void do_sizing_of_entire_array(DynamicArray *flattened_array) {
    for (int d = (int)flattened_array->count - 1; d >= 0; d--) {
        DynamicArray elements = ((DynamicArray *)(flattened_array->items))[d];

        if (d == 0) assert(elements.count == 1); // Only one root array

        //
        // Static specified size
        //
        TypeArray *array = ((TypeArray **)(elements.items))[0];
        if (array->flags & ARRAY_IS_STATIC) {
            assert(array->capicity != 0 && array->capacity_expr != NULL);

            array->head.size = array->capicity * array->elem_type->size;
            continue;
        }

        //
        // Dynamic + inferred array size
        //
        int size_of_biggest_array     = 0;
        int capacity_of_biggest_array = 0;

        // First pass to find the biggest size of array
        for (unsigned j = 0; j < elements.count; j++) {
            
            TypeArray *array = ((TypeArray **)(elements.items))[j];
            AstArrayLiteral *array_lit = array->node;

            if (d == (int)(flattened_array->count - 1)) assert(array->elem_type->kind != TYPE_ARRAY);

            int capacity   = array_lit->expressions.count;
            int array_size = capacity * array->elem_type->size; 

            if (capacity   > capacity_of_biggest_array) capacity_of_biggest_array = capacity;
            if (array_size > size_of_biggest_array)     size_of_biggest_array = array_size;
        }

        // Second pass to set all the inner arrays size and capacity to be the biggest
        for (unsigned j = 0; j < elements.count; j++) {
            TypeArray *array = ((TypeArray **)(elements.items))[j];

            array->capicity  = capacity_of_biggest_array;
            array->head.size = size_of_biggest_array;
        }
    }
}

Type *check_array_literal(Typer *typer, AstArrayLiteral *array_lit, Type *ctx_type) {

    if(ctx_type && ctx_type->kind != TYPE_ARRAY) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Expected expression to have type %s", type_to_str(ctx_type));
        return NULL;
    }

    if (ctx_type && array_lit->expressions.count == 0) {
        // Don't bother type the empty array
        return ctx_type;
    }

    if (typer->array_literal_depth == 0) {
        typer->flattened_array = da_init(4, sizeof(DynamicArray));
    }
    typer->array_literal_depth += 1;

    bool infer_type_from_first_element = ctx_type == NULL;
    if (infer_type_from_first_element) {
        if (array_lit->expressions.count == 0) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Couldn't infer type of array literal from the context as it didn't contain any elements");
            return NULL;
        }

        
        Type *first_element_type = check_expression(typer, ((AstExpr **)(array_lit->expressions.items))[0], NULL);
        if (!first_element_type) return NULL;

        TypeArray *array       = type_alloc(&typer->parser->type_table, sizeof(TypeArray));
        array->head.head.type  = AST_TYPE;
        array->head.head.start = array_lit->head.head.start;
        array->head.head.end   = array_lit->head.head.end;
        array->head.kind       = TYPE_ARRAY;
        array->flags           = ARRAY_IS_STATIC_WITH_INFERRED_CAPACITY;
        array->elem_type       = first_element_type;

        ctx_type = (Type *)(array);
    }

    if(ctx_type->kind != TYPE_ARRAY) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Expected element to have type %s, but element is of type []%s", type_to_str(ctx_type), type_to_str(ctx_type));
        return NULL;
    }

    // Copying ctx_type to not alter its size and capacity
    TypeArray *array = type_alloc(&typer->parser->type_table, sizeof(TypeArray));
    memcpy(array, ctx_type, sizeof(TypeArray));
    array->node = array_lit;

    for (unsigned int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr *expr = ((AstExpr **)(array_lit->expressions.items))[i];

        bool ok = check_for_integer_overflow(typer, array->elem_type, expr);
        if (!ok) return NULL;

        if (infer_type_from_first_element && i == 0) continue;

        Type *expr_type = check_expression(typer, expr, array->elem_type);
        if (!expr_type) return NULL;
        if (!types_are_equal(array->elem_type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(expr), "Expected element to have type %s, but element is of type %s", type_to_str(array->elem_type), type_to_str(expr_type));
            return NULL;
        }
    }

    // @Hack - Reassigning the element type, as the pointer gets outdated when doing copies of the array type
    Type *first_element_type = ((AstExpr **)(array_lit->expressions.items))[0]->evaluated_type;
    array->elem_type = first_element_type;

    //
    //  Sizing of the array
    //
    if (array->capacity_expr) {
        if (array_lit->expressions.count > array->capicity) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Trying to assign %d elements to array with size %d", array_lit->expressions.count, array->capicity);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(array->capacity_expr), "Here is the specified size of that array");
            return NULL;
        }
    }

    // @Note - We have to size the array no matter what, as we allow inferred array sizes in sized arrays. F.x we could have
    // 'a : [][10][] int' where the size of the second dimension with the specified capacity is still unknown due to the last dimension being unknown
    queue_array_to_be_sized(&typer->flattened_array, array, typer->array_literal_depth - 1);

    typer->array_literal_depth -= 1;
    if (typer->array_literal_depth == 0) {
        do_sizing_of_entire_array(&typer->flattened_array);
        free_flattened_array(&typer->flattened_array);
    }

    return (Type *)(array);
}

Type *check_array_access(Typer *typer, AstArrayAccess *array_ac) {
    Type *type_being_accessed = check_expression(typer, array_ac->accessing, NULL);
    if (!type_being_accessed) return NULL;

    if (type_being_accessed->kind != TYPE_ARRAY) {
        report_error_range(typer->parser, array_ac->accessing->head.start, array_ac->open_bracket.start, "Cannot subscript into expression of type %s", type_to_str(type_being_accessed));
        return NULL;
    }

    AstExpr *subscript = array_ac->subscript;
    Type    *subscript_type = check_expression(typer, subscript, type_being_accessed); // @Note - passing down type_being_accessed to allow enum literals to be used
    if (!subscript_type) return NULL;
    if (subscript_type->kind != TYPE_INTEGER && subscript_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(subscript), "Array subscript must be an integer or enum type, got type %s", type_to_str(subscript_type));
        return NULL;
    }

    return ((TypeArray *)(type_being_accessed))->elem_type;
}

Type *check_expression(Typer *typer, AstExpr *expr, Type *ctx_type) {
    Type *result = NULL;
    if      (expr->head.type == AST_FUNCTION_CALL)  result = check_function_call(typer, (AstFunctionCall *)(expr));
    else if (expr->head.type == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr), ctx_type);
    else if (expr->head.type == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr), ctx_type);
    else if (expr->head.type == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_ARRAY_LITERAL)  result = check_array_literal(typer, (AstArrayLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_ENUM_LITERAL)   result = check_enum_literal(typer, (AstEnumLiteral *)(expr), ctx_type);
    else if (expr->head.type == AST_MEMBER_ACCESS)  result = check_member_access(typer, (AstMemberAccess *)(expr));
    else if (expr->head.type == AST_ARRAY_ACCESS)   result = check_array_access(typer, (AstArrayAccess *)(expr));
    else if (expr->head.type == AST_RANGE_EXPR)     result = primitive_type(PRIMITIVE_S64); // @Investigate - Shouldn't this be checked for both sides being integers???
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_type_name(expr->head.type));
        exit(1);
    }

    expr->evaluated_type = result;
    return result;
}

Type *check_member_access(Typer *typer, AstMemberAccess *ma) {
    Type *type_lhs = check_expression(typer, ma->left, NULL);
    if (!type_lhs) return NULL;

    //
    // Explicit enum value
    // @Cleanup - I think this bit of code can be better put elsewhere in a unary dot as that is what we are checking for here. e.g .ENUM
    //            That way, i wouldn't need to have an access kind for the member
    //
    AstExpr *lhs = ma->left;
    if (type_lhs->kind == TYPE_ENUM && lhs->head.type == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) {
        char *ident_name    = ((AstLiteral *)(lhs))->as.value.identifier.name;
        TypeEnum *enum_defn = ((TypeEnum *)(type_lhs));
        if (strcmp(ident_name, enum_defn->identifier->name) == 0) {

            AstExpr *rhs = ma->right;
            if (rhs->head.type == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
                char *name = ((AstLiteral *)(rhs))->as.value.identifier.name;
                AstEnumerator *enum_member = find_enum_member(enum_defn->node, name);
                if (!enum_member) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "%s is not a member of enum %s", name, enum_defn->identifier->name);
                    report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enum_defn), "Here is the definition of %s", enum_defn->identifier->name);
                    return NULL;
                }

                ma->access_kind = MEMBER_ACCESS_ENUM;
                ma->enum_member = enum_member;
                return (Type *)(enum_defn);
            }
        }
    }

    TypeStruct *struct_defn;
    bool valid_lhs = false;
    if (type_lhs->kind == TYPE_POINTER) {
        Type *points_to = ((TypePointer *)(type_lhs))->pointer_to;

        if (points_to->kind == TYPE_STRUCT) {
            struct_defn = (TypeStruct *)(points_to);
            valid_lhs = true;
        }
    }
    else if (type_lhs->kind == TYPE_STRUCT) {
        struct_defn = (TypeStruct *)(type_lhs);
        valid_lhs = true;
    }
    if (!valid_lhs) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ma->left), "Cannot field access into expression of type %s", type_to_str(type_lhs));
        return NULL;
    }

    AstExpr *rhs = ma->right;
    if (rhs->head.type == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
        char *member_name = ((AstLiteral *)(rhs))->as.value.identifier.name;
        
        AstDeclaration *member = get_struct_member(struct_defn->node, member_name); // @Cleanup - get_struct_member should probably take TypeStruct instead of AstStruct
        if (!member) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "'%s' is not a member of '%s'", member_name, type_to_str((Type *)struct_defn));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn), "Here is the definition of %s", type_to_str((Type *)struct_defn));
            return NULL;
        }

        ma->access_kind   = MEMBER_ACCESS_STRUCT;
        ma->struct_member = member;
        return member->declared_type;
    } else {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "Invalid member expression");
        return NULL;
    }

}

Type *check_struct_literal(Typer *typer, AstStructLiteral *literal, Type *lhs_type) {
    TypeStruct *struct_defn = NULL;
    if (literal->explicit_type) {
        // Explicit type is used
        Type *type = literal->explicit_type;
        if (type->kind == TYPE_STRUCT) {
            // Explicit type have already been resolved @Note - Currently this only happens in check_array_literal where the first expression is typechecked, and then all the expressions are again typechecked against that first expression, so we might end here.
            return literal->explicit_type;
        }

        if (type->kind == TYPE_NAME) {
            Type *found = type_lookup(&typer->parser->type_table, type->as.name);
            if (!found) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "Unknown type '%s'", type_to_str(type));
                return NULL;
            }

            if (found->kind != TYPE_STRUCT) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "'%s' is not the name of a struct. Type of '%s' is '%s'", found->as.name, found->as.name, type_to_str(type));
                return NULL;
            }
            
            literal->explicit_type = found;
            struct_defn = (TypeStruct *)(found);
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Struct literal cannot conform to type '%s'", type_to_str(lhs_type));
            return NULL;
        }
    }
    else {
        // Type is inferred from the type on the declaration
        if (lhs_type == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Type of struct literal could not be infered from the context");
            return NULL;
        }

        if (lhs_type->kind != TYPE_STRUCT) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Struct literal cannot conform to type '%s'", type_to_str(lhs_type));
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
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->designator), "'%s' is not a member of '%s'", init->designator->name, struct_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            Type *member_type = member->declared_type;
            Type *value_type  = check_expression(typer, init->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            init->member = member;
            curr_member_index = member->member_index + 1;
        } else {
            if (curr_member_index > members.count - 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init), "Initializing to unknown member of '%s'", type_to_str((Type *)(struct_defn)));
                return NULL;
            }
            
            AstDeclaration *member = ((AstDeclaration **)(members.items))[i];
            Type *member_type = member->declared_type;
            Type *value_type  = check_expression(typer, init->value, member_type);
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->identifier->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
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

Type *biggest_type(Type *lhs, Type *rhs) {
    if (lhs->size >= rhs->size) return lhs;
    else                        return rhs;
}

Type *check_binary(Typer *typer, AstBinary *binary, Type *ctx_type) {
    Type *ti_lhs = check_expression(typer, binary->left, ctx_type);
    Type *ti_rhs = check_expression(typer, binary->right, ctx_type);
    if (!ti_lhs || !ti_rhs) return NULL;

    TypeKind lhs = ti_lhs->kind;
    TypeKind rhs = ti_rhs->kind;

    if (lhs == TYPE_STRUCT && rhs == TYPE_STRUCT) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Binary operation between two structs is not yet supported!");
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
        if (lhs == TYPE_POINTER && rhs == TYPE_POINTER) return primitive_type(PRIMITIVE_BOOL);
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Type '%s' and '%s' is not compatible with operator %s\n", type_to_str(ti_lhs), type_to_str(ti_rhs), token_type_to_str(binary->operator));
    return NULL;
}

Type *check_unary(Typer *typer, AstUnary *unary, Type *ctx_type) {
    Type *expr_type = check_expression(typer, unary->expr, ctx_type);
    if (!expr_type) return NULL;

    TypeKind type_kind = expr_type->kind;

    if (unary->operator == OP_NOT) {
        if (type_kind != TYPE_BOOL && type_kind != TYPE_POINTER) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Expression needs to be of type 'bool' or pointer. Got type %s\n", type_to_str(expr_type));
            exit(1);
        }

        // @Note - If applied to a pointer, we will do a pointer to bool conversion

        return primitive_type(PRIMITIVE_BOOL);
    }
    else if (unary->operator == OP_UNARY_MINUS) {
        if (type_kind != TYPE_INTEGER && type_kind != TYPE_FLOAT && type_kind != TYPE_ENUM) { // Maybe give a warning when applying unary minus to an enum??? Seems kinda strange
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Type mismatch. Operator '-' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            return NULL;
        };

        return expr_type;
    } 
    else if (unary->operator == OP_ADDRESS_OF) {
        bool lvalue = false;
        if (unary->expr->head.type == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
            lvalue = true;
        }
        else if (unary->expr->head.type == AST_ARRAY_ACCESS) {
            lvalue = true;
        }
        else if (unary->expr->head.type == AST_MEMBER_ACCESS) {
            lvalue = true;
        }

        if (!lvalue) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Invalid lvalue");
            return NULL;
        }

        TypePointer *ptr     = type_alloc(&typer->parser->type_table, sizeof(TypePointer));
        ptr->head.head.type  = AST_TYPE;
        ptr->head.head.start = expr_type->head.start;
        ptr->head.head.end   = expr_type->head.end;
        ptr->head.kind       = TYPE_POINTER;
        ptr->head.size       = 8;
        ptr->pointer_to      = expr_type;
        return (Type *)(ptr);
    }
    else if (unary->operator == OP_POINTER_DEREFERENCE) {
        if (expr_type->kind != TYPE_POINTER) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Invalid pointer dereference. Expression has type %s", type_to_str(expr_type));
            return NULL;
        }

        Type *points_to = ((TypePointer *)(expr_type))->pointer_to;
        return points_to;
    }
    else {
        XXX;
    }
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

Type *check_literal(Typer *typer, AstLiteral *literal, Type *ctx_type) {
    switch (literal->kind) {
    case LITERAL_INTEGER:   return (ctx_type && ctx_type->kind == TYPE_INTEGER) ? ctx_type : primitive_type(PRIMITIVE_INT);
    case LITERAL_FLOAT:     return (ctx_type && ctx_type->kind == TYPE_FLOAT)   ? ctx_type : primitive_type(PRIMITIVE_FLOAT);
    case LITERAL_STRING:    return (ctx_type && ctx_type->kind == TYPE_STRING)  ? ctx_type : primitive_type(PRIMITIVE_STRING);
    case LITERAL_BOOLEAN:   return (ctx_type && ctx_type->kind == TYPE_BOOL)    ? ctx_type : primitive_type(PRIMITIVE_BOOL);
    case LITERAL_NIL:       return (Type *)t_nil_ptr;
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.value.identifier.name;
        Symbol *ident_symbol = symbol_lookup(&typer->parser->ident_table, ident_name, (Ast *)literal);
        if (ident_symbol == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Undeclared variable '%s'", ident_name);
            return NULL;
        }

        AstIdentifier *ident = ident_symbol->as.identifier;

        assert(ident->type);
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
    Symbol *struct_sym = symbol_lookup(type_table, name, NULL);
    if (struct_sym == NULL) return NULL;
    assert(struct_sym->type == AST_STRUCT);
    return struct_sym->as.struct_defn;
}

AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name) {
    Symbol *member_sym = symbol_lookup(&struct_defn->member_table, name, NULL);
    if (member_sym == NULL) return NULL;

    assert(member_sym->type == AST_DECLARATION);
    return member_sym->as.struct_member;
}