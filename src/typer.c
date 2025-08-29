#include "const_expr.c"
#include "lib/string_builder.h"

typedef struct Typer {
    Parser *parser;
    ConstEvaluater *ce;

    AstBlock *current_scope;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
    AstFor          *enclosing_for;      // Used by break or continue statements to know where to branch to
    AstWhile        *enclosing_while;    // Same as above

    DynamicArray flattened_array; // of *DynamicArray of *TypeArray. Used to infer size of array literals
    int array_literal_depth;
} Typer;

bool  check_block(Typer *typer, AstBlock *block);
bool  check_statement(Typer *typer, Ast *stmt);
Type *check_expression(Typer *typer, AstExpr *expr, Type *ctx_type);
Type *check_binary(Typer *typer, AstBinary *binary, Type *ctx_type);
Type *check_unary(Typer *typer, AstUnary *unary, Type *ctx_type);
Type *check_cast(Typer *typer, AstCast *cast, Type *ctx_type);
Type *check_literal(Typer *typer, AstLiteral *literal, Type *ctx_type);
Type *check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, Type *ctx_type);
Type *check_enum_literal(Typer *typer, AstEnumLiteral *enum_literal, Type *ctx_type);
Type *check_member_access(Typer *typer, AstMemberAccess * ma);
bool  check_break_or_continue(Typer *typer, AstBreakOrContinue *boc);
Type *check_enum_defn(Typer *typer, AstEnum *ast_enum);
Type *check_function_defn(Typer *typer, AstFunctionDefn *func_defn);

char *type_to_str(Type *type);
bool types_are_equal(Type *lhs, Type *rhs);
bool can_cast_implicitly(Type *from, Type *to);
bool can_cast_explicitly(Type *from, Type *to);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);
bool is_untyped_literal(Type *type);
void reserve_temporary_storage(AstFunctionDefn *func_defn, int size);
int push_temporary_value(AstFunctionDefn *func_defn, int size);
int pop_temporary_value(AstFunctionDefn *func_defn);
void reset_temporary_storage();
AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name);
char *generate_c_format_specifier_for_type(Type *type);


Typer typer_init(Parser *parser, ConstEvaluater *ce) {
    Typer typer = {0};
    typer.parser = parser;
    typer.ce = ce;
    typer.current_scope = parser->current_scope;
    typer.enclosing_function = NULL;
    typer.enclosing_for = NULL;
    typer.enclosing_while = NULL;

    typer.array_literal_depth = 0;

    return typer;
}

bool check_code(Typer *typer, AstCode *code) {
    bool ok;

    for (int i = 0; i < code->statements.count; i++) {
        Ast *stmt = ((Ast **)(code->statements.items))[i];
        ok = check_statement(typer, stmt);
        if (!ok) return false; // @Improvement - Maybe proceed with typechecking if we can still continue after seing the error
    }

    return true;
}

void dump_scope(AstBlock *scope) {
    AstBlock *s = scope;
    while (s) {
        for (int i = 0; i < s->identifiers.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)s->identifiers.items)[i];
            printf("(#%d, %s, Ln %d, Col %d)\n", s->scope_number, ident->name, ident->head.start.line, ident->head.start.col);
        }
        s = s->parent;
        printf("\n");
    }
}

bool check_block(Typer *typer, AstBlock *block) {
    bool ok;

    typer->current_scope = block;
    for (int i = 0; i < block->statements.count; i++) {
        Ast *stmt = ((Ast **)block->statements.items)[i];
        ok = check_statement(typer, stmt);
        if (!ok) return false;
    }
    typer->current_scope = block->parent;


    return true;
}

Type *resolve_type(Typer *typer, Type *type, AstDeclaration *decl) {
    if (is_primitive_type(type->kind)) {
        return type;
    } 

    if (type->kind == TYPE_NAME) {
        Type *found = type_lookup(&typer->parser->type_table, type->name);
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
                AstExpr *constexpr = simplify_expression(typer->ce, typer->current_scope, array->capacity_expr);
                if (constexpr->head.kind != AST_LITERAL && ((AstLiteral *)(constexpr))->kind != LITERAL_INTEGER) {
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
    case PRIMITIVE_UNTYPED_INT: return U64_MAX;
    default:
        printf("Internal compiler error: Unknown type kind %d in max_integer_value()", type->kind);
        exit(1);
    }
}

bool leads_to_integer_overflow(Typer *typer, Type *lhs_type, AstExpr *expr) {
    assert(lhs_type);
    if (expr == NULL) return true;
    if (lhs_type->kind != TYPE_INTEGER && lhs_type->kind != TYPE_ENUM) return true;

    if (lhs_type->kind == TYPE_ENUM) {
        // @TODO - Handle assignment to enum with integer that might be too large
        return true;
    }

    if (expr->head.kind == AST_LITERAL) {
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

void concretise_untyped_literal(AstDeclaration *decl) {
    if (decl->type->kind == TYPE_INTEGER) {
        TypePrimitive *tp = (TypePrimitive *)decl->type;
        if (tp->kind == PRIMITIVE_UNTYPED_INT) {
            if (!(decl->flags & DECLARATION_CONSTANT)) {
                // Turn into a simple integer
                // @Incomplete - We should give it the lowest representable integer type based on the integer value
                decl->ident->type = primitive_type(PRIMITIVE_INT);
                decl->type        = primitive_type(PRIMITIVE_INT);
            }
        }
    }
}

bool check_declaration(Typer *typer, AstDeclaration *decl) {
    if (decl->flags & DECLARATION_TYPED) {
        Type *resolved_type = resolve_type(typer, decl->type, decl);
        if (!resolved_type) return false;

        decl->ident->type = resolved_type;
        decl->type        = resolved_type;
        
        Type *expr_type = check_expression(typer, decl->expr, decl->type);
        if (!expr_type) return false;
        if (!types_are_equal(decl->type, expr_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(decl->expr), "'%s' is said to be of type %s, but expression is of type %s", decl->ident->name, type_to_str(decl->type), type_to_str(expr_type));
            return false;
        }

        if (expr_type->kind == TYPE_ARRAY) {
            decl->ident->type = expr_type;
            decl->type        = expr_type;
        }
    }
    else if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        Type *resolved_type = resolve_type(typer, decl->type, decl);
        if (!resolved_type) return false;

        decl->ident->type = resolved_type;
        decl->type        = resolved_type;
    }
    else if (decl->flags & DECLARATION_INFER) {
        Type *expr_type = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;

        decl->ident->type = expr_type;
        decl->type        = expr_type;
    }
    else if (decl->flags & DECLARATION_CONSTANT) {
        Type *expr_type  = check_expression(typer, decl->expr, NULL);
        if (!expr_type) return false;

        AstExpr *const_expr = simplify_expression(typer->ce, typer->current_scope, decl->expr);
        if (const_expr->head.kind == AST_LITERAL) {
            // Swap out the current expression for the simplified expression
            // @Speed? @Note - We might in the future cleanup the already allocated expression tree as its no longer needed after this swap.
            //                 We might be taking many cache misses if we leave big gaps in the allocated ast nodes, so something like packing the ast nodes could be
            //                 a thing?
            decl->expr = const_expr;
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(decl->expr), "Constant declaration with a non-constant expression");
            return false;
        }

        decl->ident->type = expr_type;
        decl->type        = expr_type;
    }

    concretise_untyped_literal(decl);

    bool ok = leads_to_integer_overflow(typer, decl->type, decl->expr);
    if (!ok) return false;
    
    if (decl->flags & (DECLARATION_IS_FUNCTION_PARAMETER | DECLARATION_IS_STRUCT_MEMBER)) {
        // Omit sizing the declaration as it is done at the call site
        return true;
    } 

    if (typer->enclosing_function != NULL) {
        typer->enclosing_function->num_bytes_locals += decl->type->size;

        if (decl->expr && decl->expr->head.kind == AST_ARRAY_LITERAL) {
            // A little iffy, if we wanna keep it this way
            typer->enclosing_function->num_bytes_locals += 16; // data + count
        }
    } else {
        if (decl->flags & DECLARATION_CONSTANT) {
            // Constants don't need to be sized
        } else {
            // @TODO: Non-constant declarations in global scope should also be sized. Variables that live in global scope, should probably have some defined stack address of where they live, so they can be referenced locally. Still an open question on how i would do this.
            XXX;
        }
    }

    return true;
}

bool types_are_equal(Type *lhs, Type *rhs) {
    assert(!(lhs->kind == TYPE_NAME || rhs->kind == TYPE_NAME)); // Type names should have been resolved at this point

    if (is_primitive_type(lhs->kind) && is_primitive_type(rhs->kind)) {
        // @Incomplete - I think we only want constants that are untyped integers to implicitly convert
        // and not any integer type. E.g passing a variable declared as an int to a function accepting a float should fail
        //
        // Allow int to float implicit casting
        return can_cast_implicitly(rhs, lhs);

        // if (lhs->kind == TYPE_FLOAT && rhs->kind == TYPE_INTEGER) return true;
        // if (lhs->kind == TYPE_FLOAT && rhs->kind == TYPE_FLOAT) {
        //     // Only say they are equal if we can up-cast from f32 -> f64, not the other way around
        //     return lhs->size >= rhs->size;  // @Incomplete - We don't want to do this when we are dealing with literals that are untyped. Generally we want to prevent implicit down-cast e,g f64 -> f32  
        // } else {
        //     return lhs->kind == rhs->kind;
        // }
    } 
    if ((lhs->kind == TYPE_STRUCT && rhs->kind == TYPE_STRUCT) || (lhs->kind == TYPE_ENUM && rhs->kind == TYPE_ENUM)) {
        return strcmp(lhs->name, rhs->name) == 0;
    } 
    if (lhs->kind == TYPE_ARRAY && rhs->kind == TYPE_ARRAY) {
        Type *left_elem_type  = ((TypeArray *)(lhs))->elem_type;
        Type *right_elem_type = ((TypeArray *)(rhs))->elem_type;
        return types_are_equal(left_elem_type, right_elem_type);
    } 
    if (lhs->kind == TYPE_POINTER && rhs->kind == TYPE_POINTER) {
        Type *left_points_to  = ((TypePointer *)(lhs))->pointer_to;
        Type *right_points_to = ((TypePointer *)(rhs))->pointer_to;
        if (right_points_to->kind == TYPE_VOID) {
            return true;
        }
        return types_are_equal(left_points_to, right_points_to);
    }

    return false;
}

Type *check_function_call(Typer *typer, AstFunctionCall *call) {
    AstIdentifier *func_ident = lookup_from_scope(typer->parser, typer->current_scope, call->identifer->name, NULL); // @Note - Passing NULL here omits the checking that the function needs to exist before it can be called to allow arbitrary order. If, or when we get closues, this should probably work differently!
    if (func_ident == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Unknown function '%s'", call->identifer->name);
        return NULL;
    }
    if (func_ident->type->kind != TYPE_FUNCTION) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "%s is not the name of a function. Type of %s is %s", func_ident->name, func_ident->name, type_to_str(func_ident->type));
        return NULL;
    }

    AstFunctionDefn *func_defn = ((TypeFunction *)func_ident->type)->node;
    
    if (call->arguments.count != func_defn->parameters.count) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Mismatch in number of arguments. Function '%s' takes %d %s, but %d were supplied", func_defn->identifier->name, func_defn->parameters.count, func_defn->parameters.count == 1 ? "parameter" : "parameters", call->arguments.count);
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn), "Here is the definition of %s", func_defn->identifier->name);
        return NULL;
    }

    for (int i = 0; i < call->arguments.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];

        AstExpr *arg   = ((AstExpr **)(call->arguments.items))[i];
        Type *arg_type = check_expression(typer, arg, param->type);
        if (!arg_type) return NULL;

        if (!types_are_equal(param->type, arg->type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Expected argument to be of type '%s', but argument is of type '%s'", type_to_str(param->type), type_to_str(arg_type));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(param), "Here is the definition of '%s'", param->ident->name);
            return NULL;
        }
    }

    call->head.type = func_defn->return_type;
    call->func_defn = func_defn;

    //
    // Allocate space for arguments and potentially for the return value in the calling function
    // @Note - We try and follow the msvc x86 calling convention ???
    // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#parameter-passing 
    //

    int bytes_arguments = 0;
    For (AstExpr*, call->arguments, {
        Type *arg_type = it->type;
        if (it_index < 4) {
            if (arg_type->size > 8) {
                // Argument goes on stack
                bytes_arguments += arg_type->size;
            } else {
                // Argument fits into a register
            }
        } else {
            // Argument goes on stack
            bytes_arguments += align_value(arg_type->size, 8);
        }
    });

    reserve_temporary_storage(typer->enclosing_function, bytes_arguments);
    
    bool big_return_value = func_defn->return_type->size > 8;
    if (big_return_value) {
        reserve_temporary_storage(typer->enclosing_function, func_defn->return_type->size);
    }

    return func_defn->return_type;
}

bool check_assignment(Typer *typer, AstAssignment *assign) {
    Type *lhs_type  = check_expression(typer, assign->lhs, NULL);
    if (!lhs_type) return false;
    Type *expr_type = check_expression(typer, assign->expr, lhs_type);
    if (!expr_type) return false;

    bool is_member     = assign->lhs->head.kind == AST_MEMBER_ACCESS;
    bool is_array_ac   = assign->lhs->head.kind == AST_ARRAY_ACCESS;
    bool is_identifier = assign->lhs->head.kind == AST_LITERAL && ((AstLiteral *)(assign->lhs))->kind == LITERAL_IDENTIFIER;

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
        ident = lookup_from_scope(typer->parser, typer->current_scope, ((AstLiteral *)(assign->lhs))->as.value.identifier.name, (Ast *)assign->lhs);
        assert(ident); // Is checked in check_expression so no need to check it here also
        lhs_is_constant = ident->flags & IDENTIFIER_IS_CONSTANT;
    } else {
        XXX;
    }

    if (lhs_is_constant) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign new value to constant '%s'", is_member ? member->ident->name : ident->name);
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

            bool ok = leads_to_integer_overflow(typer, points_to, assign->expr);
            if (!ok) return false;

            return true;
        }
        /* Fallthrough */
    } 

    if (!types_are_equal(lhs_type, expr_type)) {
        if (is_member) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to member '%s' of type '%s'", type_to_str(expr_type), member->ident->name, type_to_str(lhs_type));
        } else if (is_identifier) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to variable '%s' of type '%s'", type_to_str(expr_type), ident->name, type_to_str(lhs_type));
        } else if (is_array_ac) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to array index '%s' of type '%s'", type_to_str(expr_type), ident->name, type_to_str(lhs_type));
        } else {
            XXX;
        }

        return false;
    }


    bool ok = leads_to_integer_overflow(typer, lhs_type, assign->expr);
    if (!ok) return false;

    return true;
}

bool check_struct_defn(Typer *typer, AstStruct *ast_struct) {
    TypeStruct *type_struct = (TypeStruct *)(type_lookup(&typer->parser->type_table, ast_struct->identifier->name));
    assert(type_struct != NULL && type_struct->head.kind == TYPE_STRUCT);


    typer->current_scope = ast_struct->scope;
    DynamicArray members = ast_struct->scope->members;
    bool ok;
    for (int i = 0; i < members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];
        ok = check_declaration(typer, member);
        if (!ok) return false;
    }
    typer->current_scope = ast_struct->scope->parent;
    
    // C-style struct alignment + padding
    int alignment = 0;
    int largest_alignment = 0;
    int offset  = 0;
    for (int i = 0; i < members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];
        
        int member_size = member->type->size;
        if (member->type->kind == TYPE_STRUCT) {
            if (!(member->type->flags & TYPE_IS_FULLY_SIZED)) {
                AstStruct *nested_struct = ((TypeStruct *)member->type)->node;
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)member, "Error: Temporarily, nested structs needs to be declared before the struct they appear in.");
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)nested_struct, "Move this struct above the struct using it");
                return false;
            }

            alignment = ((TypeStruct *)(member->type))->alignment;
        } else {
            alignment = member_size;
        }
        offset = align_value(offset, alignment);

        member->member_offset = offset;
        offset               += member_size;

        if (alignment > largest_alignment) largest_alignment = alignment;
    }

    type_struct->head.size  = align_value(offset, largest_alignment);
    type_struct->head.flags |= TYPE_IS_FULLY_SIZED;
    type_struct->alignment  = largest_alignment;
    type_struct->members    = members;

    return true;
}

bool check_for(Typer *typer, AstFor *ast_for) {
    typer->enclosing_for = ast_for;

    if (ast_for->kind == FOR_INFINITY_AND_BEYOND) {
        // No header to check!
    }
    else if (ast_for->iterable->head.kind == AST_RANGE_EXPR) {
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

        ast_for->iterator->type = primitive_type(PRIMITIVE_S32);
        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_S32);
        }

        // allocate space for the iterator, start, end and optionally for the index
        assert(typer->enclosing_function != NULL);
        typer->enclosing_function->num_bytes_locals += 24;
        if (ast_for->index) {
            typer->enclosing_function->num_bytes_locals += 8;
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
            ast_for->index->type = primitive_type(PRIMITIVE_S32);
        }

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index
        assert(typer->enclosing_function != NULL);
        typer->enclosing_function->num_bytes_locals += align_value(ast_for->iterator->type->size, 8) + 24;
    }

    bool ok = check_block(typer, ast_for->body);
    if (!ok) return false;

    typer->enclosing_for = NULL;
    return true;
}

bool check_while(Typer *typer, AstWhile *ast_while) {
    typer->enclosing_while = ast_while;

    Type *cond_type = check_expression(typer, ast_while->condition, NULL);
    if (!cond_type) return false;

    if (cond_type->kind != TYPE_BOOL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ast_while->condition, "Type mistmatch. Expeced a bool, but got type %s", type_to_str(cond_type));
        return false;
    }

    bool ok = check_block(typer, ast_while->body);
    if (!ok) return false;

    typer->enclosing_while = NULL;

    return true;
}

AstEnumerator *enum_value_is_unique(AstEnum *ast_enum, int value) {
    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        if (!etor->is_typechecked) continue;

        if (etor->value == value) return etor;
    }
    
    return NULL;
}

char *generate_c_printf_string(Typer *typer, AstPrint *print) {
    assert(print->arguments.count >= 1);

    AstExpr *arg0 = ((AstExpr **)print->arguments.items)[0];
    assert(arg0->head.kind == AST_LITERAL && ((AstLiteral *)arg0)->kind == LITERAL_STRING);

    StringBuilder sb = sb_init(((AstLiteral *)arg0)->as.value.string.length + 1);
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

    int num_arguments = print->arguments.count - 1;
    if (num_specifiers != num_arguments) {
        char *specifier_str = num_specifiers == 1 ? "specifier" : "specifiers";
        char *argument_str  = num_arguments == 1  ? "argument" : "arguments";

        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)print, "Mismatch in the number of specifiers and arguments. Got %d %s but %d %s", num_specifiers, specifier_str,  num_arguments, argument_str);
        return NULL;
    }

    // Second scan to generate the string
    c = head;
    char *copied = c;
    int chars_to_copy = 0;
    int arg_index = 1; // Starts at 1 to skip the constant string
    while (*c != '\0') {
        if (*c == '%') {
            AstExpr *arg      = ((AstExpr **)print->arguments.items)[arg_index];
            Type    *arg_type = arg->type;
            
            char *format_specifier = generate_c_format_specifier_for_type(arg_type);

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

    char *str = sb_to_string(&sb);

    return str;
}

char *generate_c_format_specifier_for_type(Type *type) {
    switch (type->kind) {
    case TYPE_BOOL:    return "%s";
    case TYPE_INTEGER: return type->size == 4 ? "%d" : "%lld";
    case TYPE_FLOAT:   return type->size == 4 ? "%f" : "%lf";
    case TYPE_STRING:  return "%s";
    case TYPE_ENUM:    return "%s";
    case TYPE_POINTER: return "0x%p";
    case TYPE_ARRAY:   return "0x%p";
    case TYPE_STRUCT: {
        StringBuilder builder = sb_init(8);

        AstStruct *struct_ = ((TypeStruct *)(type))->node;
        DynamicArray members = struct_->scope->members;

        // sb_append(&builder, "(%s)", struct_->identifier->name);
        sb_append(&builder, "{ ");
        for (int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)members.items)[i];
            char           *member_name = member->ident->name;
            Type           *member_type = member->type;

            char *format_specifier = generate_c_format_specifier_for_type(member_type);

            sb_append(&builder, "%s = %s", member_name, format_specifier);

            if (i != members.count - 1) {
                sb_append(&builder, ", ");
            }
        }
        sb_append(&builder, " }");

        return sb_to_string(&builder);
    }
    default:
        printf("Internal Compiler Error: Got unknown argument type %s in generate_c_format_specifier_for_type()\n", type_to_str(type));
        return NULL;
}
}

int count_nested_sizeable_struct_members(Typer *typer, AstStruct *struct_, int count) {
    DynamicArray members = struct_->scope->members;

    for (int i = 0; i < members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];

        if (member->type->kind == TYPE_STRUCT) {
            AstStruct *nested_struct = ((TypeStruct *)member->type)->node;
            count += count_nested_sizeable_struct_members(typer, nested_struct, 0);
        } else {
            count += 1;
        }
    }

    return count;
}

bool check_statement(Typer *typer, Ast *stmt) {

    // Each statement might reserve a chunk of memory for temporary values such as function arguments and return values.
    // This call limits the lifetime of the temporary storage to that of a single statement.
    reset_temporary_storage(typer->enclosing_function);

    switch (stmt->kind) {
    case AST_DECLARATION: return check_declaration(typer, (AstDeclaration *)(stmt));
    case AST_ASSIGNMENT:  return check_assignment(typer, (AstAssignment *)(stmt));
    case AST_PRINT: {
        AstPrint *print = (AstPrint *)(stmt);
        if (print->arguments.count < 1) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)print, "Missing first argument to print");
            return false;
        }

        for (int i = 0; i < print->arguments.count; i++) {
            AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
            Type *arg_type = check_expression(typer, arg, NULL);
            if (!arg_type) return false;
            if (i == 0) {
                if (arg_type->kind != TYPE_STRING) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "Print expects the first argument to have type string, but argument is of type %s", type_to_str(arg_type));
                    return false;
                }
                if (arg->head.kind != AST_LITERAL && ((AstLiteral *)arg)->kind != LITERAL_STRING) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "First argument to printf must be a constant string");
                    return false;
                }
            }
        }

        //
        //  Do sizing of the arguments
        //

        // Since we are pushing all of the arguments on the stack to later pop them,
        // we need to allocate 8 bytes of space for each one of them.
        // @NOTE: We reuse the stack space for arguments
        // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170 
        int num_c_args = 1;
        for (int i = 1; i < print->arguments.count; i++) {
            AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
            Type    *arg_type = arg->type;

            if (arg_type->kind == TYPE_STRUCT) {
                AstStruct *struct_ = ((TypeStruct *)arg_type)->node;
                num_c_args += count_nested_sizeable_struct_members(typer, struct_, 0);
            } else {
                num_c_args += 1;
            }
        }
        print->c_args = num_c_args;

        if (num_c_args > 4) {
            int bytes_args = (num_c_args - 4) * 8;
            reserve_temporary_storage(typer->enclosing_function, bytes_args);
        }

        char *c_string = generate_c_printf_string(typer, print);
        if (!c_string) return false;

        print->c_string = c_string;

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

        bool ok = check_block(typer, ast_if->then_block);
        if (!ok) return false;

        for (int i = 0; i < ast_if->else_ifs.count; i++) {
            AstIf *else_if = &(((AstIf *)(ast_if->else_ifs.items))[i]);
            ok = check_statement(typer, (Ast *)(else_if));
            if (!ok) return false;
        }
        if (ast_if->else_block) {
            ok = check_block(typer, ast_if->else_block);
            if (!ok) return false;
        }

        return true;
    }
    case AST_BLOCK: {
        AstBlock *block = (AstBlock *)(stmt);
        return check_block(typer, block);
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

        if (!types_are_equal(ast_return->expr->type, ef->return_type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_return), "Type mismatch. Returning type %s from function '%s' with return type %s", type_to_str(ast_return->expr->type), ef->identifier->name, type_to_str(ef->return_type));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(ef), "... Here is the function signature for '%s'", ef->identifier->name);
            return false;
        }

        ast_return->enclosing_function = ef;
        return true;
    }
    case AST_BREAK_OR_CONTINUE: return check_break_or_continue(typer, (AstBreakOrContinue *)(stmt));
    case AST_FUNCTION_DEFN: return check_function_defn(typer, (AstFunctionDefn *)(stmt));
    case AST_STRUCT: return check_struct_defn(typer, (AstStruct *)(stmt));
    case AST_ENUM: return check_enum_defn(typer, (AstEnum *)stmt);
    case AST_FOR: return check_for(typer, (AstFor *)(stmt));
    case AST_WHILE: return check_while(typer, (AstWhile *)(stmt));
    default:
        XXX;
    }
}

bool check_break_or_continue(Typer *typer, AstBreakOrContinue *boc) {

    if (!typer->enclosing_for && !typer->enclosing_while) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(boc), "%s is only allowed inside for-loops or while-loops", boc->token.type ==TOKEN_BREAK ? "break" : "continue");
        return false;
    }

    if (typer->enclosing_for && typer->enclosing_while) {
        // We are inside a for loop and a while loop. We need to figure out
        // which is the inner one
        if (is_a_before_b((Ast *)typer->enclosing_for, (Ast *)typer->enclosing_while)) {
            boc->enclosing_loop = TOKEN_WHILE;
            boc->enclosing.while_loop = typer->enclosing_while;
        } else {
            boc->enclosing_loop = TOKEN_FOR;
            boc->enclosing.for_loop = typer->enclosing_for;
        }
    }

    if (typer->enclosing_for) {
        boc->enclosing_loop = TOKEN_FOR;
        boc->enclosing.for_loop = typer->enclosing_for;
    } else {
        boc->enclosing_loop = TOKEN_WHILE;
        boc->enclosing.while_loop = typer->enclosing_while;
    }

    return true;
}

Type *check_function_defn(Typer *typer, AstFunctionDefn *func_defn) {
    typer->enclosing_function = func_defn;

    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)func_defn->parameters.items)[i];
        bool ok = check_declaration(typer, param);
        if (!ok) return NULL;
    }
    
    Type *return_type = resolve_type(typer, func_defn->return_type, NULL);
    if (!return_type) return NULL;
    func_defn->return_type = return_type;

    typer->current_scope = func_defn->body;

    bool ok = check_block(typer, func_defn->body);
    if (!ok) return NULL;

    typer->enclosing_function = NULL;

    // Do a narrow scan for a return statement
    bool has_return = false;
    For (Ast*, func_defn->body->statements, {
        if (it->kind == AST_RETURN) {
            has_return = true;
            break;
        }
    });

    if (!has_return && func_defn->return_type->kind != TYPE_VOID && !func_defn->is_extern) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)func_defn, "Function is missing a return");
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)func_defn, "Put an explicit return at the outer scope of the function. In the future we should be able to detect nested returns");
        return NULL;
    }

    //
    // Do sizing for parameters
    //
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)func_defn->parameters.items)[i];
        func_defn->num_bytes_locals += param->type->size;
    }

    // Do sizing of large return values
    if (func_defn->return_type->size > 8) {
        func_defn->num_bytes_locals += 8;
    }


    return return_type;
}

Type *check_enum_defn(Typer *typer, AstEnum *ast_enum) {
    TypeEnum *enum_defn = (TypeEnum *)(type_lookup(&typer->parser->type_table, ast_enum->identifier->name));
    assert(enum_defn != NULL); // Was put in during parsing

    int auto_increment_value = 0;
    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        if (etor->expr) {
            Type *ok = check_expression(typer, etor->expr, (Type *)(enum_defn)); // Type definition of the enum gets to be the ctx type so we can refer to enum members inside the enum with the shorthand syntax .ENUM_MEMBER
            if (!ok) return NULL;

            AstExpr *constexpr = simplify_expression(typer->ce, typer->current_scope, etor->expr); // @ScopeRefactoring - We still need to give enums their own scope. This probably doesn't work!!!
            if (constexpr->head.kind != AST_LITERAL) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Value must be a constant expression");
                return NULL;
            }
            if (constexpr->type->kind != TYPE_INTEGER) { // @Robustness - Check that the value of the integer does not overflow the backing type if made explicit. Maybe if the value goes beyond the default int we promote the integer type to fit the largest value???
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Enum value must have type int, but value is of type %s", type_to_str(etor->expr->type));
                return NULL;
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
            return NULL;
        }

        etor->is_typechecked = true;
    }


    enum_defn->head.size = enum_defn->backing_type->size;
    return (Type *)enum_defn;
}

AstEnumerator *find_enum_member(AstEnum *enum_defn, char *name) {
    for (int i = 0; i < enum_defn->enumerators.count; i++) {
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

        if (enum_defn->node->enumerators.count <= 5) {
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
        }

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
    for (int i = 0; i < flattened_array->count; i++) {
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
        for (int j = 0; j < elements.count; j++) {
            
            TypeArray *array = ((TypeArray **)(elements.items))[j];
            AstArrayLiteral *array_lit = array->node;

            if (d == (int)(flattened_array->count - 1)) assert(array->elem_type->kind != TYPE_ARRAY);

            int capacity   = array_lit->expressions.count;
            int array_size = capacity * array->elem_type->size; 

            if (capacity   > capacity_of_biggest_array) capacity_of_biggest_array = capacity;
            if (array_size > size_of_biggest_array)     size_of_biggest_array = array_size;
        }

        // Second pass to set all the inner arrays size and capacity to be the biggest
        for (int j = 0; j < elements.count; j++) {
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
        array->head.head.kind  = AST_TYPE;
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

    for (int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr *expr = ((AstExpr **)(array_lit->expressions.items))[i];

        bool ok = leads_to_integer_overflow(typer, array->elem_type, expr);
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
    Type *first_element_type = ((AstExpr **)(array_lit->expressions.items))[0]->type;
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
    if      (expr->head.kind == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr), ctx_type);
    else if (expr->head.kind == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr), ctx_type);
    else if (expr->head.kind == AST_CAST)           result = check_cast(typer, (AstCast *)(expr), ctx_type);
    else if (expr->head.kind == AST_FUNCTION_CALL)  result = check_function_call(typer, (AstFunctionCall *)(expr));
    else if (expr->head.kind == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_ARRAY_LITERAL)  result = check_array_literal(typer, (AstArrayLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_ENUM_LITERAL)   result = check_enum_literal(typer, (AstEnumLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_MEMBER_ACCESS)  result = check_member_access(typer, (AstMemberAccess *)(expr));
    else if (expr->head.kind == AST_ARRAY_ACCESS)   result = check_array_access(typer, (AstArrayAccess *)(expr));
    else if (expr->head.kind == AST_RANGE_EXPR)     result = primitive_type(PRIMITIVE_S64); // @Investigate - Shouldn't this be checked for both sides being integers???
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_to_str((Ast *)expr));
        exit(1);
    }

    expr->type = result;
    return result;
}

Type *check_member_access(Typer *typer, AstMemberAccess *ma) {
    Type *type_lhs = check_expression(typer, ma->left, NULL);
    if (!type_lhs) return NULL;

    //
    // 1'st case : Explicit enum value
    // @Cleanup - I think this bit of code can be better put elsewhere in a unary dot as that is what we are checking for here. e.g .ENUM
    //            That way, i wouldn't need to have an access kind for the member
    //
    AstExpr *lhs = ma->left;
    if (type_lhs->kind == TYPE_ENUM && lhs->head.kind == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) {
        char *ident_name    = ((AstLiteral *)(lhs))->as.value.identifier.name;
        TypeEnum *enum_defn = ((TypeEnum *)(type_lhs));
        if (strcmp(ident_name, enum_defn->identifier->name) == 0) {

            AstExpr *rhs = ma->right;
            if (rhs->head.kind == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
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

    //
    // 2'nd case : Member access on struct
    //
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
    if (rhs->head.kind == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
        char *member_name = ((AstLiteral *)(rhs))->as.value.identifier.name;
        
        AstDeclaration *member = get_struct_member(struct_defn->node, member_name);
        if (!member) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "'%s' is not a member of '%s'", member_name, type_to_str((Type *)struct_defn));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn), "Here is the definition of %s", type_to_str((Type *)struct_defn));
            return NULL;
        }

        ma->access_kind   = MEMBER_ACCESS_STRUCT;
        ma->struct_member = member;
        return member->type;
    } else {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "Invalid member expression");
        return NULL;
    }

}

Type *check_struct_literal(Typer *typer, AstStructLiteral *literal, Type *ctx_type) {
    TypeStruct *struct_defn = NULL;
    if (literal->explicit_type) {
        // Explicit type is used
        Type *type = literal->explicit_type;
        if (type->kind == TYPE_STRUCT) {
            // Explicit type have already been resolved @Note - Currently this only happens in check_array_literal where the first expression is typechecked, and then all the expressions are again typechecked against that first expression, so we might end here.
            return literal->explicit_type;
        }

        if (type->kind == TYPE_NAME) {
            Type *found = type_lookup(&typer->parser->type_table, type->name);
            if (!found) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "Unknown type '%s'", type_to_str(type));
                return NULL;
            }

            if (found->kind != TYPE_STRUCT) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "'%s' is not the name of a struct. Type of '%s' is '%s'", found->name, found->name, type_to_str(type));
                return NULL;
            }
            
            literal->explicit_type = found;
            struct_defn = (TypeStruct *)(found);
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Struct literal cannot conform to type '%s'", type_to_str(ctx_type));
            return NULL;
        }
    }
    else {
        // Type is inferred from the type on the declaration
        if (ctx_type == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Type of struct literal could not be infered from the context");
            return NULL;
        }

        if (ctx_type->kind == TYPE_NAME) {
            printf("%s:%d: Internal Compiler Error: Struct or enum type was not specialized before infering the struct literal", __FILE__, __LINE__);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(literal), "... while infering this struct literal of type '%s'", type_to_str(ctx_type));
            return NULL;
        }

        if (ctx_type->kind != TYPE_STRUCT) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Struct literal cannot conform to type '%s'", type_to_str(ctx_type));
            return NULL;
        }

        struct_defn = (TypeStruct *)(ctx_type);
    }

    DynamicArray members = struct_defn->node->scope->members;
    int curr_member_index = 0;
    for (int i = 0; i < literal->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(literal->initializers.items))[i];

        if (init->designator != NULL) {
            AstDeclaration *member = get_struct_member(struct_defn->node, init->designator->name);
            if (member == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->designator), "'%s' is not a member of '%s'", init->designator->name, struct_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            Type *member_type = member->type;
            Type *value_type  = check_expression(typer, init->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->ident->name, type_to_str(member_type), type_to_str(value_type));
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
            Type *member_type = member->type;
            Type *value_type  = check_expression(typer, init->value, member_type);
            if (!value_type) return NULL;

            if (!types_are_equal(member_type, value_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->ident->name, type_to_str(member_type), type_to_str(value_type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            init->member = member;
            curr_member_index += 1;
        }
    }

    Type *evaluated_type;
    if (literal->explicit_type != NULL) {
        evaluated_type = literal->explicit_type;
    } else {
        evaluated_type = ctx_type;
    }

    // Reserve space for the struct literal if its bigger than 8 bytes
    // @Incomplete: Handle structs smaller than 8 bytes! They should'nt need to be put on the stack!
    reserve_temporary_storage(typer->enclosing_function, evaluated_type->size);

    return evaluated_type;
}

Type *biggest_type(Type *lhs, Type *rhs) {
    if (lhs->size >= rhs->size) return lhs;
    else                        return rhs;
}

Type *check_binary(Typer *typer, AstBinary *binary, Type *ctx_type) {

    Type *ti_lhs = NULL;
    Type *ti_rhs = NULL;

    // Use the type info from the opposite side of an auto-cast to give it a context type
    bool checked_both_sides = false;
    if (binary->left->head.kind == AST_CAST || binary->right->head.kind == AST_CAST) {
        if (binary->left->head.kind == AST_CAST) {
            AstCast *cast = (AstCast *)binary->left;
            if (cast->is_auto && !ctx_type) {
                ti_rhs = check_expression(typer, binary->right, NULL);
                ti_lhs = check_expression(typer, binary->left, ti_rhs);
                checked_both_sides = true;
            }
        }
        else if (binary->right->head.kind == AST_CAST) {
            AstCast *cast = (AstCast *)binary->right;
            if (cast->is_auto && !ctx_type) {
                ti_lhs = check_expression(typer, binary->left, NULL);
                ti_rhs = check_expression(typer, binary->right, ti_lhs);
                checked_both_sides = true;
            }
        }
    }

    if (is_comparison_operator(binary->operator)) {
        // If one of the two sides contains an enum literal then use the other side to provide a potential enum
        // context type so that we can have expressions like this: "if x == .Some_Enum"
        if (binary->left->head.kind == AST_ENUM_LITERAL || binary->right->head.kind == AST_ENUM_LITERAL) {
            bool enum_lit_is_lhs = binary->left->head.kind == AST_ENUM_LITERAL;
            AstExpr *enum_side  = enum_lit_is_lhs ? binary->left  : binary->right;
            AstExpr *other_side = enum_lit_is_lhs ? binary->right : binary->left;

            Type *os_type = check_expression(typer, other_side, ctx_type);
            if (!os_type) return NULL;

            if (os_type->kind == TYPE_ENUM) {
                Type *es_type = check_expression(typer, enum_side, os_type);
                if (!es_type) return NULL;

                ti_lhs = enum_lit_is_lhs ? es_type : os_type;
                ti_rhs = enum_lit_is_lhs ? os_type : es_type;
                checked_both_sides = true;
            } else {
                // Check if we can concretize the enum literal with the ctx type
                Type *es_type = check_expression(typer, enum_side, ctx_type);
                if (!es_type) return NULL;

                ti_lhs = enum_lit_is_lhs ? es_type : os_type;
                ti_rhs = enum_lit_is_lhs ? os_type : es_type;
                checked_both_sides = true;
            }
        }
    }


    if (!checked_both_sides) {
        ti_lhs = check_expression(typer, binary->left, ctx_type);
        ti_rhs = check_expression(typer, binary->right, ctx_type);
    }

    if (!ti_lhs || !ti_rhs) return NULL;


    TypeKind lhs = ti_lhs->kind;
    TypeKind rhs = ti_rhs->kind;

    if (lhs == TYPE_STRUCT && rhs == TYPE_STRUCT) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Binary operation between two structs is not yet supported!");
        return NULL;
    }

    if (strchr("+-*/^", binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return ti_lhs;
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return ti_rhs;
        if ((lhs == TYPE_INTEGER && rhs == TYPE_ENUM) || (lhs == TYPE_ENUM && rhs == TYPE_INTEGER)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)binary, "Enums must be explicitly casted to integers before applying arithmetic on them");
            return NULL;
        }
    }
    if (binary->operator == '%') {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
    }
    if (is_boolean_operator(binary->operator)) {
        if (lhs == TYPE_BOOL && rhs == TYPE_BOOL)       return primitive_type(PRIMITIVE_BOOL);
    }
    if (is_comparison_operator(binary->operator)) {
        if (lhs == TYPE_BOOL    && rhs == TYPE_BOOL)    return primitive_type(PRIMITIVE_BOOL);

        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_POINTER && rhs == TYPE_POINTER) return primitive_type(PRIMITIVE_BOOL);

        if (lhs == TYPE_ENUM    && rhs == TYPE_ENUM) {
            if (strcmp(ti_lhs->name, ti_rhs->name) == 0) {
                return primitive_type(PRIMITIVE_BOOL);
            }
        }
        if (lhs == TYPE_ENUM    && rhs == TYPE_INTEGER) return primitive_type(PRIMITIVE_BOOL);
        if (lhs == TYPE_INTEGER && rhs == TYPE_ENUM)    return primitive_type(PRIMITIVE_BOOL);


        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Type '%s' and '%s' are not comparable", type_to_str(ti_lhs), type_to_str(ti_rhs));
        return NULL;
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
        if (unary->expr->head.kind == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
            lvalue = true;
        }
        else if (unary->expr->head.kind == AST_ARRAY_ACCESS) {
            lvalue = true;
        }
        else if (unary->expr->head.kind == AST_MEMBER_ACCESS) {
            lvalue = true;
        }

        if (!lvalue) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Expression is not an addressable value (lvalue)");
            return NULL;
        }

        TypePointer *ptr     = type_alloc(&typer->parser->type_table, sizeof(TypePointer));
        ptr->head.head.kind  = AST_TYPE;
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

bool can_cast_implicitly(Type *from, Type *to) {
    // Can we cast without truncation?
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_INTEGER) return from->size <= to->size;
    if (from->kind == TYPE_FLOAT   && to->kind == TYPE_FLOAT)   return from->size <= to->size;

    // Allow untyped integers to cast to floats
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_FLOAT) {
        return ((TypePrimitive *)from)->kind == PRIMITIVE_UNTYPED_INT;
    }
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_INTEGER) {
        return ((TypePrimitive *)to)->kind == PRIMITIVE_UNTYPED_INT;
    }

    if (from->kind == TYPE_BOOL   && to->kind == TYPE_BOOL) return true;
    if (from->kind == TYPE_STRING && to->kind == TYPE_STRING) return true;

    return false;
}

bool can_cast_explicitly(Type *from, Type *to) {
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_INTEGER) return true;
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_FLOAT) return true;
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_BOOL) return true;
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_ENUM) return true;

    if (from->kind == TYPE_FLOAT   && to->kind == TYPE_FLOAT) return true;
    if (from->kind == TYPE_FLOAT   && to->kind == TYPE_INTEGER) return true;
    if (from->kind == TYPE_FLOAT   && to->kind == TYPE_BOOL) return true;

    if (from->kind == TYPE_ENUM    && to->kind == TYPE_INTEGER) return true;
    if (from->kind == TYPE_ENUM    && to->kind == TYPE_ENUM) {
        return strcmp(from->name, to->name) == 0;
    }

    if (from->kind == TYPE_BOOL    && to->kind == TYPE_INTEGER) return true;
    if (from->kind == TYPE_BOOL    && to->kind == TYPE_FLOAT) return true;  

    if (from->kind == TYPE_POINTER && to->kind == TYPE_INTEGER) return true;    // address to a u64 number
    if (from->kind == TYPE_POINTER && to->kind == TYPE_BOOL) return true;       // null pointer = false, otherwise = true

    return false;
}

Type *check_cast(Typer *typer, AstCast *cast, Type *ctx_type) {

    Type *wanted_type = NULL;
    if (cast->is_auto) {
        if (ctx_type == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)cast, "Unable to infer what the auto-cast type should be from the context");
            return NULL;
        }
        wanted_type = ctx_type;
    } else {
        cast->cast_to = resolve_type(typer, cast->cast_to, NULL);
        if (!cast->cast_to) return NULL;
        wanted_type = cast->cast_to;
    }

    Type *expr_type = check_expression(typer, cast->expr, NULL);
    if (!expr_type) return NULL;

    if (!can_cast_explicitly(expr_type, wanted_type)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)cast, "Invalid Cast: Type %s can not be casted to a %s", type_to_str(expr_type), type_to_str(wanted_type));
        return NULL;
    }

    cast->cast_to = wanted_type;

    return wanted_type;
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
    case LITERAL_INTEGER:   return (ctx_type && ctx_type->kind == TYPE_INTEGER) ? ctx_type : primitive_type(PRIMITIVE_UNTYPED_INT);
    case LITERAL_FLOAT:     return (ctx_type && ctx_type->kind == TYPE_FLOAT)   ? ctx_type : primitive_type(PRIMITIVE_FLOAT);
    case LITERAL_STRING:    return (ctx_type && ctx_type->kind == TYPE_STRING)  ? ctx_type : primitive_type(PRIMITIVE_STRING);
    case LITERAL_BOOLEAN:   return (ctx_type && ctx_type->kind == TYPE_BOOL)    ? ctx_type : primitive_type(PRIMITIVE_BOOL);
    case LITERAL_NULL:      return (Type *)t_null_ptr;
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.value.identifier.name;
        AstIdentifier *ident = lookup_from_scope(typer->parser, typer->current_scope, ident_name, (Ast *)literal);
        if (ident == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Undeclared variable '%s'", ident_name);
            return NULL;
        }

        assert(ident->type);
        return ident->type;
    }}

    XXX;
}

void reserve_temporary_storage(AstFunctionDefn *func_defn, int size) {
    func_defn->temp_ptr += size;
    
    if (func_defn->temp_ptr > func_defn->num_bytes_temporaries) {
        func_defn->num_bytes_temporaries = func_defn->temp_ptr;
    }
}

int push_temporary_value(AstFunctionDefn *func_defn, int size) {
    // Temporary storage lives after the shadow-space and local variables
    int aligned_size = size;
    int loc = - (align_value(func_defn->num_bytes_locals, 8) + func_defn->temp_ptr + aligned_size);

    // Assert that we don't use more space than what we have reserved during the sizing step!
    assert(func_defn->num_bytes_temporaries > func_defn->temp_ptr);

    func_defn->temp_ptr += aligned_size;

    return loc;
}

// Pop the latest 8 bytes from temporary storage
int pop_temporary_value(AstFunctionDefn *func_defn) {
    assert(func_defn);

    int loc = - (align_value(func_defn->num_bytes_locals, 8) + func_defn->temp_ptr);

    func_defn->temp_ptr -= 8;
    assert(func_defn->temp_ptr >= 0);

    return loc;
}

void reset_temporary_storage(AstFunctionDefn *func_defn) {
    if (func_defn == NULL) return;

    func_defn->temp_ptr = 0;
}

// @Cleanup - Still kinda handy until we don't use AstIdentifier as the primary lookup in scopes
AstDeclaration *get_struct_member(AstStruct *struct_defn, char *name) {
    return find_member_in_scope(struct_defn->scope, name);
}

bool is_untyped_literal(Type *type) {
    if (type->kind == TYPE_INTEGER) {
        TypePrimitive *prim = (TypePrimitive *)type;
        return prim->kind == PRIMITIVE_UNTYPED_INT;
    }

    return false;
}
