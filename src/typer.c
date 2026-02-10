#include "const_expr.c"
#include "lib/string_builder.h"

typedef struct Typer {
    Parser *parser;
    ConstEvaluator *const_evaluator;

    AstFile  *current_file;
    AstBlock *current_scope;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to
    AstFor          *enclosing_for;      // Used by break or continue statements to know where to branch to
    AstWhile        *enclosing_while;    // Same as above

    AstDeclaration  *inside_declaration;  // Set if we are currently type checking a declaration
    AstExtern       *inside_extern_block; // Set if we are currently type checking function definitions inside an extern block

    TypeAny         *type_any;       // The resolved type of 'any'
    
    int array_literal_depth;

    DynamicArray    scope_rewrites; // of *void. [AstBlock*, AstIdentifier*, AstBlock*, AstIdentifier*]
} Typer;

bool  check_block(Typer *typer, AstBlock *block);
bool  check_statement(Typer *typer, Ast *stmt);
bool  check_print(Typer *typer, AstPrint *print);
bool  check_break_or_continue(Typer *typer, AstBreakOrContinue *boc);
Type *check_expression(Typer *typer, AstExpr *expr, Type *ctx_type);
Type *check_binary(Typer *typer, AstBinary *binary, Type *ctx_type);
Type *check_unary(Typer *typer, AstUnary *unary, Type *ctx_type);
Type *check_cast(Typer *typer, AstCast *cast, Type *ctx_type);
Type *check_literal(Typer *typer, AstLiteral *literal, Type *ctx_type);
Type *check_struct_literal(Typer *typer, AstStructLiteral *struct_literal, Type *ctx_type);
Type *check_enum_literal(Typer *typer, AstEnumLiteral *enum_literal, Type *ctx_type);
Type *check_member_access(Typer *typer, AstMemberAccess * ma);
Type *check_typeof(Typer *typer, AstTypeof *ast_typeof);
Type *check_enum_defn(Typer *typer, AstEnum *ast_enum);
bool  check_function_defn(Typer *typer, AstFunctionDefn *func_defn);

char *type_to_str(Type *type);
bool types_are_equal(Type *lhs, Type *rhs);
bool can_cast_implicitly(Type *from, Type *to);
bool can_cast_explicitly(Type *from, Type *to);
bool is_bitwise_operator(TokenType op);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);
bool is_untyped_type(Type *type);
bool is_untyped_literal(AstExpr *expr);
void statically_cast_literal(AstLiteral *untyped_literal, Type *cast_into);
void reserve_local_storage(AstFunctionDefn *func_defn, int size);
void reserve_temporary_storage(AstFunctionDefn *func_defn, int size);
void reset_temporary_storage();
AstIdentifier *get_struct_member(AstStruct *struct_defn, char *name);

AstCast *generate_cast_to_any(Typer *typer, AstExpr *expr);
char *generate_c_format_specifier_for_type(Type *type);


Typer typer_init(Parser *parser, ConstEvaluator *ce) {
    Typer typer = {0};
    typer.parser = parser;
    typer.const_evaluator = ce;

    typer.current_file        = NULL;
    typer.current_scope       = NULL;
    typer.enclosing_function  = NULL;
    typer.enclosing_for       = NULL;
    typer.enclosing_while     = NULL;
    typer.inside_declaration  = NULL;
    typer.scope_rewrites      = da_init(8, sizeof(void*));
    typer.array_literal_depth = 0;

    return typer;
}

bool check_file(Typer *typer, AstFile *ast_file) {
    typer->current_file        = ast_file;
    typer->current_scope       = ast_file->scope;
    typer->enclosing_function  = NULL;
    typer->enclosing_for       = NULL;
    typer->enclosing_while     = NULL;
    typer->inside_declaration  = NULL;
    typer->array_literal_depth = 0;

    // @Cleanup @CopyPasta :CopyPasteCurrentFile 
    typer->parser->current_file  = ast_file;
    typer->parser->current_scope = ast_file->scope;

    for (int i = 0; i < ast_file->statements.count; i++) {
        Ast *stmt = ((Ast **)(ast_file->statements.items))[i];
        bool ok = check_statement(typer, stmt);
        if (!ok) return false; // @Improvement - Maybe proceed with typechecking if we can still continue after seing the error
    }

    return true;
}

void dump_scope_levels(AstBlock *scope) {
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

void dump_struct(AstStruct *struct_defn) {
    printf("%s :: struct {\n", struct_defn->identifier->name);

    DynamicArray members = struct_defn->scope->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        printf("    %s: %s;\n", member->name, type_to_str(member->type));
    }
    printf("}\n");
}

void typer_restore_state(Typer *typer, Typer saved_state) {
    typer->current_file        = saved_state.current_file;
    typer->current_scope       = saved_state.current_scope;
    typer->enclosing_function  = saved_state.enclosing_function;
    typer->enclosing_for       = saved_state.enclosing_for;
    typer->enclosing_while     = saved_state.enclosing_while;
    typer->array_literal_depth = saved_state.array_literal_depth;

    // @Cleanup @CopyPasta :CopyPasteCurrentFile 
    typer->parser->current_file  = saved_state.current_file;
    typer->parser->current_scope = saved_state.current_scope;
}

bool import_declarations(Parser *parser, AstImport *import, AstFile *to_file) {
    AstFile *from_file = import->imported_file;

    for (int i = 0; i < from_file->scope->identifiers.count; i++) {
        AstIdentifier *src_ident = ((AstIdentifier **)from_file->scope->identifiers.items)[i];

        // Don't include the identifier if it does not originate from the file it self
        if (src_ident->head.file != from_file) {
            continue;
        }

        AstIdentifier *existing = add_identifier_to_scope(parser, to_file->scope, src_ident);
        if (existing != NULL) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)src_ident, "Import of identifier '%s' conflicts with existing identifier", src_ident->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "Here is the existing identifier ...");
            report_error_ast(parser, LABEL_HINT, (Ast *)import, "Either namespace the import or rename one of the identifiers in the files");
            return false;
        }
    }

    return true;
}

bool check_import(Typer *typer, AstImport *import) {
    assert(import->imported_file != NULL);

    // Go typecheck the imported file
    Typer current_state = *typer;

    bool ok = check_file(typer, import->imported_file);
    if (!ok) {
        return false;
    }

    typer_restore_state(typer, current_state);

    // Copy all the declarations from the imported file into the current files scope
    ok = import_declarations(typer->parser, import, typer->current_file);
    if (!ok) {
        return false;
    }

    return true;
}

bool check_extern(Typer *typer, AstExtern *ast_extern) {

    // We don't call check_block() as it expects a normal scope, while the extern scope lives at the same level
    typer->inside_extern_block = ast_extern;
    for (int i = 0; i < ast_extern->block->statements.count; i++) {
        Ast *stmt = ((Ast **)ast_extern->block->statements.items)[i];
        bool ok = check_statement(typer, stmt);
        if (!ok) return false;
    }
    typer->inside_extern_block = NULL;
    
    return true;
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

int round_up_to_nearest_power_of_two(int value) {
    if (value <= 1) return 2;
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

void dump_scope_recursive(Parser *parser, AstBlock *scope) {
    if (scope == NULL) return;

    printf("---------------------- Scope %d ----------------------\n", scope->scope_number);
    for (int i = 0; i < scope->identifiers.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **) scope->identifiers.items)[i];
        printf("%s: %s\n", ident->name, type_to_str(ident->type));
    }

    dump_scope_recursive(parser, scope->parent);
}

Type *resolve_type(Typer *typer, Type *type) {
    if (is_primitive_type(type->kind)) {
        return type;
    }

    if (type->kind == TYPE_ANY) {
        if (!typer->inside_extern_block)  {
            TypeAny *any = (TypeAny *)type;
    
            // Look for the Type :: struct {...} in the "reflect" package
            // dump_scope_recursive(typer->parser, typer->current_scope);

            if (typer->type_any == NULL) {
                AstIdentifier *type_any_identifier = lookup_from_scope(typer->parser, typer->current_scope, "Any");

                if (!type_any_identifier) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)any, "Compiler Error: Failed to find the 'Any' struct in reflect.sd");
                    return NULL;
                }

                if (type_any_identifier->type == NULL || type_any_identifier->type->kind != TYPE_STRUCT) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)any, "Compiler Error: Type 'Any' was unresolved or of a different type in typechecking");
                    return NULL;
                }

                // if (strcmp(type_any_identifier->head.file->absolute_path, "reflect") != 0) {
                //     report_error_ast(typer->parser, LABEL_ERROR, (Ast *)any, "Compiler Error: Type 'Any' was linked to a different 'Any' than that found in reflect.sd");
                //     return NULL;
                // }
                
                // Construct a global instance of TypeAny with the struct definition of the struct "Any" found in the reflect.sd package
                TypeAny *any = ast_allocate(typer->parser, sizeof(TypeAny));
                any->head.head.kind  = AST_TYPE;
                any->head.head.file  = type_any_identifier->head.file;
                any->head.head.start = type_any_identifier->type->head.start;
                any->head.head.end   = type_any_identifier->type->head.end;
                any->head.head.flags = AST_FLAG_COMPILER_GENERATED;
                any->head.kind       = TYPE_ANY;
                any->head.size       = 16;
                any->struct_defn     = (TypeStruct *)type_any_identifier->type;

                typer->type_any = any;
            }

            any->struct_defn = typer->type_any->struct_defn;
        }

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

        Type *points_to = resolve_type(typer, ptr->pointer_to);
        if (!points_to) return NULL;

        ptr->pointer_to = points_to;
        return (Type *)(ptr);
    }

    if (type->kind == TYPE_VARIADIC) {
        TypeVariadic *variadic = (TypeVariadic *)type;

        Type *resolved_type = resolve_type(typer, variadic->type);
        if (!resolved_type) return NULL;

        variadic->type = resolved_type;
        
        return (Type *)variadic;
    }

    if (type->kind == TYPE_ARRAY) {
        TypeArray *array = (TypeArray *)(type);

        // Resolve the size of any inner element type
        Type *elem_type = resolve_type(typer, array->elem_type);
        if (!elem_type) return NULL;

        if (elem_type->kind == TYPE_VOID) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(elem_type), "Invalid type. Cannot have %s of type void", text_bold("array"));
            report_error_ast(typer->parser, LABEL_HINT, (Ast *)(elem_type), "If you wanted the array to accept any type, then make it []any");
            return NULL;
        }

        array->elem_type = elem_type;

        switch (array->array_kind) {
        case ARRAY_FIXED: {
            assert(array->capacity_expr);

            Type *cap_type = check_expression(typer, array->capacity_expr, NULL);
            if (!cap_type) return NULL;

            AstExpr *constexpr = simplify_expression(typer->const_evaluator, typer->current_scope, array->capacity_expr);
            if (constexpr->head.kind != AST_LITERAL && ((AstLiteral *)(constexpr))->kind != LITERAL_INTEGER) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array->capacity_expr), "Size of the array must be an integer constant");
                return NULL;
            }

            long long capacity = ((AstLiteral *)(constexpr))->as.value.integer;
            if (capacity < 0) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array->capacity_expr), "Size of the array must be non-negative. Value is %lld", capacity);
                return NULL;
            }

            array->capacity  = capacity;
            array->count     = capacity;
            array->head.size = capacity * elem_type->size;
            break;
        }
        case ARRAY_SLICE: {
            array->capacity  = 0;
            array->count     = 0;
            array->head.size = 16; // Reserve 16 bytes for .data, .count
            break;
        }
        case ARRAY_DYNAMIC: {
            array->capacity  = 2;
            array->count     = 0;
            array->head.size = 24; // Reserve 24 bytes for .data, .count and .capacity
            break;
        }
        }

        return (Type *)(array);
    }

    if (type->kind == TYPE_FUNCTION) {
        TypeFunction *func_type = (TypeFunction *) type;

        for (int i = 0; i < func_type->node->parameters.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)func_type->node->parameters.items)[i];
            Type *resolved_type = resolve_type(typer, param->type);
            if (!resolved_type) return NULL;
            param->type = resolved_type;
        }

        for (int i = 0; i < func_type->node->return_types.count; i++) {
            Type **return_type_ptr = &((Type **)func_type->node->return_types.items)[i];
            Type *resolved_type = resolve_type(typer, *return_type_ptr);
            if (!resolved_type) return NULL;
            *return_type_ptr = resolved_type;
        }

        return (Type *)func_type;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *) type, "Compiler Error: Failed to resolve type %s", type_to_str(type));

    return NULL;
}

unsigned long long max_integer_value(TypePrimitive *type) {
    switch (type->kind) {
    case PRIMITIVE_UINT: return U32_MAX;
    case PRIMITIVE_INT:  return S32_MAX;
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
            // assert(lit->as.value.integer >= 0);

            TypePrimitive *prim_type = (TypePrimitive *)(lhs_type);
            unsigned long long lit_value = (unsigned long long) lit->as.value.integer;
            unsigned long long max_value = max_integer_value(prim_type);

            if (lit_value > max_value) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(lit), "Assignment produces an integer overflow. Max value of type %s is %llu", text_bold(type_to_str(lhs_type)), max_value);
                return false;
            }
        } 
    }
    // Expr is complex so we just let it slide. 
    // @Note - When we can evaluate expressions, we should
    // be able to handle checking overflow situations.
    return true;
}

// Returns the concrete type of an untyped type. E.g untyped(int) -> int
Type *solidify_untyped_type(Type *untyped_type) {
    assert(is_untyped_type(untyped_type));

    TypePrimitive *prim = (TypePrimitive *) untyped_type;

    if (prim->kind == PRIMITIVE_UNTYPED_INT) {
        // @Incomplete - We should give it the lowest representable integer type based on the integer value
        return primitive_type(PRIMITIVE_INT);
    }

    return untyped_type;
}

TypeTuple *make_tuple_type_from_expression_list(Typer *typer, DynamicArray exprs) {
    TypeTuple *tuple      = ast_allocate(typer->parser, sizeof(TypeTuple));
    tuple->head.head.kind = AST_TYPE;
    tuple->head.kind      = TYPE_TUPLE;
    tuple->types          = da_init(exprs.count, sizeof(Type *));

    int size = 0;
    for (int i = 0; i < exprs.count; i++) {
        Type *expr_type = ((AstExpr **)exprs.items)[i]->type;

        // Flatten out inner tuple types
        if (expr_type->kind == TYPE_TUPLE) {
            TypeTuple *inner_tuple = (TypeTuple *) expr_type;
            for (int j = 0; j < inner_tuple->types.count; j++) {
                Type *inner_type = ((Type **)inner_tuple->types.items)[j];
                da_append(&tuple->types, inner_type);
            }
        } else {
            da_append(&tuple->types, expr_type);
        }

        size += expr_type->size;
    }
    tuple->head.size = size;

    return tuple;
}

TypeTuple *make_tuple_type_from_type_list(Typer *typer, DynamicArray types) {
    TypeTuple *tuple       = ast_allocate(typer->parser, sizeof(TypeTuple));
    tuple->head.head.kind  = AST_TYPE;
    tuple->head.head.file  = ((Type **)types.items)[0]->head.file;
    tuple->head.head.start = ((Type **)types.items)[0]->head.start;
    tuple->head.head.end   = ((Type **)types.items)[types.count - 1]->head.end;
    tuple->head.kind       = TYPE_TUPLE;
    tuple->types           = da_init(types.count, sizeof(Type *));

    int size = 0;
    for (int i = 0; i < types.count; i++) {
        Type *type = ((Type **)types.items)[i];
        da_append(&tuple->types, type);
        size += type->size;
    }
    tuple->head.size = size;

    return tuple;
}

void reserve_space_for_runtime_any_value(Typer *typer, Type *value_type) {

    // 8 bytes for preserving the destination address across a function call
    reserve_temporary_storage(typer->enclosing_function, 8);

    // Reserve space for copying the value on the stack
    reserve_local_storage(typer->enclosing_function, value_type->size);

    if (value_type->kind == TYPE_STRUCT) {
        reserve_temporary_storage(typer->enclosing_function, 16); // 16 bytes is needed to keep a stable base_offset and cursor for types that needs arrays

        TypeStruct *type_struct = (TypeStruct *) value_type;

        DynamicArray members = type_struct->node->scope->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = ((AstIdentifier **) members.items)[i];
            reserve_space_for_runtime_any_value(typer, member->type);
        }
    }
}

bool check_declaration(Typer *typer, AstDeclaration *decl) {
    if (decl->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return true;
    }

    typer->inside_declaration = decl;

    if (decl->flags & DECLARATION_TYPED) {
        Type *resolved_type = resolve_type(typer, decl->type);
        if (!resolved_type) return false;

        decl->type = resolved_type;
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            ident->type = resolved_type;
        }
        
        for (int i = 0; i < decl->values.count; i++) {
            AstExpr **value_ptr = &((AstExpr **)decl->values.items)[i];
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            AstExpr *value = *value_ptr;

            Type *resolved = check_expression(typer, value, decl->type);
            if (!resolved) return false;

            if (decl->type->kind == TYPE_ANY && value->type->kind != TYPE_ANY) {
                AstCast *any_cast = generate_cast_to_any(typer, value);
                if (!any_cast) return false;
                *value_ptr = (AstExpr *) any_cast;
                value = (AstExpr *) any_cast;
            }

            if (!types_are_equal(decl->type, value->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(value), "Type mismatch. '%s' is said to be of type %s, but expression is of type %s. ", ident->name, type_to_str(decl->type), type_to_str(value->type));
                return false;
            }
        }

    }
    else if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        Type *resolved_type = resolve_type(typer, decl->type);
        if (!resolved_type) return false;

        decl->type = resolved_type;
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            ident->type = resolved_type;
        }
    }
    else if (decl->flags & DECLARATION_INFER) {
        int ident_index = 0;
        for (int i = 0; i < decl->values.count; i++) {
            AstExpr *value = ((AstExpr **)decl->values.items)[i];
            Type *expr_type = check_expression(typer, value, NULL);
            if (!expr_type) return false;

            if (expr_type->kind == TYPE_TUPLE) {
                TypeTuple *tuple_type = (TypeTuple *) expr_type;
                for (int j = 0; j < tuple_type->types.count; j++) {
                    Type *type = ((Type **)tuple_type->types.items)[j];

                    ident_index = i + j;
                    if (ident_index > decl->idents.count - 1) {
                        goto check_identifiers_vs_values;
                    }

                    AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];
                    ident->type = type;
                }

                // Skip the next identifiers as we've already checked them
                ident_index += tuple_type->types.count - 1;
            } else {
                AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];
                ident->type = expr_type;
                ident_index += 1;
            }
        }

        if (decl->values.count > 1) {
            decl->type = (Type *) make_tuple_type_from_expression_list(typer, decl->values);
        } else {
            decl->type = ((AstExpr **)decl->values.items)[0]->type;
        }
    }
    else if (decl->flags & DECLARATION_CONSTANT) {
        for (int i = 0; i < decl->values.count; i++) {
            AstExpr **value_ptr = &((AstExpr **)decl->values.items)[i];
            AstExpr *value = *value_ptr;
            Type *expr_type = check_expression(typer, value, NULL);
            if (!expr_type) return false;

            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            ident->type = expr_type;

            AstExpr *const_expr = simplify_expression(typer->const_evaluator, typer->current_scope, value);
            if (const_expr->head.kind == AST_LITERAL) {
                // Swap out the current expression for the simplified expression
                // @Speed? @Note - We might in the future cleanup the already allocated expression tree as its no longer needed after this swap.
                //                 We might be taking many cache misses if we leave big gaps in the allocated ast nodes, so something like packing the ast nodes could be
                //                 a thing?
                *value_ptr = const_expr;
            } else {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(value), "Constant declaration with a non-constant expression");
                if (value->head.kind == AST_FUNCTION_CALL) {
                    report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(value), "Function calls are not yet allowed to act as a constant expression");
                }
                return false;
            }
        }

        if (decl->values.count > 1) {
            decl->type = (Type *) make_tuple_type_from_expression_list(typer, decl->values);
        } else {
            decl->type = ((AstExpr **)decl->values.items)[0]->type;
        }
    }

    // Check for any void assignments
    if (decl->values.count > 0) {
        for (int i = 0, ident_index = 0; i < decl->values.count; i++, ident_index++) {
            AstExpr *value = ((AstExpr **)decl->values.items)[i];
    
            if (value->type->kind == TYPE_TUPLE) {
                TypeTuple *tuple_type = (TypeTuple *) value->type;
                ident_index += tuple_type->types.count - 1;
                continue;
            }
    
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];
    
            if (ident->type->kind == TYPE_VOID) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(value), "Cannot assign variable to expression of type void", ident->name);
                return false;
            }
        }
    } else {
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];

            if (ident->type->kind == TYPE_VOID) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ident->type), "Variables are not allowed to have type void");
                return false;
            }
        }
    }

    // Solidify any literals
    for (int i = 0, ident_index = 0; i < decl->values.count; i++, ident_index++) {
        AstExpr *value = ((AstExpr **)decl->values.items)[i];

        if (value->type->kind == TYPE_TUPLE) {
            // Value stems from a function call or something else, so nothing to solidify
            TypeTuple *tuple_type = (TypeTuple *) value->type;
            ident_index += tuple_type->types.count - 1;
            continue;
        }

        AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];

        if (is_untyped_type(value->type) && decl->flags & DECLARATION_INFER) {
            Type *solidified_type = solidify_untyped_type(value->type);
            ident->type = solidified_type;
        }

    }

    // Check for value type overflow
    for (int i = 0; i < decl->values.count; i++) {
        AstExpr *value = ((AstExpr **)decl->values.items)[i];
        bool ok = leads_to_integer_overflow(typer, decl->type, value);
        if (!ok) return false;
    }

    // Check for 1-1 match between identifiers and values
check_identifiers_vs_values:
    if (!(decl->flags & DECLARATION_TYPED_NO_EXPR)) {
        int num_values = 0;
        for (int i = 0; i < decl->values.count; i++) {
            AstExpr *value = ((AstExpr **)decl->values.items)[i];
            
            if (value->type->kind == TYPE_TUPLE) {
                TypeTuple *tuple = (TypeTuple *) value->type;
                num_values += tuple->types.count;
            } else {
                num_values += 1;
            }
        }
        if (decl->idents.count > num_values) {
            AstIdentifier *extra_ident = ((AstIdentifier **)decl->idents.items)[decl->idents.count - 1];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(extra_ident), "Assignment mismatch. Have %d %s and %d %s", decl->idents.count, decl->idents.count == 1 ? "identifier" : "identifiers", num_values, num_values == 1 ? "value" : "values");
            return NULL;
        }
        if (decl->idents.count < num_values) {
            AstExpr *extra_expr = ((AstExpr **)decl->values.items)[decl->values.count - 1];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(extra_expr), "Assignment mismatch. Have %d %s and %d %s", decl->idents.count, decl->idents.count == 1 ? "identifier" : "identifiers", num_values, num_values == 1 ? "value" : "values");
            return NULL;
        }
    }

    // Set the value that each identifier corresponds to in the declaration
    for (int i = 0, ident_index = 0; i < decl->values.count; i++, ident_index++) {
        AstExpr *value = ((AstExpr **)decl->values.items)[i];

        if (value->type->kind == TYPE_TUPLE) {
            TypeTuple *tuple_type = (TypeTuple *) value->type;
            for (int j = 0; j < tuple_type->types.count; j++) {
                ident_index = i + j;
                AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];

                ident->value = value;
            }
        }
        else {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[ident_index];
            ident->value = value;
        }
    }

    
    // Mark the identifiers and declaration as fully typechecked
    decl->head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    for (int i = 0; i < decl->idents.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
        ident->flags |= IDENTIFIER_IS_RESOLVED;
        ident->head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    }

    //
    // Do sizing of the declaration
    //
    if (decl->flags & (DECLARATION_IS_FUNCTION_PARAMETER | DECLARATION_IS_STRUCT_MEMBER)) {
        // Omit sizing the declaration as it is done at the call site
        return true;
    }

     if (decl->flags & DECLARATION_CONSTANT) {
        // Constants are not sized
        return true;
     }

    if (typer->enclosing_function != NULL) {
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];
            reserve_local_storage(typer->enclosing_function, ident->type->size);
        }

        return true;
    } else {
        // @TODO: Non-constant declarations in global scope should also be sized. Variables that live in global scope, should probably have some defined stack address of where they live, so they can be referenced locally. Still an open question on how i would do this.
        XXX;
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
        TypePrimitive *lhs_primitive = (TypePrimitive *) lhs;
        TypePrimitive *rhs_primitive = (TypePrimitive *) rhs;

        if (lhs_primitive->kind == rhs_primitive->kind) return true;

        return can_cast_implicitly(rhs, lhs);
    }

    if ((lhs->kind == TYPE_STRUCT && rhs->kind == TYPE_STRUCT) || (lhs->kind == TYPE_ENUM && rhs->kind == TYPE_ENUM)) {
        return strcmp(lhs->name, rhs->name) == 0;
    }

    if (lhs->kind == TYPE_ARRAY && rhs->kind == TYPE_ARRAY) {
        TypeArray *lhs_array = (TypeArray *)lhs;
        TypeArray *rhs_array = (TypeArray *)rhs;

        // Allow dynamic array to "fixed" array cast but not the other way around
        if (!(lhs_array->array_kind == ARRAY_DYNAMIC) && rhs_array->array_kind == ARRAY_DYNAMIC) return false;

        return types_are_equal(lhs_array->elem_type, rhs_array->elem_type);
    } 

    if (lhs->kind == TYPE_POINTER && rhs->kind == TYPE_POINTER) {
        Type *left_pointer_to  = ((TypePointer *)(lhs))->pointer_to;
        Type *right_pointer_to = ((TypePointer *)(rhs))->pointer_to;

        // Allow casting to/from void pointer
        if (right_pointer_to->kind == TYPE_VOID || left_pointer_to->kind == TYPE_VOID) {
            return true;
        }
        return types_are_equal(left_pointer_to, right_pointer_to);
    }

    // Allow spreaded slices or varargs to be passed as variadic parameters
    if (lhs->kind == TYPE_VARIADIC && rhs->kind == TYPE_VARIADIC) {
        // Match on the inner type
        Type *inner_lhs = ((TypeVariadic *) lhs)->type;
        Type *inner_rhs = ((TypeVariadic *) rhs)->type;

        return types_are_equal(inner_lhs, inner_rhs);
    }

    // if (lhs->kind == TYPE_STRING && rhs->kind == TYPE_POINTER) {
    //     Type *right_pointer_to = ((TypePointer *)(rhs))->pointer_to;

    //     if (right_pointer_to->kind == TYPE_INTEGER) {

    //     }
    // }

    if (lhs->kind == TYPE_FUNCTION && rhs->kind == TYPE_FUNCTION) {
        TypeFunction *left_func = (TypeFunction *) lhs;
        TypeFunction *right_func = (TypeFunction *) rhs;

        if (left_func->node->parameters.count != right_func->node->parameters.count) {
            return false;
        }
        if (left_func->node->return_types.count != right_func->node->return_types.count) {
            return false;
        }

        // Compare parameter types
        for (int i = 0; i < left_func->node->parameters.count; i++) {
            AstIdentifier *left_param = ((AstIdentifier **)left_func->node->parameters.items)[i];
            AstIdentifier *right_param = ((AstIdentifier **)right_func->node->parameters.items)[i];

            bool match = types_are_equal(left_param->type, right_param->type);
            if (!match) return false;
        }

        // Compare return types
        for (int i = 0; i < left_func->node->return_types.count; i++) {
            Type *left_return_type = ((Type **)left_func->node->return_types.items)[i];
            Type *right_return_type = ((Type **)right_func->node->return_types.items)[i];

            bool match = types_are_equal(left_return_type, right_return_type);
            if (!match) return false;
        }

        return true;
    }

    if (lhs->kind == TYPE_ANY && rhs->kind == TYPE_ANY) {
        // Allow all types to cast into type any
        return true;
    }

    return false;
}

bool check_assignment(Typer *typer, AstAssignment *assign) {
    Type *lhs_type  = check_expression(typer, assign->lhs, NULL);
    if (!lhs_type) return false;
    Type *expr_type = check_expression(typer, assign->expr, lhs_type);
    if (!expr_type) return false;

    bool is_member     = assign->lhs->head.kind == AST_MEMBER_ACCESS;
    bool is_array_ac   = assign->lhs->head.kind == AST_ARRAY_ACCESS;
    bool is_identifier = assign->lhs->head.kind == AST_LITERAL && ((AstLiteral *)(assign->lhs))->kind == LITERAL_IDENTIFIER;

    AstIdentifier  *member = NULL;
    AstIdentifier  *ident  = NULL;

    bool lhs_is_constant = false;
    if (is_member) {
        member = ((AstMemberAccess *)(assign->lhs))->struct_member;
        lhs_is_constant = member->flags & IDENTIFIER_IS_CONSTANT;
    } 
    else if (is_array_ac) {
        lhs_is_constant = false;
    }
    else if (is_identifier) {
        ident = lookup_from_scope(typer->parser, typer->current_scope, ((AstLiteral *)(assign->lhs))->as.value.identifier.name);
        assert(ident); // Is checked in check_expression so no need to check it here also
        lhs_is_constant = ident->flags & IDENTIFIER_IS_CONSTANT;
    } else {
        XXX;
    }

    if (lhs_is_constant) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign new value to constant '%s'", is_member ? member->name : ident->name);
        return false;
    }

    if (assign->op != '=') {
        if (expr_type->kind != TYPE_INTEGER && expr_type->kind != TYPE_FLOAT) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Compound operators are only allowed for int and float types. Given type was '%s'\n", type_to_str(expr_type));
            return false;
        }
    }

    if (assign->lhs->type->kind == TYPE_ANY && assign->expr->type->kind != TYPE_ANY) {
        // Insert an any cast
        AstCast *any_cast = generate_cast_to_any(typer, assign->expr);
        if (!any_cast) return false;
        assign->expr = (AstExpr *) any_cast;
        expr_type = any_cast->head.type;
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
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign value of type '%s' to member '%s' of type '%s'", type_to_str(expr_type), member->name, type_to_str(lhs_type));
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

    if (assign->expr->head.kind == AST_ARRAY_LITERAL) {
        TypeArray *lhs_array = (TypeArray *) lhs_type;

        if (lhs_array->array_kind == ARRAY_FIXED) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot reassign a fixed size array. Only slices and dynamic arrays may be reassigned");
            // report_error_ast(typer->parser, LABEL_HINT, NULL, "If you really need to reassign the array, make a slice out of the fixed size array");
            return false;
        }
    }

    return true;
}

bool check_struct_defn(Typer *typer, AstStruct *ast_struct) {
    TypeStruct *struct_type = (TypeStruct *)(type_lookup(&typer->parser->type_table, ast_struct->identifier->name));
    assert(struct_type != NULL && struct_type->head.kind == TYPE_STRUCT);


    typer->current_scope = ast_struct->scope;
    DynamicArray members = ast_struct->scope->identifiers;
    bool ok;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        ok = check_declaration(typer, member->decl);
        if (!ok) return false;
    }
    typer->current_scope = ast_struct->scope->parent;
    
    // C-style struct alignment + padding
    int alignment = 0;
    int largest_alignment = 0;
    int offset  = 0;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        
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

    struct_type->head.size  = align_value(offset, largest_alignment);
    struct_type->head.flags |= TYPE_IS_FULLY_SIZED;
    struct_type->alignment  = largest_alignment;

    return true;
}

bool check_for(Typer *typer, AstFor *ast_for) {
    typer->enclosing_for = ast_for;

    // for {...}
    if (ast_for->kind == FOR_INFINITY_AND_BEYOND) {
        // No header to check!
    }
    // for 0..10 {...}
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
        ast_for->iterator->flags |= IDENTIFIER_IS_RESOLVED;

        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_S32);
            ast_for->index->flags |= IDENTIFIER_IS_RESOLVED;
        }

        // Allocate space for the iterator, start, end and optionally for the index
        assert(typer->enclosing_function != NULL);
        reserve_local_storage(typer->enclosing_function, 24);
        if (ast_for->index) {
            reserve_local_storage(typer->enclosing_function, 8);
        }
    }
    // for  x in xs {...}
    // for *x in xs {...}
    else {
        Type *type = check_expression(typer, (AstExpr *)ast_for->iterable, NULL);
        if (!type) return NULL;

        if (type->kind != TYPE_ARRAY && type->kind != TYPE_VARIADIC) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_for->iterable), "Cannot iterate through expression of type %s", type_to_str(type));
            return NULL;
        }

        TypeArray *iterable_type = type->kind == TYPE_ARRAY ? (TypeArray *)type : ((TypeVariadic *)type)->slice;

        Type *iterator_type = iterable_type->elem_type;

        if (ast_for->is_by_pointer) {
            // Wrap the iterator in a pointer type
            TypePointer *ptr = type_alloc(&typer->parser->type_table, sizeof(TypePointer));
            ptr->head.head.kind  = AST_TYPE;
            ptr->head.head.start = ast_for->asterix.start;
            ptr->head.head.end   = ast_for->asterix.end;
            ptr->head.kind       = TYPE_POINTER;
            ptr->head.size       = 8;
            ptr->pointer_to      = iterable_type->elem_type;

            iterator_type = (Type *) ptr;
        }

        ast_for->iterator->type = iterator_type;
        ast_for->iterator->flags |= IDENTIFIER_IS_RESOLVED;

        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_S32);
            ast_for->index->flags |= IDENTIFIER_IS_RESOLVED;
        }

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index
        assert(typer->enclosing_function != NULL);
        reserve_local_storage(typer->enclosing_function, ast_for->iterator->type->size + 24);
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
        if (!(etor->head.flags & AST_FLAG_IS_TYPE_CHECKED)) continue;

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
    case TYPE_FLOAT:   return type->size == 4 ? "%f" : "%lf";
    case TYPE_STRING:  return "%s";
    case TYPE_ENUM:    return "%s";
    case TYPE_POINTER: return "0x%p";
    case TYPE_ARRAY:   return "0x%p";
    case TYPE_ANY:     return "0x%p";
    case TYPE_INTEGER: {
        if (is_unsigned_integer(type)) {
            return type->size == 4 ? "%u" : "%llu";
        } else {
            return type->size == 4 ? "%d" : "%lld";
        }
    }
    case TYPE_STRUCT: {
        StringBuilder builder = sb_init(8);

        TypeStruct *struct_defn = (TypeStruct *) type;

        DynamicArray members = struct_defn->node->scope->identifiers;

        sb_append(&builder, "%s{ ", struct_defn->identifier->name);
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = ((AstIdentifier **)members.items)[i];

            if (member->flags & IDENTIFIER_IS_STRUCT_METHOD) continue;

            char *format_specifier = generate_c_format_specifier_for_type(member->type);

            sb_append(&builder, "%s = %s", member->name, format_specifier);
            
            if (i != members.count - 1) {
                bool next_member_is_method = ((AstIdentifier **)members.items)[i + 1]->flags & IDENTIFIER_IS_STRUCT_METHOD;
                if (!next_member_is_method) {
                    sb_append(&builder, ", ");
                }
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
    DynamicArray members = struct_->scope->identifiers;

    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];

        if (member->flags & IDENTIFIER_IS_STRUCT_METHOD) continue;

        if (member->type->kind == TYPE_STRUCT) {
            AstStruct *nested_struct = ((TypeStruct *)member->type)->node;
            count += count_nested_sizeable_struct_members(typer, nested_struct, 0);
        } else {
            count += 1;
        }
    }

    return count;
}

bool check_print(Typer *typer, AstPrint *print) {
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
        int bytes_args = (num_c_args - 3) * 8;
        reserve_temporary_storage(typer->enclosing_function, bytes_args);
    }

    // :PrintSmallStructHack
    reserve_temporary_storage(typer->enclosing_function, 8);

    char *c_string = generate_c_printf_string(typer, print);
    if (!c_string) return false;

    print->c_string = c_string;

    return true;
}

bool check_expr_stmt(Typer *typer, AstExprStmt *expr_stmt) {
    Type *expr_type = check_expression(typer, expr_stmt->expr, NULL);
    if (expr_type == NULL) {
        return false;
    }
    return true;
}

bool check_return(Typer *typer, AstReturn *ast_return) {
    AstFunctionDefn *ef = typer->enclosing_function;
    if (ef == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_return), "Attempting to return outside of a function");
        return false;
    }

    ast_return->enclosing_function = ef;

    // Cross-match the types evaluated from the return values against the defined return types of the function
    for (int i = 0; i < ast_return->values.count; i++) {
        AstExpr *return_value = ((AstExpr **)ast_return->values.items)[i];
        Type    *wanted_type  = ((Type **)ef->return_types.items)[i];

        Type *ok = check_expression(typer, return_value, wanted_type);
        if (!ok) return false;
    }

    TypeTuple *returning_tuple   = make_tuple_type_from_expression_list(typer, ast_return->values);
    TypeTuple *func_return_types = make_tuple_type_from_type_list(typer, ef->return_types);

    if (returning_tuple->types.count > func_return_types->types.count) {
        AstExpr *last_return_value = ((AstExpr **)ast_return->values.items)[ast_return->values.count - 1];
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_return_value), "Too many return values. Want type %s, got type %s", text_bold(type_to_str((Type *)func_return_types)), text_bold(type_to_str((Type *)returning_tuple)));
        return false;
    }

    if (returning_tuple->types.count < func_return_types->types.count) {
        AstExpr *last_return_value = ((AstExpr **)ast_return->values.items)[ast_return->values.count - 1];
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_return_value), "Not enough return values. Want type %s, got type %s", text_bold(type_to_str((Type *)func_return_types)), text_bold(type_to_str((Type *)returning_tuple)));
        return false;
    }

    for (int i = 0; i < returning_tuple->types.count; i++) {
        Type *given  = ((Type **)returning_tuple->types.items)[i];
        Type *wanted = ((Type **)ef->return_types.items)[i];

        if (!types_are_equal(given, wanted)) {
            // Seek to find the corresponding return value that causes the type mismatch
            int      type_index = -1;
            AstExpr *failing_return_value = NULL;
            for (int j = 0; j < ast_return->values.count; j++) {
                AstExpr *return_value = ((AstExpr **)ast_return->values.items)[j];

                if (return_value->type->kind == TYPE_TUPLE) {
                    TypeTuple *tuple = (TypeTuple *) return_value->type;
                    type_index += tuple->types.count;
                } else {
                    type_index += 1;
                }

                if (type_index == i) {
                    failing_return_value = return_value;
                    break;
                }
            }

            if (failing_return_value == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(given), "Compiler Error: Failed to find the mismatching return value that is causing a type mismatch");
                return false;    
            }

            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(failing_return_value), "Return type mismatch. Wanted type %s, got type %s", text_bold(type_to_str((Type *)func_return_types)), text_bold(type_to_str((Type *)returning_tuple)));
            if (wanted->kind != TYPE_VOID) {
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_return_types), "Here is the return types of function '%s'", ef->identifier->name);
            }
            
            return false;
        }
    }
    
    return true;
}

bool check_assert(Typer *typer, AstAssert *ast_assert) {
    Type *expr_type = check_expression(typer, ast_assert->expr, NULL);
    if (!expr_type) return false;
    if (expr_type->kind != TYPE_BOOL && expr_type->kind != TYPE_POINTER) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_assert), "Expression needs to be of type 'bool' or pointer, but expression evaluated to type '%s'", type_to_str(expr_type));
        return false;
    }

    return true;
}

bool check_if(Typer *typer, AstIf *ast_if) {
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

bool check_statement(Typer *typer, Ast *stmt) {

    // When doing sizing of a function, we sometimes reserve temporary space for array literals
    // and struct literals, but we try and avoid it if we are declaring a new variable. Therefore
    // this "flag" is used to reduce temporary sizing
    typer->inside_declaration = NULL;

    // Each statement might reserve a chunk of memory for temporary values such as function arguments and return values.
    // This call limits the lifetime of the temporary storage to that of a single statement.
    reset_temporary_storage(typer->enclosing_function);

    switch (stmt->kind) {
    case AST_IMPORT:            return check_import(typer, (AstImport *)(stmt));
    case AST_EXTERN:            return check_extern(typer, (AstExtern *)(stmt));
    case AST_BLOCK:             return check_block(typer, (AstBlock *)(stmt));
    case AST_DECLARATION:       return check_declaration(typer, (AstDeclaration *)(stmt));
    case AST_ASSIGNMENT:        return check_assignment(typer, (AstAssignment *)(stmt));
    case AST_FUNCTION_DEFN:     return check_function_defn(typer, (AstFunctionDefn *)(stmt));
    case AST_STRUCT:            return check_struct_defn(typer, (AstStruct *)(stmt));
    case AST_ENUM:              return check_enum_defn(typer, (AstEnum *)stmt);
    case AST_FOR:               return check_for(typer, (AstFor *)(stmt));
    case AST_WHILE:             return check_while(typer, (AstWhile *)(stmt));
    case AST_BREAK_OR_CONTINUE: return check_break_or_continue(typer, (AstBreakOrContinue *)(stmt));
    case AST_EXPR_STMT:         return check_expr_stmt(typer, (AstExprStmt *)(stmt));
    case AST_RETURN:            return check_return(typer, (AstReturn *)(stmt));
    case AST_PRINT:             return check_print(typer, (AstPrint *)(stmt));
    case AST_ASSERT:            return check_assert(typer, (AstAssert *)(stmt));
    case AST_IF:                return check_if(typer, (AstIf *)(stmt));
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

bool check_if_function_needs_a_return(Typer *typer, AstFunctionDefn *func_defn) {
    bool has_return = false;
    For (Ast*, func_defn->body->statements, {
        if (it->kind == AST_RETURN) {
            has_return = true;
            break;
        }
    });

    Type *return_type_0 = ((Type **)func_defn->return_types.items)[0];
    if (!has_return && return_type_0->kind != TYPE_VOID && !func_defn->is_extern && !func_defn->is_lambda) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)func_defn, "Function is missing a return");
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)func_defn, "Put an explicit return at the outer scope of the function. In the future we should be able to detect nested returns");
        return false;
    }

    return true;
}

AstCast *generate_cast_to_any(Typer *typer, AstExpr *expr) {
    if (typer->type_any == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)expr, "Compiler Error: Global type 'Any' was not set during typechecking while doing an any cast");
        return NULL;
    }

    AstCast *any_cast = ast_allocate(typer->parser, sizeof(AstCast));
    any_cast->head.head.kind      = AST_CAST;
    any_cast->head.head.start     = expr->head.start;
    any_cast->head.head.end       = expr->head.end;
    any_cast->head.head.file      = expr->head.file;
    any_cast->head.head.flags     = AST_FLAG_COMPILER_GENERATED | expr->head.flags;
    any_cast->head.type           = (Type *) typer->type_any;
    any_cast->cast_to             = (Type *) typer->type_any;
    any_cast->expr                = expr;

    Type *resolved = check_cast(typer, any_cast, any_cast->cast_to);
    if (!resolved) return NULL;

    return any_cast;
}

Type *check_function_call(Typer *typer, AstFunctionCall *call) {

    if (typer->enclosing_function == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Function calls are not allowed at top-level yet");
        return NULL;
    }

    AstIdentifier *func_ident = NULL;

    if (call->is_method_call) {
        // Lookup the function within the scope of the struct
        assert(call->belongs_to_struct);

        AstIdentifier *func_member = get_struct_member(call->belongs_to_struct, call->identifer->name);
        if (!func_member) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined member function '%s'", text_bold(call->identifer->name));
            return NULL;
        }

        func_ident = func_member;
    } else {
        // Look in the current local scope
        func_ident = lookup_from_scope(typer->parser, typer->current_scope, call->identifer->name);

        if (func_ident == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined function '%s'", text_bold(call->identifer->name));
            return NULL;
        }

    }

    if (func_ident->decl && !(func_ident->decl->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *) func_ident, "Illegal shadowing. Declaration of %s shadows the name of function %s(...)", func_ident->name, func_ident->name);
        report_error_ast(typer->parser, LABEL_HINT, (Ast *) func_ident, "Use a name other than %s to declare the identifier", func_ident->name);
        return NULL;
    }

    if (func_ident->type->kind != TYPE_FUNCTION) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "%s is not the name of a function. Type of %s is %s", func_ident->name, func_ident->name, type_to_str(func_ident->type));
        return NULL;
    }

    AstFunctionDefn *func_defn = ((TypeFunction *)func_ident->type)->node;

    // Make sure that the function definition is fully resolved
    if (!(func_defn->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
        Typer temp = *typer;
        bool ok = check_function_defn(typer, func_defn);
        if (!ok) return NULL;

        typer_restore_state(typer, temp);
    }

    bool call_contains_spreaded_arg = false;
    int spreaded_arg_index     = -1;
    for (int i = 0; i < call->arguments.count; i++) {
        AstExpr *arg   = ((AstExpr **)(call->arguments.items))[i];
        if (arg->head.kind == AST_UNARY && ((AstUnary *)arg)->operator == OP_SPREAD) {
            call_contains_spreaded_arg = true;
            spreaded_arg_index = i;
        }
    }

    //
    // Typecheck the arguments against the parameter list
    //
    if (func_defn->is_variadic) {

        if (call_contains_spreaded_arg) {
            // Arguments vs parameters must match in length 1:1
            // @Cleanup @CopyPasta
            if (call->arguments.count != func_defn->parameters.count) {
                if (call->arguments.count < func_defn->parameters.count) {
                    if (call->arguments.count > 0) {
                        AstExpr *last_argument = ((AstExpr **)call->arguments.items)[call->arguments.count - 1];
                        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_argument), "Too few arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
                    } else {
                        report_error_token(typer->parser, LABEL_ERROR, call->paren_start_token, "Too few arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
                    }
                } else {
                    AstExpr *last_argument = ((AstExpr **)call->arguments.items)[call->arguments.count - 1];
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_argument), "Too many arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
                }
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn->identifier), "Here is the definition of %s", func_defn->identifier->name);
                if (call->is_method_call && call->arguments.count == func_defn->parameters.count + 1) {
                    report_error_ast(typer->parser, LABEL_NOTE, ((Ast **)func_defn->parameters.items)[0], "Methods takes their first parameter as the receiver and should therefore not be passed as an explicit argument");
                }
                return NULL;
            }
        } else {
            // All arguments up untill the varargs must match in length
            if (call->arguments.count < func_defn->variadic_parameter_index) {
                if (call->arguments.count == 0) {
                    report_error_token(typer->parser, LABEL_ERROR, call->paren_start_token, "Too few arguments: Wanted atleast %d, got %d", func_defn->variadic_parameter_index, call->arguments.count);
                } else {
                    AstExpr *last_argument = ((AstExpr **)call->arguments.items)[call->arguments.count - 1];
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_argument), "Too few arguments: Wanted atleast %d, got %d", func_defn->variadic_parameter_index, call->arguments.count);
                }
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn->identifier), "Here is the definition of %s", func_defn->identifier->name);
                return NULL;
            }
        }
        
        // Typecheck the arguments up until the varargs arg begins
        AstIdentifier *var_param = ((AstIdentifier **)func_defn->parameters.items)[func_defn->variadic_parameter_index];
        assert(var_param->type->kind == TYPE_VARIADIC);
        TypeVariadic *vararg_type = (TypeVariadic *) var_param->type;

        for (int i = 0; i < call->arguments.count; i++) {
            Type *wanted_type    = NULL;
            AstIdentifier *param = NULL;
            
            if (i >= func_defn->variadic_parameter_index) {
                param = var_param;
                wanted_type = vararg_type->type;
            } else {
                param = ((AstIdentifier **)(func_defn->parameters.items))[i];
                wanted_type = param->type;
            }

            AstExpr **arg_ptr = &((AstExpr **)(call->arguments.items))[i];
            AstExpr *arg   = ((AstExpr **)(call->arguments.items))[i];
            Type *given_type = check_expression(typer, arg, wanted_type);
            if (!given_type) return NULL;

            // Match a spreaded vararg or slice against their inner type
            if (arg->head.flags & AST_FLAG_EXPR_IS_SPREADED) {
                if (arg->type->kind == TYPE_VARIADIC) {
                    TypeVariadic *variadic = (TypeVariadic *) arg->type;
                    given_type = variadic->type;
                } else if (arg->type->kind == TYPE_ARRAY) {
                    TypeArray *array = (TypeArray *) arg->type;
                    given_type = array->elem_type;
                }
            }

            if (arg->type->kind != TYPE_ANY && wanted_type->kind == TYPE_ANY) {
                if (!func_defn->is_extern) {
                    if (arg->type->kind != TYPE_VARIADIC) {
                        AstCast *any_cast = generate_cast_to_any(typer, arg);
                        if (!any_cast) return NULL;
                        *arg_ptr = (AstExpr *) any_cast;
                        arg      = (AstExpr *) any_cast;
                        given_type = any_cast->cast_to;
                    }
                } else {
                    // Let the given type be coerced into the type any so 
                    given_type = wanted_type;
                }
            }

            if (!types_are_equal(given_type, wanted_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Wanted type %s, got type %s", text_bold(type_to_str(wanted_type)), text_bold(type_to_str(given_type)));
                if (given_type->kind == TYPE_VARIADIC && ((TypeVariadic *)given_type)->type->kind == wanted_type->kind) {
                    report_error_ast(typer->parser, LABEL_HINT, (Ast *)(arg), "Use ... to spread the variadic argument into the parameter");
                }
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(param->decl), "Here is the definition of '%s'", param->name);
                return NULL;
            }
        }

        if (!func_defn->is_extern) {
            if (call_contains_spreaded_arg) {
                // Use the provided slice or variadic as the underlying slice
                AstExpr *spreaded_arg = ((AstExpr **)call->arguments.items)[spreaded_arg_index];

                if (spreaded_arg->type->kind != TYPE_ARRAY && spreaded_arg->type->kind != TYPE_VARIADIC) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)spreaded_arg, "Compiler Error: Expected spreaded argument to be of type slice or variadic");
                    return NULL;
                }

                if (spreaded_arg->type->kind == TYPE_VARIADIC) {
                    TypeArray *slice_type = generate_slice_of(typer->parser, vararg_type->type);    
                    spreaded_arg->type = (Type *)slice_type;
                }

                // Build up the lowered argument list
                call->lowered_arguments = da_init(func_defn->variadic_parameter_index + 1, sizeof(AstExpr *));
    
                // Add the user arguments
                for (int i = 0; i < call->arguments.count; i++) {
                    AstExpr *arg = ((AstExpr **)call->arguments.items)[i];
                    da_append(&call->lowered_arguments, arg);
                }
            } else {
                // Generate a slice argument to hold the variadic arguments
                int num_varargs = call->arguments.count - func_defn->variadic_parameter_index;
    
                TypeArray *slice_type = generate_slice_of(typer->parser, vararg_type->type);
                slice_type->count = num_varargs;
                
                AstArrayLiteral *slice = ast_allocate(typer->parser, sizeof(AstArrayLiteral));
                slice->head.head.kind = AST_ARRAY_LITERAL;
                slice->head.head.flags = AST_FLAG_COMPILER_GENERATED;
                slice->head.type = (Type *)slice_type;
                slice->expressions = da_init(num_varargs, sizeof(AstExpr *));
                
                for (int i = 0; i < call->arguments.count; i++) {
                    if (i < func_defn->variadic_parameter_index) continue;
                    AstExpr *arg = ((AstExpr **)call->arguments.items)[i];

                    if (arg->type->kind != TYPE_ANY && vararg_type->type->kind == TYPE_ANY) {
                        AstCast *any_cast = generate_cast_to_any(typer, arg);
                        if (!any_cast) return NULL;
                        arg = (AstExpr *) any_cast;
                    }

                    da_append(&slice->expressions, arg);
                    
                    // Reserve space for the caller to allocate the each variadic argument
                    reserve_temporary_storage(typer->enclosing_function, arg->type->size);
                }

                // Build up the lowered argument list
                call->lowered_arguments = da_init(func_defn->variadic_parameter_index + 1, sizeof(AstExpr *));
    
                // Add the user arguments
                for (int i = 0; i < call->arguments.count; i++) {
                    if (i == func_defn->variadic_parameter_index) break;
                    AstExpr *arg = ((AstExpr **)call->arguments.items)[i];
                    da_append(&call->lowered_arguments, arg);
                }
    
                // Add the slice
                da_append(&call->lowered_arguments, slice);
            }


        } else {

            // Build up the lowered argument list
            call->lowered_arguments = da_init(func_defn->variadic_parameter_index + 1, sizeof(AstExpr *));

            // Add all the arguments and mark the variadic arguments
            for (int i = 0; i < call->arguments.count; i++) {
                AstExpr *arg = ((AstExpr **)call->arguments.items)[i];
                if (i >= func_defn->variadic_parameter_index) {
                    arg->head.flags |= AST_FLAG_IS_C_VARARG;
                }

                da_append(&call->lowered_arguments, arg);
            }
        }

    } else {
        if (call->arguments.count != func_defn->parameters.count) {
            if (call->arguments.count < func_defn->parameters.count) {
                if (call->arguments.count > 0) {
                    AstExpr *last_argument = ((AstExpr **)call->arguments.items)[call->arguments.count - 1];
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_argument), "Too few arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
                } else {
                    report_error_token(typer->parser, LABEL_ERROR, call->paren_start_token, "Too few arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
                }
            } else {
                AstExpr *last_argument = ((AstExpr **)call->arguments.items)[call->arguments.count - 1];
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(last_argument), "Too many arguments: Wanted %d, got %d", func_defn->parameters.count, call->arguments.count);
            }
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn->identifier), "Here is the definition of %s", func_defn->identifier->name);
            if (call->is_method_call && call->arguments.count == func_defn->parameters.count + 1) {
                report_error_ast(typer->parser, LABEL_NOTE, ((Ast **)func_defn->parameters.items)[0], "Methods takes their first parameter as the receiver and should therefore not be passed as an explicit argument");
            }
            return NULL;
        }

        call->lowered_arguments = da_init(call->arguments.count, sizeof(AstExpr *));

        for (int i = 0; i < call->arguments.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)(func_defn->parameters.items))[i];

            AstExpr **arg_ptr = &((AstExpr **)(call->arguments.items))[i];
            AstExpr *arg   = ((AstExpr **)(call->arguments.items))[i];
            Type *resolved = check_expression(typer, arg, param->type);
            if (!resolved) return NULL;

            if (!func_defn->is_extern && arg->type->kind != TYPE_ANY && param->type->kind == TYPE_ANY) {
                AstCast *any_cast = generate_cast_to_any(typer, arg);
                if (!any_cast) return NULL;
                *arg_ptr = (AstExpr *) any_cast;
                arg      = (AstExpr *) any_cast;
            }

            if (!types_are_equal(arg->type, param->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Wanted type %s, got type %s", text_bold(type_to_str(param->type)), text_bold(type_to_str(arg->type)));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(param->decl), "Here is the definition of '%s'", param->name);
                return NULL;
            }

            da_append(&call->lowered_arguments, arg);
        }
    }
    

    if (func_defn->return_types.count > 1) {
        call->head.type = (Type *) make_tuple_type_from_type_list(typer, func_defn->return_types);
    } else {
        call->head.type = ((Type **)func_defn->return_types.items)[0];
    }
    call->func_defn = func_defn;

    //
    // Allocate space for arguments and potentially for the return value in the calling function
    // @Note - We try and follow the msvc x86 calling convention ???
    // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#parameter-passing 
    //

    if (func_defn->is_variadic) {
        // Reserve space for the created array + slice to it
        int num_varargs = call->arguments.count - func_defn->variadic_parameter_index;
        if (!func_defn->is_extern) {
            // Reserve space for the created array + slice to it
            AstIdentifier *var_param = ((AstIdentifier **)func_defn->parameters.items)[func_defn->variadic_parameter_index];
            TypeVariadic *variadic = (TypeVariadic *) var_param->type;
            reserve_local_storage(typer->enclosing_function, num_varargs * variadic->type->size);
        }
    }

    for (int i = 0; i < call->lowered_arguments.count; i++) {
        AstExpr *arg = ((AstExpr **)call->lowered_arguments.items)[i];
        Type    *arg_type = arg->type;

        // Pass fixed size arrays as slices, so allocate 16 bytes for them
        if (arg_type->kind == TYPE_ARRAY && ((TypeArray *)arg_type)->array_kind == ARRAY_FIXED) {
            reserve_temporary_storage(typer->enclosing_function, 16);
        } else {
            reserve_temporary_storage(typer->enclosing_function, arg_type->size);
        }

        // Reserve for putting arguments into temporary locations
        reserve_temporary_storage(typer->enclosing_function, 8);
    }

    // Reserve space in the calling function for hidden return parameters and for storing them in temporary locations
    for (int i = 0; i < func_defn->return_types.count; i++) {
        Type *return_type = ((Type **)func_defn->return_types.items)[i];
        reserve_temporary_storage(typer->enclosing_function, return_type->size + 8);
    }

    return call->head.type;
}

bool check_function_defn(Typer *typer, AstFunctionDefn *func_defn) {
    if (func_defn->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return true;
    }

    // Check the parameters
    typer->enclosing_function = func_defn;
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];
        bool ok = check_declaration(typer, param->decl);
        if (!ok) return false;

        if (param->type->kind == TYPE_ARRAY && ((TypeArray *)param->type)->array_kind == ARRAY_FIXED) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)param, "Use of fixed size array as parameter. Passing a fixed size array is always done by slice (data + count)");

            TypeArray *array_type = (TypeArray *) param->type;
            array_type->array_kind = ARRAY_SLICE;
            report_error_ast(typer->parser, LABEL_HINT, (Ast *)array_type, "Make the parameter a slice type: %s", type_to_str((Type *)array_type));
            return false;
        }

        if (param->type->kind == TYPE_VARIADIC) {
            func_defn->is_variadic = true;
            func_defn->variadic_parameter_index = i;

            if (i != func_defn->parameters.count - 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *) param, "Variadic parameters must appear as the last parameter");
                return false;
            }
        }
    }

    // Check the return type/s
    for (int i = 0; i < func_defn->return_types.count; i++) {
        Type **return_type_ptr = &((Type **)func_defn->return_types.items)[i];
        Type *resolved_type = resolve_type(typer, *return_type_ptr);
        if (!resolved_type) return false;
        *return_type_ptr = resolved_type;

        if (func_defn->return_types.count > 1 && resolved_type->kind == TYPE_VOID) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)resolved_type, "Function with multiple return values are not allowed to return type void");
            return NULL;
        }

        if (resolved_type->kind == TYPE_ARRAY && ((TypeArray *)resolved_type)->array_kind == ARRAY_FIXED) {
            TypeArray *array_type = (TypeArray *) resolved_type;
            array_type->array_kind = ARRAY_SLICE;
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)resolved_type, "Use of fixed size array as return type. Passing a fixed size array is always done by slice (data + count)");
            report_error_ast(typer->parser, LABEL_HINT, (Ast *)array_type, "Make the return type a slice type: %s", type_to_str((Type *)array_type));
            return false;
        }
    }

    if (func_defn->is_extern) {
        // Limit what C calls can have as a function signature
        if (func_defn->return_types.count > 1) {
            Type *last_return_type = ((Type **)func_defn->return_types.items)[func_defn->return_types.count - 1];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)last_return_type, "Only 1 return type is allowed for external C functions");
            return false;
        }
        if (func_defn->is_method) {
            AstIdentifier *receiver = ((AstIdentifier **)func_defn->parameters.items)[0];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)receiver, "Methods are not allowed for external C functions");
            return false;
        }
    }

    TypeFunction *func_type = ast_allocate(typer->parser, sizeof(TypeFunction));
    func_type->head.head.kind  = AST_TYPE;
    func_type->head.head.start = func_defn->head.start;
    func_type->head.head.end   = func_defn->head.end;
    func_type->head.kind       = TYPE_FUNCTION;
    func_type->node            = func_defn;

    // Do method specific stuff
    if (func_defn->is_method) {
        func_defn->receiver = ((AstIdentifier**)func_defn->parameters.items)[0];
        Type *receiver_type = func_defn->receiver->type;

        // For now we only allow the receiver type to be a struct or a pointer to a struct
        TypeStruct *receiver_struct = NULL;

        if (receiver_type->kind == TYPE_POINTER) {
            TypePointer *pointer_type = (TypePointer *) receiver_type;

            if (pointer_type->pointer_to->kind == TYPE_STRUCT) {
                receiver_struct = (TypeStruct *) pointer_type->pointer_to;
            }
            else {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)receiver_type, "Receiver type must be a struct or a pointer to a struct. Got type %s", text_bold(type_to_str(receiver_type)));
                report_error_ast(typer->parser, LABEL_NOTE, NULL, "Methods on arbitrary types is still unsupported");
                return false;
            }
        }
        else if (receiver_type->kind == TYPE_STRUCT) {
            receiver_struct = (TypeStruct *) receiver_type;
        }
        else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)receiver_type, "Receiver type must be a struct or a pointer to a struct. Got type %s", text_bold(type_to_str(receiver_type)));
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "Methods on arbitrary types is still unsupported");
            return NULL;
        }

        // Add the function as a member of the receiver struct
        AstIdentifier *method_member = generate_identifier(typer->parser, func_defn->identifier->name, (Type *)func_type, IDENTIFIER_IS_STRUCT_MEMBER | IDENTIFIER_IS_STRUCT_METHOD);
        add_member_to_struct_scope(typer->parser, receiver_struct->node, method_member);

        char *symbol_name = malloc(strlen(receiver_struct->identifier->name) + 8 + strlen(func_defn->identifier->name));
        sprintf(symbol_name, "%s.%s", receiver_struct->identifier->name, func_defn->identifier->name);

        func_defn->symbol_name = symbol_name;
    }
    else {
        func_defn->symbol_name = func_defn->identifier->name;
    }

    // Mark the function as fully typed, when we have typechecked the function signature
    // @Note: We mark it typechecked here so that if the function recurses on itself, then the recursive call knows that it shouldn't typecheck the function definition again
    func_defn->head.flags |= AST_FLAG_IS_TYPE_CHECKED;

    // Check the body
    bool ok = check_block(typer, func_defn->body);
    if (!ok) return false;
    typer->enclosing_function = false;

    // Do a narrow scan for a return statement
    ok = check_if_function_needs_a_return(typer, func_defn);
    if (!ok) return false;


    // Build up the lowered representation of the parameter list for the function (to simplify codegen)
    func_defn->lowered_return_params = da_init(func_defn->return_types.count, sizeof(AstIdentifier *));
    func_defn->lowered_params = da_init(func_defn->return_types.count + func_defn->parameters.count, sizeof(AstIdentifier *));

    switch (func_defn->calling_convention) {
    case CALLING_CONV_SAAD:
        // Return types gets to be hidden out parameters in the lowered representation
        for (int i = 0; i < func_defn->return_types.count; i++) {
            Type *return_type = ((Type **)func_defn->return_types.items)[i];
    
            if (return_type->kind == TYPE_VOID) {
                // Skip
                continue;
            }
    
            Type *ptr_return_type = (Type *) generate_pointer_to_type(typer->parser, return_type);
            char *param_name = malloc(16);
            sprintf(param_name, "ret_%d", i);
            AstIdentifier *param = generate_identifier(typer->parser, param_name, ptr_return_type, 0);
            da_append(&func_defn->lowered_params, param);
            da_append(&func_defn->lowered_return_params, param);
        }
    
        for (int i = 0; i < func_defn->parameters.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];

            if (param->type->kind == TYPE_VARIADIC) {
                // Lower a variadic parameter to a slice
                TypeVariadic *variadic = (TypeVariadic *)param->type;
                TypeArray *slice_type = generate_slice_of(typer->parser, variadic->type);

                AstIdentifier *lowered_var_param = generate_identifier(typer->parser, param->name, (Type *) slice_type, 0);

                // In codegen, we want any lookup of the variadic parameter to be the lowered parameter with the slice type, so we
                // have to replace the identifier that is returned by the scope lookup. This can only be done when we know, that we
                // are finished type checking all the nodes
                da_append(&typer->scope_rewrites, func_defn->body);
                da_append(&typer->scope_rewrites, lowered_var_param);

                da_append(&func_defn->lowered_params, lowered_var_param);
                continue;
            }

            da_append(&func_defn->lowered_params, param);
        }
    break;
    case CALLING_CONV_MSVC:
        // Link to the ABI
        // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170
        assert(func_defn->return_types.count <= 1);
        assert(!func_defn->is_method);

        // A large return value (> 8 bytes) needs to be allocated by the caller and the pointer to it is passed as a hidden return parameter
        Type *return_type = ((Type **)func_defn->return_types.items)[0];
        if (return_type->size > 8) {
            Type *ptr_return_type = (Type *) generate_pointer_to_type(typer->parser, return_type);
            AstIdentifier *param = generate_identifier(typer->parser, "ret0", ptr_return_type, 0);
            da_append(&func_defn->lowered_params, param);
            da_append(&func_defn->lowered_return_params, param);
        }

        for (int i = 0; i < func_defn->parameters.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];
            da_append(&func_defn->lowered_params, param);
        }
    break;
    }

    //
    // Do sizing for parameters
    //
    for (int i = 0; i < func_defn->lowered_params.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func_defn->lowered_params.items)[i];
        reserve_local_storage(func_defn, param->type->size);
    }

    return true;
}

Type *check_enum_defn(Typer *typer, AstEnum *ast_enum) {
    TypeEnum *enum_defn = (TypeEnum *)(type_lookup(&typer->parser->type_table, ast_enum->identifier->name));
    assert(enum_defn != NULL);

    if (ast_enum->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return (Type *) enum_defn;
    }

    int min_value = 0;
    int max_value = 0;
    int auto_increment_value = 0;

    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        if (etor->expr) {
            Type *ok = check_expression(typer, etor->expr, (Type *)(enum_defn)); // Type definition of the enum gets to be the ctx type so we can refer to enum members inside the enum with the shorthand syntax .ENUM_MEMBER
            if (!ok) return NULL;

            AstExpr *constexpr = simplify_expression(typer->const_evaluator, typer->current_scope, etor->expr); // @ScopeRefactoring - We still need to give enums their own scope. This probably doesn't work!!!
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

        if (etor->value < min_value) min_value = etor->value;
        if (etor->value > max_value) max_value = etor->value;
        
        AstEnumerator *enumerator_with_same_value = enum_value_is_unique(ast_enum, etor->value);
        if (enumerator_with_same_value) {
            Ast *site = (Ast *) etor->expr;
            if (!site) site = (Ast *) etor;
            report_error_ast(typer->parser, LABEL_ERROR, site, "Enum values must be unique. Both '%s' and '%s' have value %d", etor->name, enumerator_with_same_value->name, etor->value);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enumerator_with_same_value), "Here is the enum member with the same value ...");
            return NULL;
        }

        etor->head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    }

    enum_defn->head.size = enum_defn->backing_type->size;
    enum_defn->min_value = min_value;
    enum_defn->max_value = max_value;

    ast_enum->head.flags |= AST_FLAG_IS_TYPE_CHECKED;

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

    if (!(found->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Enum member used before being declared", enum_name, enum_defn->identifier->name);
        return NULL;
    }

    literal->enum_member = found;

    return lhs_type;
}

Type *check_array_literal(Typer *typer, AstArrayLiteral *array_lit, Type *ctx_type) {
    if (ctx_type && ctx_type->kind == TYPE_ANY) {
        // Strip the context type
        ctx_type = NULL;
    }

    if(ctx_type && ctx_type->kind != TYPE_ARRAY) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Expected expression to have type %s", type_to_str(ctx_type));
        return NULL;
    }

    if (ctx_type && array_lit->expressions.count == 0) {
        // Don't bother type the empty array
        return ctx_type;
    }

    TypeArray *array = (TypeArray *) ctx_type;
    bool infer_type_from_first_element = ctx_type == NULL;

    if (infer_type_from_first_element) {

        if (array_lit->expressions.count == 0) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Couldn't infer type of array literal from the context as it didn't contain any elements");
            return NULL;
        }
        
        Type *first_element_type = check_expression(typer, ((AstExpr **)(array_lit->expressions.items))[0], NULL);
        if (!first_element_type) return NULL;

        if (is_untyped_type(first_element_type)) {
            // Solidify the type so we don't get arrays of untyped integers for example
            first_element_type = solidify_untyped_type(first_element_type);
        }

        TypeStruct *struct_defn = generate_struct_type_with_data_and_count(typer->parser, first_element_type, "Array");

        array = ast_allocate(typer->parser, sizeof(TypeArray));
        array->head.head.kind  = AST_TYPE;
        array->head.head.start = array_lit->head.head.start;
        array->head.head.end   = array_lit->head.head.end;
        array->head.kind       = TYPE_ARRAY;
        array->array_kind      = ARRAY_FIXED;
        array->elem_type       = first_element_type;
        array->struct_defn     = struct_defn;
        array->capacity_expr   = NULL;
        array->capacity        = array_lit->expressions.count;
        array->count           = array_lit->expressions.count;
    } else {

        // We inherit the array type from the context type

        if (array->array_kind == ARRAY_FIXED) {
            if (array_lit->expressions.count > array->capacity) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_lit), "Trying to assign %d elements to array with size %d", array_lit->expressions.count, array->capacity);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(array->capacity_expr), "Here is the specified size of that array");
                return NULL;
            }
        }
    }

    // Typecheck each of the expressions
    for (int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr **expr_ptr = &((AstExpr **)(array_lit->expressions.items))[i];
        AstExpr *expr = ((AstExpr **)(array_lit->expressions.items))[i];

        bool ok = leads_to_integer_overflow(typer, array->elem_type, expr);
        if (!ok) return NULL;

        if (infer_type_from_first_element && i == 0) continue;  // We already type checked this expression

        Type *resolved = check_expression(typer, expr, array->elem_type);
        if (!resolved) return NULL;

        if (array->elem_type->kind == TYPE_ANY && expr->type->kind != TYPE_ANY) {
            // Insert an any cast
            AstCast *any_cast = generate_cast_to_any(typer, expr);
            if (!any_cast) return false;
            *expr_ptr = (AstExpr *) any_cast;
            expr = (AstExpr *) any_cast;
        }

        if (!types_are_equal(array->elem_type, expr->type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(expr), "Expected element to have type %s, but element is of type %s", type_to_str(array->elem_type), type_to_str(expr->type));
            return NULL;
        }
    }

    // Size the array literal
    switch (array->array_kind) {
    case ARRAY_FIXED: {
        // array->count     = array_lit->expressions.count;
        // array->capacity  = array_lit->expressions.count;
        array->head.size = array->capacity * array->elem_type->size;

        // No space is reserved other than the variable itself
        break;
    }
    case ARRAY_SLICE: {
        array->count     = array_lit->expressions.count;
        array->capacity  = 0;
        array->head.size = 16;

        // Reserve space for the underlying array + slice type
        reserve_local_storage(typer->enclosing_function, array->count * array->elem_type->size);
        reserve_temporary_storage(typer->enclosing_function, 16);

        break;
    }
    case ARRAY_DYNAMIC: {
        array->count     = array_lit->expressions.count;
        array->capacity  = round_up_to_nearest_power_of_two(array_lit->expressions.count);
        array->head.size = 24;

        // Reserve space for the dynamic array type + pointer to malloced array
        reserve_temporary_storage(typer->enclosing_function, 24);
        reserve_temporary_storage(typer->enclosing_function, 8);

        break;
    }
    }

    array->node = array_lit;

    return (Type *)(array);
}

Type *check_array_access(Typer *typer, AstArrayAccess *array_ac) {
    Type *type_being_accessed = check_expression(typer, array_ac->accessing, NULL);
    if (!type_being_accessed) return NULL;

    if (type_being_accessed->kind != TYPE_ARRAY && type_being_accessed->kind != TYPE_POINTER) {
        report_error_range(typer->parser, array_ac->accessing->head.start, array_ac->open_bracket.start, "Cannot index into expression of type %s", type_to_str(type_being_accessed));
        return NULL;
    }

    if (type_being_accessed->kind == TYPE_POINTER) {
        // Treat the pointer as an array
        // @Note: We create a new array type from the pointer so they
        // are effectively treated the same
        //
        TypePointer *ptr = (TypePointer *)type_being_accessed;

        TypeStruct *struct_defn = generate_struct_type_with_data_and_count(typer->parser, ptr->pointer_to, "Array");

        TypeArray *array = ast_allocate(typer->parser, sizeof(TypeArray));
        array->head.head.kind  = AST_TYPE;
        array->head.head.start = array_ac->accessing->head.start;
        array->head.head.end   = array_ac->accessing->head.end;
        array->head.kind       = TYPE_ARRAY;
        array->array_kind      = ARRAY_SLICE;
        array->elem_type       = ptr->pointer_to;
        array->struct_defn     = struct_defn;
        array->capacity_expr   = NULL;
        array->capacity        = 0;
        array->count           = 0;

        array_ac->accessing->type = (Type *) array;
        array_ac->head.head.flags |= AST_FLAG_IS_ARRAY_INDEX_INTO_POINTER;

        type_being_accessed = (Type *) array;
    }

    TypeArray *array_defn = (TypeArray *) type_being_accessed;

    Type *index_expr_type = check_expression(typer, array_ac->index_expr, primitive_type(PRIMITIVE_U32));
    if (!index_expr_type) return NULL;

    if (index_expr_type->kind != TYPE_INTEGER && index_expr_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_ac->index_expr), "Array index must be an integer or enum type, got type %s", type_to_str(index_expr_type));
        return NULL;
    }

    // Reserve temporary space for the array literal if its
    // used in a quick array lookup like:  [1, 2, 3][0];
    if (array_ac->accessing->head.kind == AST_ARRAY_LITERAL) {
        reserve_temporary_storage(typer->enclosing_function, array_ac->accessing->type->size);
    }

    return array_defn->elem_type;
}

Type *check_typeof(Typer *typer, AstTypeof *ast_typeof) {
    Type *expr_type = check_expression(typer, ast_typeof->expr, NULL);
    if (!expr_type) return NULL;

    return primitive_type(PRIMITIVE_STRING);
}

Type *check_sizeof(Typer *typer, AstSizeof *ast_sizeof) {
    Type *resolved_type = resolve_type(typer, ast_sizeof->type);
    if (!resolved_type) return NULL;

    ast_sizeof->type = resolved_type;

    return primitive_type(PRIMITIVE_INT);
}

Type *check_new(Typer *typer, AstNew *ast_new, Type *ctx_type) {
    Type *expr_type = check_expression(typer, ast_new->expr, ctx_type);
    if (!expr_type) return NULL;

    if (expr_type->kind == TYPE_STRUCT) {
        TypePointer *ptr_to_struct = generate_pointer_to_type(typer->parser, expr_type);
        return (Type *) ptr_to_struct;
    }

    if (expr_type->kind == TYPE_ARRAY) {
        // Modify the array to be a dynamic array
        TypeArray *array = (TypeArray *)expr_type;
        array->array_kind = ARRAY_DYNAMIC;

        return (Type *) array;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ast_new->expr, "Invalid expression to new. Only arrays and structs can be newed");
    return NULL;
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
    else if (expr->head.kind == AST_TYPEOF)         result = check_typeof(typer, (AstTypeof *)(expr));
    else if (expr->head.kind == AST_SIZEOF)         result = check_sizeof(typer, (AstSizeof *)(expr));
    else if (expr->head.kind == AST_NEW)            result = check_new(typer, (AstNew *)(expr), ctx_type);
    else if (expr->head.kind == AST_RANGE_EXPR)     result = primitive_type(PRIMITIVE_S64); // @Investigate - Shouldn't this be checked for both sides being integers???
    else if (expr->head.kind == AST_SEMICOLON_EXPR) result = primitive_type(PRIMITIVE_VOID);
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'type_expression'. Expression was of type %s", __FILE__, __LINE__, ast_to_str((Ast *)expr));
        exit(1);
    }

    if (result == NULL) {
        return NULL;
    }

    expr->type = result;
    return result;
}

bool rewrite_as_method_call(Typer *typer, AstFunctionCall *func_call, AstMemberAccess *ma) {
    AstExpr       *recv_arg = ma->left;
    AstIdentifier *recv_param = func_call->func_defn->receiver;

    // @Note: In this block, we don't have full type information on the parameters, so we can only check shallowly if the receiver arg and param matches.

    if ((recv_arg->type->kind == TYPE_POINTER && recv_param->type->kind == TYPE_POINTER) || (recv_arg->type->kind != TYPE_POINTER && recv_param->type->kind != TYPE_POINTER)) {
        // Pointer vs Pointer
        // Value   vs Value
        da_insert(&func_call->arguments, ma->left, 0);
        return true;
    }

    if (recv_arg->type->kind == TYPE_POINTER && recv_param->type->kind != TYPE_POINTER) {
        // Insert a dereference to match
        AstUnary *deref        = ast_allocate(typer->parser, sizeof(AstUnary));
        deref->head.head       = recv_arg->head;
        deref->head.head.kind  = AST_UNARY;
        deref->head.head.flags = AST_FLAG_COMPILER_GENERATED;
        deref->operator        = OP_POINTER_DEREFERENCE;
        deref->expr            = recv_arg;

        Type *type = check_unary(typer, deref, recv_param->type);
        if (!type) return false;

        da_insert(&func_call->arguments, deref, 0);
        return true;
    }

    if (recv_arg->type->kind != TYPE_POINTER && recv_param->type->kind == TYPE_POINTER) {
        // Insert an address-of to match
        AstUnary *addr_of        = ast_allocate(typer->parser, sizeof(AstUnary));
        addr_of->head.head       = recv_arg->head;
        addr_of->head.head.kind  = AST_UNARY;
        addr_of->head.head.flags = AST_FLAG_COMPILER_GENERATED;
        addr_of->operator        = OP_ADDRESS_OF;
        addr_of->expr            = recv_arg;

        Type *type = check_unary(typer, addr_of, recv_param->type);
        if (!type) return false;

        da_insert(&func_call->arguments, addr_of, 0);
        return true;
    }

    // Let the check_function_call deal with the actual type mismatch
    return true;
}

Type *check_member_access(Typer *typer, AstMemberAccess *ma) {
    Type *type_lhs = check_expression(typer, ma->left, NULL);
    if (!type_lhs) return NULL;

    //
    // 1'st case : Member access on enum
    // @Cleanup - I think this bit of code can be better put elsewhere in a unary dot as that is what we are checking for here. e.g .ENUM
    //            That way, we wouldn't need to have an access kind for the member
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
    TypeStruct *struct_type = NULL;
    bool valid_lhs = false;

    if (type_lhs->kind == TYPE_POINTER) {
        Type *points_to = ((TypePointer *)(type_lhs))->pointer_to;

        if (points_to->kind == TYPE_STRUCT) {
            struct_type = (TypeStruct *)(points_to);
            valid_lhs = true;
        }

        if (points_to->kind == TYPE_ARRAY) {
            TypeArray *array_defn = (TypeArray *)(points_to);

            struct_type = array_defn->struct_defn;
            valid_lhs = true;
        }

    }
    else if (type_lhs->kind == TYPE_STRUCT) {
        struct_type = (TypeStruct *)(type_lhs);
        valid_lhs = true;
    }
    else if (type_lhs->kind == TYPE_ARRAY) {
        TypeArray *array_defn = (TypeArray *)(type_lhs);
        struct_type = array_defn->struct_defn;
        valid_lhs = true;
    }
    else if (type_lhs->kind == TYPE_VARIADIC) {
        TypeVariadic *variadic = (TypeVariadic *)(type_lhs);
        struct_type = variadic->slice->struct_defn;
        valid_lhs = true;
    }
    else if (type_lhs->kind == TYPE_ANY) {
        TypeAny *any = (TypeAny *)(type_lhs);
        struct_type = any->struct_defn;
        valid_lhs = true;
    }

    if (!valid_lhs) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ma->left), "Cannot access into expression of type %s", type_to_str(type_lhs));
        return NULL;
    }


    AstFunctionCall *func_call = NULL;
    AstIdentifier   *member = NULL;
    char            *member_name = NULL;
    AstExpr         *rhs = ma->right;

    // Get out the member
    if (rhs->head.kind == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
        AstLiteral *ident = (AstLiteral *) rhs;
        member_name = ident->as.value.identifier.name;
        member = get_struct_member(struct_type->node, member_name);

        ma->access_kind   = MEMBER_ACCESS_STRUCT;
        ma->struct_member = member;
    } 
    else if (rhs->head.kind == AST_FUNCTION_CALL) {
        func_call = (AstFunctionCall *) rhs;

        member_name = func_call->identifer->name;
        member = get_struct_member(struct_type->node, member_name);

        if (member) {

            if (member->type->kind != TYPE_FUNCTION) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *) rhs, "Member %s is not a callable function", member->name);
                return NULL;
            }

            AstFunctionDefn *func_defn = ((TypeFunction *)member->type)->node;
            func_call->func_defn = func_defn;

            // @Todo: This assumes that "static" functions on structs are also methods
            func_call->is_method_call = true;
            func_call->belongs_to_struct = struct_type->node;
    
            // Rewrite the method call into a plain function call, with the 0'th
            // argument being the left-hand side of the member access
            // e.g
            // `foo.bar(x)` -> `bar(foo, x)`
            bool ok = rewrite_as_method_call(typer, func_call, ma);
            if (!ok) return NULL;
    
            Type *func_type = check_function_call(typer, func_call);
            if (!func_type) return NULL;
    
            ma->access_kind = MEMBER_ACCESS_METHOD_CALL;
            ma->method_call = func_call;
        }

    } 
    else {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "Invalid member expression");
        return NULL;
    }

    if (!member) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "'%s' is not a member of '%s'", member_name, type_to_str((Type *)struct_type));

        if (type_lhs->kind == TYPE_ARRAY) {
            TypeArray *array = (TypeArray *) type_lhs;
            if (array->array_kind == ARRAY_DYNAMIC) {
                report_error_ast(typer->parser, LABEL_NOTE, NULL, "Dynamic arrays have the members .data, .count and .capacity");
            } else {
                report_error_ast(typer->parser, LABEL_NOTE, NULL, "Slices and fixed size arrays have the members .data and .count");
            }
        }
        else {
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_type), "Here is the definition of %s", type_to_str((Type *)struct_type));
        }

        return NULL;
    }

    //
    // Sizing
    //

    // Reserve 8 bytes for small structure returns to make it easier to do member access calculations on chained function calls
    // if (ma->left->head.kind == AST_FUNCTION_CALL) {
    //     AstFunctionCall *call = (AstFunctionCall *) ma->left;
    //     Type *return_type_0 = ((Type **)call->func_defn->return_types.items)[0];
    //     if (return_type_0->kind == TYPE_STRUCT && return_type_0->size <= 8) {
    //         reserve_temporary_storage(typer->enclosing_function, 8);
    //     }
    // }
    // if (ma->right->head.kind == AST_FUNCTION_CALL) {
    //     AstFunctionCall *call = (AstFunctionCall *) ma->right;
    //     Type *return_type_0 = ((Type **)call->func_defn->return_types.items)[0];
    //     if (return_type_0->kind == TYPE_STRUCT && return_type_0->size <= 8) {
    //         reserve_temporary_storage(typer->enclosing_function, 8);
    //     }
    // }

    if (func_call) {
        return func_call->head.type;
    }

    return member->type;
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

    DynamicArray members = struct_defn->node->scope->identifiers;
    int curr_member_index = 0;
    for (int i = 0; i < literal->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(literal->initializers.items))[i];

        if (init->designator != NULL) {
            AstIdentifier *member = get_struct_member(struct_defn->node, init->designator->name);
            if (member == NULL) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->designator), "'%s' is not a member of '%s'", init->designator->name, struct_defn->identifier->name);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            Type *member_type = member->type;
            Type *resolved  = check_expression(typer, init->value, member_type); // @Note - Passing down the member_type here, makes it possible for sub-struct initialization without explicitly having to type them
            if (!resolved) return NULL;

            if (member_type->kind == TYPE_ANY && init->value->type->kind != TYPE_ANY) {
                AstCast *any_cast = generate_cast_to_any(typer, init->value);
                if (!any_cast) return false;
                init->value = (AstExpr *) any_cast;
            }

            if (!types_are_equal(member_type, init->value->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->name, type_to_str(member_type), type_to_str(init->value->type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            bool ok = leads_to_integer_overflow(typer, member_type, init->value);
            if (!ok) return false;

            init->member = member;
            curr_member_index = member->member_index + 1;
        } else {
            if (curr_member_index > members.count - 1) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init), "Initializing to unknown member of '%s'", type_to_str((Type *)(struct_defn)));
                return NULL;
            }
            
            AstIdentifier *member = ((AstIdentifier **)(members.items))[i];
            Type *member_type = member->type;
            Type *resolved  = check_expression(typer, init->value, member_type);
            if (!resolved) return NULL;

            if (member_type->kind == TYPE_ANY && init->value->type->kind != TYPE_ANY) {
                AstCast *any_cast = generate_cast_to_any(typer, init->value);
                if (!any_cast) return false;
                init->value = (AstExpr *) any_cast;
            }

            if (!types_are_equal(member_type, init->value->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->name, type_to_str(member_type), type_to_str(init->value->type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(member), "Here is the member '%s' in %s", member->name, struct_defn->identifier->name);
                return NULL;
            }

            bool ok = leads_to_integer_overflow(typer, member_type, init->value);
            if (!ok) return false;

            init->member = member;
            curr_member_index += 1;
        }
    }

    // @Cleanup: I think we can remove the commented out code
    // Type *evaluated_struct;
    // if (literal->explicit_type != NULL) {
    //     evaluated_struct = literal->explicit_type;
    // } else {
    //     evaluated_struct = ctx_type;
    // }

    // Reserve space for the struct literal
    reserve_temporary_storage(typer->enclosing_function, struct_defn->head.size);
    // if (!typer->inside_declaration) {
    // }

    return (Type *) struct_defn;
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

    if (strchr("+-*/", binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_FLOAT)   return biggest_type(ti_lhs, ti_rhs);
        if (lhs == TYPE_FLOAT   && rhs == TYPE_INTEGER) return ti_lhs;
        if (lhs == TYPE_INTEGER && rhs == TYPE_FLOAT)   return ti_rhs;
        if ((lhs == TYPE_INTEGER && rhs == TYPE_ENUM) || (lhs == TYPE_ENUM && rhs == TYPE_INTEGER)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)binary, "Enums must be explicitly casted to integers before applying arithmetic on them");
            return NULL;
        }
    }
    
    if (is_bitwise_operator(binary->operator)) {
        if (lhs == TYPE_INTEGER && rhs == TYPE_INTEGER) return biggest_type(ti_lhs, ti_rhs);
        XXX;
    }

    if (strchr("&|", binary->operator)) {
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
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Operator '!' expects expression to be type bool or pointer. Got type %s\n", type_to_str(expr_type));
            return NULL;
        }

        // @Note - If applied to a pointer, we will do a pointer to bool conversion
        return primitive_type(PRIMITIVE_BOOL);
    }

    if (unary->operator == OP_BITWISE_NOT) {
        if (type_kind != TYPE_INTEGER) { // Maybe give a warning when not applying bitwise not to an integer?
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Operator '~' expects expression to be an integer type. Got type %s\n", type_to_str(expr_type));
            return NULL;
        };

        return expr_type;
    } 

    if (unary->operator == OP_UNARY_MINUS) {
        if (type_kind != TYPE_INTEGER && type_kind != TYPE_FLOAT && type_kind != TYPE_ENUM) { // Maybe give a warning when applying unary minus to an enum??? Seems kinda strange
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Type mismatch. Operator '-' is not applicative on expression of type '%s'\n", type_to_str(expr_type));
            return NULL;
        };

        return expr_type;
    }

    if (unary->operator == OP_SPREAD) {
        if (type_kind != TYPE_ARRAY && type_kind != TYPE_VARIADIC) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Operator '...' expects expression to be a slice or a variadic parameter. Got type %s\n", type_to_str(expr_type));
            return NULL;
        }

        unary->head.head.flags |= AST_FLAG_EXPR_IS_SPREADED;
        // if (type_kind == TYPE_ARRAY) {

            // TypeArray *array = (TypeArray *) unary->expr->type;

            // // Make a variadic type out of the element type
            // TypeVariadic *variadic    = ast_allocate(typer->parser, sizeof(TypeVariadic));
            // variadic->head.head.kind  = AST_TYPE;
            // variadic->head.head.file  = array->head.head.file;
            // variadic->head.head.start = unary->expr->head.start;
            // variadic->head.head.end   = unary->expr->head.end;
            // variadic->head.head.flags = AST_FLAG_COMPILER_GENERATED;
            // variadic->head.kind       = TYPE_VARIADIC;
            // variadic->head.size       = 16;
            // variadic->type            = array->elem_type;
            // variadic->slice           = generate_slice_of(typer->parser, array->elem_type);

            // return (Type *) variadic;
        // }

        return expr_type;
    }

    if (unary->operator == OP_ADDRESS_OF) {
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

    if (unary->operator == OP_POINTER_DEREFERENCE) {
        if (expr_type->kind != TYPE_POINTER) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Invalid pointer dereference. Expression has type %s", type_to_str(expr_type));
            return NULL;
        }

        Type *points_to = ((TypePointer *)(expr_type))->pointer_to;
        return points_to;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *) unary, "Compiler Error: Typechecking unknown unary operator");
    return NULL;
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
    if (from->kind == TYPE_POINTER && to->kind == TYPE_STRING) {
        TypePointer *from_ptr = (TypePointer *)from;
        if (from_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)from_ptr->pointer_to)->kind == PRIMITIVE_U8) {
            return true;     // allow string to *u8
        }
    }

    if (from->kind == TYPE_STRING && to->kind == TYPE_POINTER) {
        TypePointer *to_ptr = (TypePointer *)to;
        if (to_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)to_ptr->pointer_to)->kind == PRIMITIVE_U8) {
            return true;     // allow string to *u8
        }
    }

    if (from->kind == TYPE_POINTER && to->kind == TYPE_POINTER) {
        // Allow any pointers to cast to any other pointer
        return true;
    }

    if (from->kind == TYPE_STRUCT && to->kind == TYPE_STRUCT) {
        // Allow C-like downcasts of one struct to another for c-like inheritence
        // @Improvement: We probably want to limit this or have a different mechanism that would be more type-safe.
        //               e.g only allowing a downcast if the target struct is embedding the source struct
        return true;
    }

    if (from->kind == TYPE_ANY || to->kind == TYPE_ANY) {
        // Allow the "any" type to be casted to whatever
        return true;
    }

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
        cast->cast_to = resolve_type(typer, cast->cast_to);
        if (!cast->cast_to) return NULL;
        wanted_type = cast->cast_to;
    }

    Type *expr_type = check_expression(typer, cast->expr, NULL);
    if (!expr_type) return NULL;

    if (!can_cast_explicitly(expr_type, wanted_type)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)cast, "Invalid Cast: Type %s can not be casted to %s", type_to_str(expr_type), type_to_str(wanted_type));
        return NULL;
    }

    if (is_untyped_type(expr_type)) {
        Type *solidified_type = solidify_untyped_type(expr_type);
        cast->expr->type = solidified_type;
    }

    if (expr_type->kind != TYPE_ANY && wanted_type->kind == TYPE_ANY) {
        // Reserve space for boxing the value
        reserve_space_for_runtime_any_value(typer, expr_type);
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

bool is_bitwise_operator(TokenType op) {
    if (op == '&') return true;
    if (op == '^') return true;
    if (op == '|') return true;
    if (op == TOKEN_BITWISE_SHIFT_LEFT)  return true;
    if (op == TOKEN_BITWISE_SHIFT_RIGHT) return true;
    return false;
}

bool is_boolean_operator(TokenType op) {
    if (op == TOKEN_LOGICAL_AND)   return true;
    if (op == TOKEN_LOGICAL_OR)    return true;
    return false;
}

Type *check_literal(Typer *typer, AstLiteral *literal, Type *ctx_type) {
    switch (literal->kind) {
    case LITERAL_INTEGER: {
        if (ctx_type) {

            // Try and coerce the integer literal into the context type

            // Statically cast untyped integer literals into a float literal to avoid run-time casting
            if (ctx_type->kind == TYPE_FLOAT) {
                double float_value = (double) literal->as.value.integer;
    
                literal->kind              = LITERAL_FLOAT;
                literal->as.value.floating = float_value;
                literal->head.type         = ctx_type;

                return ctx_type;
            }

            if (ctx_type->kind == TYPE_ANY) {
                // @Todo: It should probably return the biggest integer type based on the literal value
                return primitive_type(PRIMITIVE_INT);
            }

            if (ctx_type->kind == TYPE_INTEGER) {
                return ctx_type;
            }

            // We couldn't coerce the integer type to the context type. Fallback to an untyped literal
        }

        return primitive_type(PRIMITIVE_UNTYPED_INT);
    }
    case LITERAL_FLOAT:     return (ctx_type && ctx_type->kind == TYPE_FLOAT)   ? ctx_type : primitive_type(PRIMITIVE_FLOAT);
    case LITERAL_STRING:    return (ctx_type && ctx_type->kind == TYPE_STRING)  ? ctx_type : primitive_type(PRIMITIVE_STRING);
    case LITERAL_BOOLEAN:   return (ctx_type && ctx_type->kind == TYPE_BOOL)    ? ctx_type : primitive_type(PRIMITIVE_BOOL);
    case LITERAL_NULL:      return (Type *) type_null_ptr;
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.value.identifier.name;
        AstIdentifier *ident = lookup_from_scope(typer->parser, typer->current_scope, ident_name);
        if (ident == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Undeclared variable '%s'", ident_name);
            return NULL;
        }

        bool needs_ordering_check = true;

        if (needs_ordering_check) {
            bool unresolved = !(ident->flags && IDENTIFIER_IS_RESOLVED);
            if (is_a_before_b((Ast *) literal, (Ast *) ident) || unresolved) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "'%s' is used before its declared", ident->name);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(ident), "'%s' is being declared here", ident->name);
                return NULL;
            }
        }

        if (!ident->type) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Internal Compiler Error: '%s' is unresolved", ident->name);
            return NULL;
        }

        return ident->type;
    }}

    XXX;
}

void reserve_local_storage(AstFunctionDefn *func_defn, int size) {
    func_defn->num_bytes_locals += size;

    // @Todo: We should reserve the same amount of local-storage the same way as the codegenerator 
    // uses the memory, otherwise we either use up too much memory (like we do now) or the temporary 
    // storage is getting misplaced because of untracked padding or small alignments
    func_defn->num_bytes_locals = align_value(func_defn->num_bytes_locals, 8);
}

void reserve_temporary_storage(AstFunctionDefn *func_defn, int size) {
    func_defn->temp_ptr += size;
    
    if (func_defn->temp_ptr > func_defn->num_bytes_temporaries) {
        func_defn->num_bytes_temporaries = func_defn->temp_ptr;
    }
}

void reset_temporary_storage(AstFunctionDefn *func_defn) {
    if (func_defn == NULL) return;

    func_defn->temp_ptr = 0;
}

AstIdentifier *get_struct_member(AstStruct *struct_defn, char *name) {
    return find_identifier_in_scope(struct_defn->scope, name);
}

bool is_untyped_literal(AstExpr *expr) {
    bool is_literal = expr->head.kind == AST_LITERAL;
    return is_literal && is_untyped_type(expr->type);
}

bool is_untyped_type(Type *type) {
    if (type == NULL) return false;

    TypePrimitive *prim = (TypePrimitive *)type;

    if (type->kind == TYPE_INTEGER) {
        return prim->kind == PRIMITIVE_UNTYPED_INT;
    }

    if (type->kind == TYPE_INTEGER) {
        return prim->kind == PRIMITIVE_UNTYPED_INT;
    }

    return false;
}