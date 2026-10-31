#include "const_expr.c"
#include "lib/string_builder.h"

#define DEBUG_THERE_WERE_ERRORS 0

typedef struct Typer {
    Parser *parser;
    ConstEvaluator *const_evaluator;

    AstFile  *current_file;
    AstBlock *current_scope;

    AstFunctionDefn *enclosing_function; // Used by return statements to know which function they belong to. Is always a pointer to the last element on the enclosing function stack
    EnclosingLoop   *enclosing_loop;     // Used by break or continue statements to know where to branch to

    DynamicArray     enclosing_function_stack;

    AstDeclaration  *inside_declaration;  // Set if we are currently type checking a declaration
    AstExtern       *inside_extern_block; // Set if we are currently type checking function definitions inside an extern block

    ////////////////////////////////////////////////////////////////////
    // Non state related fields
    ////////////////////////////////////////////////////////////////////
    TypeAny         *type_any;       // The resolved type of 'any'
    TypeString      *type_string;    // The resolved type of 'string'
    
    DynamicArray     struct_wait_list; // of *AstIdentifier. A list of struct identifiers that are waiting to be sized as they depend on knowing the size of other structs
    
    int next_unique_symbol_id;
    int array_literal_depth;

    int  generic_depth;               // How many generic function instances are currently being checked inside each other
    bool generic_depth_limit_reached; // Set when generic_depth hits GENERIC_DEPTH_LIMIT, so that the error is not followed by a note for every level
    bool checking_callee;             // Set while checking the function expression of a call. Generic functions may only be used as a callee
    int  struct_check_depth;          // How many struct definitions are currently being checked inside each other

    TypeStruct *primitive_method_holders[PRIMITIVE_COUNT]; // Holds the methods declared on each primitive type e.g 'hash :: method (x: int) -> u64'. See method_holder_of()

    bool checking_place; // Set before checking an expression that is used as a place e.g the left side of an assignment, so that xs[i] uses indexRef()
    bool current_place;  // The value of checking_place for the expression being checked right now
    int  next_hidden_local_id;

    bool checking_member_default; // Set while checking the members of a struct and their default values e.g 'age: int = 30;'

    DynamicArray slice_methods; // of *AstFunctionDefn. The generic methods on []T, see register_slice_method()

    DynamicArray anonymous_types; // of *Type. One anonymous struct or enum for each set of members e.g struct { age: int; }, so that the same members are the same type
} Typer;

#define GENERIC_DEPTH_LIMIT 64

bool  check_block(Typer *typer, AstBlock *block);
bool  check_statement(Typer *typer, Ast *stmt);
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
Type *check_enum_values(Typer *typer, AstEnum *ast_enum);
Type *check_function_defn(Typer *typer, AstFunctionDefn *func_defn);

char *type_to_str(Type *type);
bool types_are_equal(Type *lhs, Type *rhs);
bool can_cast_implicitly(Type *from, Type *to);
bool can_cast_explicitly(Type *from, Type *to);
bool is_bitwise_operator(TokenType op);
bool is_comparison_operator(TokenType op);
bool is_boolean_operator(TokenType op);
bool is_untyped_type(Type *type);
bool is_untyped_literal(AstExpr *expr);
bool is_literal_identifier(AstExpr *expr);
bool is_non_constant_identifier_found_inside_current_function(Typer *typer, AstIdentifier *ident);
void statically_cast_literal(AstLiteral *untyped_literal, Type *cast_into);
void reserve_local_storage(AstFunctionDefn *func_defn, int size);
void reserve_temporary_storage(AstFunctionDefn *func_defn, int size);
void reset_temporary_storage(AstFunctionDefn *func_defn);
AstIdentifier *get_struct_member(AstStruct *struct_defn, char *name);
bool check_attribute_value(Typer *typer, AstStructLiteral *value);
bool is_constant_value(AstExpr *expr);
AstIdentifier *union_variant(TypeStruct *union_type, char *name);
AstExpr *union_kind_value(Typer *typer, AstIdentifier *variant, Ast *at);
AstStructLiteral *make_union_literal(Typer *typer, TypeStruct *union_type, AstIdentifier *variant, Ast *at);
AstIdentifier *make_identifier_from_string(Parser *parser, const char *name, Type *type);
bool check_union_switch(Typer *typer, AstSwitch *sw, AstIdentifier *subject, TypeStruct *union_type, bool by_pointer, AstBlock *lowered, AstBlock *scope_before);
AstBinary *make_binary(Typer *typer, TokenType op, AstExpr *left, AstExpr *right, Ast *at);
AstUnary *generate_address_of_value(Typer *typer, AstExpr *value);
Type *check_switch_expression(Typer *typer, AstSwitch *sw, Type *ctx_type);
bool check_switch_arm(Typer *typer, AstSwitchArm *arm);
bool check_switch(Typer *typer, AstSwitch *sw);
bool check_struct_defn(Typer *typer, AstStruct *ast_struct);
TypeStruct *instantiate_generic_struct(Typer *typer, TypeGenericInstance *instance);
Type *copy_type(Typer *typer, Type *type);
Type *resolve_type(Typer *typer, Type *type);
bool is_generic_function(AstFunctionDefn *func_defn);
void append_mangled_name(StringBuilder *sb, char *name);
Type *solidify_untyped_type(Type *untyped_type);
Type *check_generic_function_defn(Typer *typer, AstFunctionDefn *generic);
AstFunctionDefn *instantiate_generic_function_call(Typer *typer, AstFunctionCall *call, AstFunctionDefn *generic);
bool register_method(Typer *typer, AstFunctionDefn *method, TypeStruct *receiver_struct);
TypeStruct *method_holder_of(Typer *typer, Type *type);
PrimitiveKind canonical_primitive_kind(PrimitiveKind kind);
AstIdentifier *find_method(TypeStruct *struct_type, char *name);
AstFunctionCall *generate_method_call(Typer *typer, AstExpr *receiver, char *name, AstExpr **args, int arg_count);
Type *check_collection_index(Typer *typer, AstArrayAccess *array_ac, TypeStruct *collection, bool place);
Type *check_collection_literal(Typer *typer, AstArrayLiteral *lit, TypeStruct *collection);
AstExpr *collection_as_slice(Typer *typer, AstExpr *expr, Type *wanted);
TypeStruct *instantiate_standard_collection(Typer *typer, char *name, Type *arg0, Type *arg1, Ast *site);
AstArgument *generate_argument_from_expr(Typer *typer, AstExpr *expr);
bool struct_is_comparable(TypeStruct *struct_type, AstIdentifier **bad_member);
bool register_slice_method(Typer *typer, AstFunctionDefn *generic);
AstFunctionDefn *find_slice_method(Typer *typer, const char *name);
bool rewrite_slice_method_call(Typer *typer, AstFunctionCall *call);
bool register_generic_method(Typer *typer, AstFunctionDefn *generic);
bool register_methods_early(Typer *typer, AstFile *file);
bool check_struct_statement(Typer *typer, AstStruct *ast_struct);
char *type_identity(Type *type);
AstIdentifier *resolve_qualified_name(Typer *typer, AstExpr *expr, bool *failed);
AstIdentifier *lookup_type_identifier(Typer *typer, char *name, Ast *site);
AstIdentifier *find_namespace_declaration(Namespace *ns, char *name);
bool check_namespace_files(Typer *typer, Namespace *ns);
bool is_declared_by(AstIdentifier *ident, AstFile *file);
bool report_if_not_visible(Typer *typer, AstIdentifier *ident, Ast *site);
void report_if_deprecated(Typer *typer, AstIdentifier *ident, Ast *site);
AstLiteral *make_resolved_identifier_literal(Typer *typer, AstIdentifier *ident, Ast *at);
AstFunctionDefn *readonly_self_method(Typer *typer, AstExpr *expr);
AstFunctionDefn *readonly_self_place(Typer *typer, AstExpr *expr);
void report_readonly_self(Typer *typer, Ast *at, AstFunctionDefn *method, const char *action, AstExpr *place);
void append_place_text(StringBuilder *sb, AstExpr *expr);
bool check_declaration(Typer *typer, AstDeclaration *decl);
bool identifier_is_type(AstIdentifier *ident);
Type *resolve_anonymous_type(Typer *typer, Type *type);
AstExpr *member_default_value(AstIdentifier *member);
bool desugar_key_value_for(Typer *typer, AstFor *ast_for);
bool check_declaration_of(Typer *typer, AstIdentifier *ident);
AstIdentifier *get_struct_method(AstStruct *struct_defn, char *name);

AstCast *generate_cast_to_any(Typer *typer, AstExpr *expr);

void push_enclosing_function(Typer *typer, AstFunctionDefn *func_defn);
void pop_enclosing_function(Typer *typer);

Typer typer_init(Parser *parser, ConstEvaluator *ce) {
    Typer typer = {0};
    typer.parser = parser;
    typer.const_evaluator = ce;

    typer.current_file        = NULL;
    typer.current_scope       = NULL;
    typer.enclosing_function  = NULL;
    typer.enclosing_function_stack = da_init(2, sizeof(AstFunctionDefn *));
    typer.enclosing_loop      = NULL;
    typer.inside_declaration  = NULL;
    typer.struct_wait_list    = da_init(8, sizeof(AstIdentifier *));
    typer.anonymous_types     = da_init(8, sizeof(Type *));
    typer.slice_methods       = da_init(8, sizeof(AstFunctionDefn *));

    return typer;
}

void typer_restore_state(Typer *typer, Typer saved_state) {
    typer->current_file        = saved_state.current_file;
    typer->current_scope       = saved_state.current_scope;
    typer->enclosing_function  = saved_state.enclosing_function;
    typer->enclosing_function_stack = saved_state.enclosing_function_stack;
    typer->enclosing_loop      = saved_state.enclosing_loop;
    typer->array_literal_depth = saved_state.array_literal_depth;

    // @Cleanup @CopyPasta :CopyPasteCurrentFile 
    typer->parser->current_file  = saved_state.current_file;
    typer->parser->current_scope = saved_state.current_scope;
}

// Adds the declarations of the other files of the same namespace to a file's scope. A namespace shares its declarations, also the private ones
bool import_namespace_siblings(Typer *typer, AstFile *file) {
    Parser *parser = typer->parser;
    for (int i = 0; i < file->ns->files.count; i++) {
        AstFile *sibling = ((AstFile **)file->ns->files.items)[i];
        if (sibling == file) continue;

        for (int j = 0; j < sibling->scope->identifiers.count; j++) {
            AstIdentifier *ident = ((AstIdentifier **)sibling->scope->identifiers.items)[j];
            if (!is_declared_by(ident, sibling)) continue; // Only what the sibling declares itself

            AstIdentifier *existing = find_identifier_in_scope(file->scope, ident->name);
            if (existing == ident) continue;
            if (existing && existing->head.file == file) {
                report_error_ast(parser, LABEL_ERROR, (Ast *)existing, "Redeclaration of '%s' in the namespace '%s'", ident->name, file->ns->name);
                report_error_ast(parser, LABEL_NOTE, (Ast *)ident, "Here is the other declaration, in another file of the namespace");
                return false;
            }
            if (existing) continue;
            da_append(&file->scope->identifiers, ident);
        }
    }
    return true;
}

bool check_file(Typer *typer, AstFile *ast_file) {
    // Files can use each other, so a file that is being checked is not checked again
    if (ast_file->check_state != 0) return true;
    ast_file->check_state = 1;

    typer->current_file        = ast_file;
    typer->current_scope       = ast_file->scope;
    typer->enclosing_function  = NULL;
    typer->enclosing_function_stack = da_init(2, sizeof(AstFunctionDefn *));
    typer->enclosing_loop      = NULL;
    typer->inside_declaration  = NULL;
    typer->array_literal_depth = 0;

    // @Cleanup @CopyPasta :CopyPasteCurrentFile 
    typer->parser->current_file  = ast_file;
    typer->parser->current_scope = ast_file->scope;

    if (ast_file->ns) {
        bool ok = import_namespace_siblings(typer, ast_file);
        if (!ok) return false;
    }

    bool ok = register_methods_early(typer, ast_file);
    if (!ok) return false;

    // The other files of the namespace e.g so that their methods are registered
    if (ast_file->ns) {
        Typer saved_state = *typer;
        bool siblings_ok = check_namespace_files(typer, ast_file->ns);
        typer_restore_state(typer, saved_state);
        typer->enclosing_function_stack = saved_state.enclosing_function_stack;
        if (!siblings_ok) return false;
    }

    for (int i = 0; i < ast_file->statements.count; i++) {
        Ast *stmt = ((Ast **)(ast_file->statements.items))[i];
        bool ok = check_statement(typer, stmt);
        if (!ok) {

        #if DEBUG_THERE_WERE_ERRORS
            report_error_ast(typer->parser, LABEL_ERROR, stmt, "Compiler Error: Something went wrong while typechecking this statement");
        #endif
            return false; // @Improvement - Maybe proceed with typechecking if we can still continue after seing the error
        }
    }

    ast_file->check_state = 2;
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

    DynamicArray members = struct_defn->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        printf("    %s: %s;\n", member->name, type_to_str(member->type));
    }
    printf("}\n");
}

bool import_declarations(Parser *parser, AstImport *import, AstFile *to_file) {
    AstFile *from_file = import->imported_file;

    for (int i = 0; i < from_file->scope->identifiers.count; i++) {
        AstIdentifier *src_ident = ((AstIdentifier **)from_file->scope->identifiers.items)[i];

        // Don't include the identifier if it does not originate from the file it self
        if (!is_declared_by(src_ident, from_file)) {
            continue;
        }
        if (src_ident->flags & IDENTIFIER_IS_PRIVATE) {
            continue;
        }

        // A file's own declarations win over the standard collections it imports, so a file can have its own List
        if (import->is_prelude && find_identifier_in_scope(to_file->scope, src_ident->name)) {
            continue;
        }

        AstIdentifier *existing = add_identifier_to_scope(parser, to_file->scope, src_ident);

        if (existing == src_ident) {
            continue;
        }

        if (existing != NULL) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)src_ident, "Import of identifier '%s' conflicts with existing identifier", src_ident->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "Here is the existing identifier ...");
            report_error_ast(parser, LABEL_HINT, (Ast *)import, "Either namespace the import or rename one of the identifiers in the files");
            return false;
        }
    }

    return true;
}

bool check_namespace_files(Typer *typer, Namespace *ns) {
    for (int i = 0; i < ns->files.count; i++) {
        AstFile *file = ((AstFile **)ns->files.items)[i];
        if (file->check_state != 0) continue;

        Typer current_state = *typer;
        bool ok = check_file(typer, file);
        if (!ok) return false;
        typer_restore_state(typer, current_state);
    }
    return true;
}

// Names that a file gets from its imports belong to the file, but are not its declarations e.g the 'm' of 'import math as m;'
bool is_declared_by(AstIdentifier *ident, AstFile *file) {
    if (ident->head.file != file) return false;
    return !(ident->flags & (IDENTIFIER_IS_NAMESPACE | IDENTIFIER_IS_IMPORTED_NAME | IDENTIFIER_IS_AMBIGUOUS));
}

// A public declaration of a namespace (or a private one, for code in the same namespace), or NULL
AstIdentifier *find_namespace_declaration(Namespace *ns, char *name) {
    for (int i = 0; i < ns->files.count; i++) {
        AstFile *file = ((AstFile **)ns->files.items)[i];
        for (int j = 0; j < file->scope->identifiers.count; j++) {
            AstIdentifier *ident = ((AstIdentifier **)file->scope->identifiers.items)[j];
            if (is_declared_by(ident, file) && strcmp(ident->name, name) == 0) return ident;
        }
    }
    return NULL;
}

char *namespace_of(AstIdentifier *ident) {
    if (ident->head.file && ident->head.file->ns) return ident->head.file->ns->name;
    return ident->head.file ? ident->head.file->absolute_path : "?";
}

bool is_prelude_identifier(Typer *typer, AstIdentifier *ident) {
    Parser *parser = typer->parser;
    AstFile *file  = ident->head.file;
    if (file == NULL) return false;
    if (file == parser->collections_file) return true;
    if (parser->runtime_file_path && strcmp(file->absolute_path, parser->runtime_file_path) == 0) return true;
    if (parser->builtin_string_file_path && strcmp(file->absolute_path, parser->builtin_string_file_path) == 0) return true;
    if (parser->slices_file_path && strcmp(file->absolute_path, parser->slices_file_path) == 0) return true;
    return false;
}

void replace_in_scope(AstBlock *scope, AstIdentifier *old_ident, AstIdentifier *new_ident) {
    for (int i = 0; i < scope->identifiers.count; i++) {
        if (((AstIdentifier **)scope->identifiers.items)[i] == old_ident) {
            ((AstIdentifier **)scope->identifiers.items)[i] = new_ident;
            return;
        }
    }
}

// Adds a declaration that 'use' brings into a file. The file's own declarations and explicit imports win over it, and it wins over the prelude.
// Two different declarations with the same name from 'use' are ambiguous, which is an error where the name is used
void add_used_identifier(Typer *typer, AstFile *file, AstIdentifier *ident) {
    AstIdentifier *existing = find_identifier_in_scope(file->scope, ident->name);
    if (existing == NULL) {
        da_append(&file->scope->identifiers, ident);
        return;
    }
    if (existing == ident) return;
    if (existing->head.file == file) return;
    if (existing->flags & (IDENTIFIER_IS_IMPORTED_NAME | IDENTIFIER_IS_NAMESPACE)) return;

    if (is_prelude_identifier(typer, existing)) {
        replace_in_scope(file->scope, existing, ident);
        return;
    }

    if (existing->flags & IDENTIFIER_IS_AMBIGUOUS) {
        for (int i = 0; i < existing->ambiguous.count; i++) {
            if (((AstIdentifier **)existing->ambiguous.items)[i] == ident) return;
        }
        da_append(&existing->ambiguous, ident);
        return;
    }

    AstIdentifier *ambiguous = ast_allocate(typer->parser, sizeof(AstIdentifier));
    ambiguous->head      = existing->head;
    ambiguous->head.file = file;
    ambiguous->name      = existing->name;
    ambiguous->flags     = IDENTIFIER_IS_AMBIGUOUS | IDENTIFIER_IS_RESOLVED;
    ambiguous->scope     = file->scope;
    ambiguous->ambiguous = da_init(2, sizeof(AstIdentifier *));
    da_append(&ambiguous->ambiguous, existing);
    da_append(&ambiguous->ambiguous, ident);
    replace_in_scope(file->scope, existing, ambiguous);
}

// Adds a name that an import gives a file e.g the 'm' of 'import math as m;' or the 'sin' of 'import math.sin;'
bool add_imported_name(Typer *typer, AstFile *file, AstIdentifier *ident, AstImport *import) {
    AstIdentifier *existing = find_identifier_in_scope(file->scope, ident->name);
    if (existing && (existing->head.file == file || (existing->flags & (IDENTIFIER_IS_IMPORTED_NAME | IDENTIFIER_IS_NAMESPACE)))) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)import, "The import of '%s' conflicts with another '%s'", ident->name, ident->name);
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)existing, "Here is the other '%s'", ident->name);
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Give the import another name with 'as' e.g import %s as other;", import->path);
        return false;
    }
    if (existing) {
        replace_in_scope(file->scope, existing, ident);
    } else {
        da_append(&file->scope->identifiers, ident);
    }
    return true;
}

bool check_namespace_import(Typer *typer, AstImport *import) {
    Parser    *parser = typer->parser;
    AstFile   *file   = typer->current_file;
    Namespace *ns     = import->ns;

    Typer current_state = *typer;
    bool ok = check_namespace_files(typer, ns);
    if (!ok) return false;
    typer_restore_state(typer, current_state);

    bool same_namespace = file->ns == ns;

    if (import->kind == IMPORT_USE) {
        for (int i = 0; i < ns->files.count; i++) {
            AstFile *ns_file = ((AstFile **)ns->files.items)[i];
            for (int j = 0; j < ns_file->scope->identifiers.count; j++) {
                AstIdentifier *ident = ((AstIdentifier **)ns_file->scope->identifiers.items)[j];
                if (!is_declared_by(ident, ns_file)) continue;
                if ((ident->flags & IDENTIFIER_IS_PRIVATE) && !same_namespace) continue;
                add_used_identifier(typer, file, ident);
            }
        }
        return true;
    }

    if (import->kind == IMPORT_NAMESPACE) {
        char *last = strrchr(import->path, '.');
        AstIdentifier *ident = ast_allocate(parser, sizeof(AstIdentifier));
        ident->head.kind  = AST_IDENTIFIER;
        ident->head.file  = file;
        ident->head.start = import->head.start;
        ident->head.end   = import->head.end;
        ident->name       = import->alias ? import->alias : (last ? last + 1 : import->path);
        ident->flags      = IDENTIFIER_IS_NAMESPACE | IDENTIFIER_IS_CONSTANT | IDENTIFIER_IS_RESOLVED;
        ident->scope      = file->scope;
        ident->ns         = ns;
        return add_imported_name(typer, file, ident, import);
    }

    // import math.sin; or import math.sin as s;
    AstIdentifier *target = find_namespace_declaration(ns, import->name);
    if (target == NULL) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)import, "'%s' is not declared in the namespace '%s'", import->name, ns->name);
        return false;
    }
    if (!report_if_not_visible(typer, target, (Ast *)import)) return false;

    AstIdentifier *ident = ast_allocate(parser, sizeof(AstIdentifier));
    memcpy(ident, target, sizeof(AstIdentifier));
    ident->name          = import->alias ? import->alias : target->name;
    ident->flags        |= IDENTIFIER_IS_IMPORTED_NAME;
    ident->imported_from = target;
    return add_imported_name(typer, file, ident, import);
}

bool check_import(Typer *typer, AstImport *import) {
    if (import->kind != IMPORT_FILE) {
        // import "fmt"; of a namespace of the standard library
        if (import->path_token.type == TOKEN_LITERAL_STRING) {
            report_error_ast(typer->parser, LABEL_WARNING, (Ast *)import, "import \"%s\"; is deprecated", import->string);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Write 'use %s;' to get all of its names, or 'import %s;' to write %s.name", import->path, import->path, import->path);
        }
        return check_namespace_import(typer, import);
    }

    assert(import->imported_file != NULL);

    // import "fmt"; is deprecated, except for the files the compiler imports itself
    if (!(import->head.flags & AST_FLAG_COMPILER_GENERATED)) {
        AstFile *imported = import->imported_file;
        if (imported->ns) {
            report_error_ast(typer->parser, LABEL_WARNING, (Ast *)import, "import \"%s\"; is deprecated", import->string);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Write 'use %s;' to get all of its names, or 'import %s;' to write %s.name", imported->ns->name, imported->ns->name, imported->ns->name);
        }
    }

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

void dump_scope_recursive(Parser *parser, AstBlock *scope) {
    if (scope == NULL) return;

    printf("---------------------- Scope %d ----------------------\n", scope->scope_number);
    for (int i = 0; i < scope->identifiers.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **) scope->identifiers.items)[i];
        printf("%s: %s\n", ident->name, type_to_str(ident->type));
    }

    dump_scope_recursive(parser, scope->parent);
}

// Deep copies a type that has not been resolved yet. resolve_type() resolves types in place, so every
// instance of a generic struct needs its own copy of the member types written in the generic struct
Type *copy_type(Typer *typer, Type *type) {
    Parser *parser = typer->parser;

    switch (type->kind) {
    case TYPE_NAME: {
        Type *copy = type_alloc(&parser->type_table, sizeof(Type));
        memcpy(copy, type, sizeof(Type));
        return copy;
    }
    case TYPE_GENERIC_INSTANCE: {
        TypeGenericInstance *instance = (TypeGenericInstance *)(type);
        TypeGenericInstance *copy     = type_alloc(&parser->type_table, sizeof(TypeGenericInstance));
        memcpy(copy, instance, sizeof(TypeGenericInstance));

        copy->type_args = da_init(instance->type_args.count, sizeof(Type *));
        for (int i = 0; i < instance->type_args.count; i++) {
            Type *arg = copy_type(typer, ((Type **)instance->type_args.items)[i]);
            if (!arg) return NULL;
            da_append(&copy->type_args, arg);
        }
        return (Type *)(copy);
    }
    case TYPE_STRING: {
        TypeString *copy = ast_allocate(parser, sizeof(TypeString));
        memcpy(copy, type, sizeof(TypeString));
        return (Type *)(copy);
    }
    case TYPE_ANY: {
        TypeAny *copy = ast_allocate(parser, sizeof(TypeAny));
        memcpy(copy, type, sizeof(TypeAny));
        return (Type *)(copy);
    }
    case TYPE_POINTER: {
        TypePointer *copy = type_alloc(&parser->type_table, sizeof(TypePointer));
        memcpy(copy, type, sizeof(TypePointer));

        copy->pointer_to = copy_type(typer, ((TypePointer *)(type))->pointer_to);
        if (!copy->pointer_to) return NULL;
        return (Type *)(copy);
    }
    case TYPE_ARRAY: {
        TypeArray *copy = ast_allocate(parser, sizeof(TypeArray));
        memcpy(copy, type, sizeof(TypeArray));

        copy->elem_type = copy_type(typer, ((TypeArray *)(type))->elem_type);
        if (!copy->elem_type) return NULL;

        copy->struct_defn = generate_struct_for_slice(parser, copy->elem_type);
        return (Type *)(copy);
    }
    case TYPE_FUNCTION: {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(type), "Function types inside generic structs are not yet supported");
        return NULL;
    }
    case TYPE_STRUCT: {
        if (((TypeStruct *)type)->node && ((TypeStruct *)type)->node->is_anonymous) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)(type), "Anonymous structs inside generic structs are not yet supported");
            return NULL;
        }
        return type;
    }
    default:
        // Primitives are never changed when resolved, so they can be shared
        return type;
    }
}

// Returns the struct for an instance of a generic struct e.g List<int>. The first time an instance is seen, a new
// struct is made by copying the members of the generic struct into a scope where the type parameters are bound to the type arguments
TypeStruct *instantiate_generic_struct(Typer *typer, TypeGenericInstance *instance) {
    Parser *parser = typer->parser;

    // The standard collections e.g [..]T is List<T> are looked up in the standard library, even if a file has its own List
    AstBlock *scope = typer->current_scope;
    if (instance->is_standard) {
        if (parser->collections_file == NULL) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)(instance), "Compiler Error: The standard collections are not loaded");
            return NULL;
        }
        scope = parser->collections_file->scope;
    }

    AstIdentifier *ident = NULL;
    if (instance->is_standard) {
        ident = lookup_from_scope(parser, scope, instance->head.name);
    } else {
        ident = lookup_type_identifier(typer, instance->head.name, (Ast *)instance);
        if (ident == NULL && strchr(instance->head.name, '.')) return NULL; // Reported by lookup_type_identifier
    }
    if (ident == NULL) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(instance), "Unknown type '%s'", instance->head.name);
        return NULL;
    }

    if (ident->type == NULL || ident->type->kind != TYPE_STRUCT || ((TypeStruct *)ident->type)->node->type_params.count == 0) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(instance), "'%s' is not a generic struct, so it cannot be given type arguments", instance->head.name);
        return NULL;
    }

    TypeStruct *generic_type = (TypeStruct *)(ident->type);
    AstStruct  *generic      = generic_type->node;

    if (instance->type_args.count != generic->type_params.count) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)(instance), "Generic struct '%s' takes %d type argument%s, but %d %s given", instance->head.name,
            generic->type_params.count, generic->type_params.count == 1 ? "" : "s",
            instance->type_args.count, instance->type_args.count == 1 ? "was" : "were");
        report_error_ast(parser, LABEL_NOTE, (Ast *)(generic->identifier), "Here is the definition of '%s'", instance->head.name);
        return NULL;
    }

    // Resolve the type arguments where they are written. They are resolved into a new list, so that the type arguments of
    // the instance are left untouched and can be resolved again
    DynamicArray type_args = da_init(instance->type_args.count, sizeof(Type *));
    for (int i = 0; i < instance->type_args.count; i++) {
        Type *arg = ((Type **)instance->type_args.items)[i];

        Type *resolved = resolve_type(typer, arg);
        if (!resolved) return NULL;

        if (resolved->kind == TYPE_VOID) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)(arg), "Type argument cannot be void");
            return NULL;
        }

        da_append(&type_args, resolved);
    }

    // The name of the instance e.g "List<int>". Structs are compared by name, so instances with the same name are the same type
    StringBuilder sb = sb_init(32);
    sb_append(&sb, "%s<", instance->head.name);
    for (int i = 0; i < type_args.count; i++) {
        sb_append(&sb, "%s", type_to_str(((Type **)type_args.items)[i]));
        if (i + 1 < type_args.count) {
            sb_append(&sb, ", ");
        }
    }
    sb_append(&sb, ">");
    char *name = sb_to_string(&sb);

    // The key tells instances apart, with the namespaces of the types e.g "List<raylib.Color>"
    StringBuilder key_sb = sb_init(32);
    sb_append(&key_sb, "%s<", generic_type->qualified_name ? generic_type->qualified_name : generic_type->head.name);
    for (int i = 0; i < type_args.count; i++) {
        sb_append(&key_sb, "%s", type_identity(((Type **)type_args.items)[i]));
        if (i + 1 < type_args.count) sb_append(&key_sb, ", ");
    }
    sb_append(&key_sb, ">");
    char *key = sb_to_string(&key_sb);

    for (int i = 0; i < generic->instances.count; i++) {
        TypeStruct *existing = ((TypeStruct **)generic->instances.items)[i];
        if (strcmp(existing->qualified_name, key) == 0) {
            da_free(&type_args);
            return existing;
        }
    }

    // Make the instance
    AstStruct *node      = ast_allocate(parser, sizeof(AstStruct));
    node->head           = generic->head;
    node->head.flags     = 0;
    node->static_members = make_empty_struct_scope(parser, node);
    node->methods        = make_empty_struct_scope(parser, node);
    node->generic_struct = generic;
    node->type_args      = type_args;
    node->is_union       = generic->is_union;

    TypeStruct *struct_type      = ast_allocate(parser, sizeof(TypeStruct));
    struct_type->head.head       = generic_type->head.head;
    struct_type->head.kind       = TYPE_STRUCT;
    struct_type->head.name       = name;
    struct_type->node            = node;
    struct_type->qualified_name  = key;

    AstIdentifier *struct_ident  = ast_allocate(parser, sizeof(AstIdentifier));
    memcpy(struct_ident, generic->identifier, sizeof(AstIdentifier));
    struct_ident->name           = name;
    struct_ident->type           = (Type *)(struct_type);
    struct_type->identifier      = struct_ident;
    node->identifier             = struct_ident;

    // Scope binding the type parameters to the type arguments. Its parent is the scope of the generic struct, so that other names are looked up from where the generic struct is defined
    AstBlock *params_scope    = ast_allocate(parser, sizeof(AstBlock));
    params_scope->head        = generic->members->head;
    params_scope->kind        = BLOCK_DECLARATIVE;
    params_scope->parent      = generic->members->parent;
    params_scope->statements  = da_init(1, sizeof(Ast *));
    params_scope->identifiers = da_init(generic->type_params.count, sizeof(AstIdentifier *));
    params_scope->enclosing_function = generic->members->enclosing_function;

    for (int i = 0; i < generic->type_params.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)generic->type_params.items)[i];

        AstIdentifier *bound = ast_allocate(parser, sizeof(AstIdentifier));
        memcpy(bound, param, sizeof(AstIdentifier));
        bound->type   = ((Type **)type_args.items)[i];
        bound->flags |= IDENTIFIER_IS_CONSTANT | IDENTIFIER_IS_RESOLVED | IDENTIFIER_IS_TYPE_ALIAS;
        da_append(&params_scope->identifiers, bound);
    }

    // Copy the members
    AstBlock *members          = ast_allocate(parser, sizeof(AstBlock));
    members->head              = generic->members->head;
    members->kind              = BLOCK_DECLARATIVE;
    members->parent            = params_scope;
    members->statements        = da_init(1, sizeof(Ast *));
    members->identifiers       = da_init(generic->members->identifiers.count, sizeof(AstIdentifier *));
    members->enclosing_function = generic->members->enclosing_function;
    members->belongs_to_struct = node;
    node->members              = members;

    AstDeclaration *generic_decl = NULL; // Members declared together e.g 'a, b: T' share a declaration
    AstDeclaration *decl         = NULL;
    for (int i = 0; i < generic->members->identifiers.count; i++) {
        AstIdentifier *generic_member = ((AstIdentifier **)generic->members->identifiers.items)[i];

        if (generic_member->decl != generic_decl) {
            generic_decl = generic_member->decl;

            decl = ast_allocate(parser, sizeof(AstDeclaration));
            memcpy(decl, generic_decl, sizeof(AstDeclaration));
            decl->idents = da_init(generic_decl->idents.count, sizeof(AstIdentifier *));
            decl->values = da_init(1, sizeof(AstExpr *));

            // A member like 'count := 0;' gets its type from its default value
            if (generic_decl->type) {
                decl->type = copy_type(typer, generic_decl->type);
                if (!decl->type) return NULL;
            }

            // Checking an expression changes it, so each instance parses the default values again e.g the 0 in 'value: T = 0'
            if (generic_decl->values.count > 0) {
                bool ok = reparse_default_values(parser, generic_decl, members, &decl->values);
                if (!ok) return NULL;
            }
        }

        AstIdentifier *member = ast_allocate(parser, sizeof(AstIdentifier));
        memcpy(member, generic_member, sizeof(AstIdentifier));
        member->decl          = decl;
        member->type          = decl->type;
        member->scope         = members;
        member->member_offset = 0;
        da_append(&decl->idents, member);

        add_identifier_to_scope(parser, members, member);
    }

    // Add the instance before it is checked, so that members referring to the instance itself e.g 'next: *Node<T>' find it
    da_append(&generic->instances, struct_type);

    Typer saved_state = *typer;
    typer->current_file         = generic->head.file;
    typer->parser->current_file = generic->head.file;

    bool ok = check_struct_defn(typer, node);

    typer_restore_state(typer, saved_state);
    typer->inside_declaration = saved_state.inside_declaration;

    if (!ok) {
        report_error_ast(parser, LABEL_NOTE, (Ast *)(instance), "... while instantiating '%s' here", name);
        return NULL;
    }

    return struct_type;
}

bool is_generic_function(AstFunctionDefn *func_defn) {
    return func_defn->type_params.count > 0 && func_defn->generic_function == NULL;
}

// Appends a type name as something that is valid in an asm symbol e.g Map<string, *int> becomes Map$string$Pint.
// The backend makes the symbol unique if two different names end up the same
void append_mangled_name(StringBuilder *sb, char *name) {
    for (char *c = name; *c != '\0'; c++) {
        if (starts_identifier(*c) || is_digit(*c)) sb_append(sb, "%c", *c);
        else if (*c == '*')                        sb_append(sb, "P");
        else if (*c == '[')                        sb_append(sb, "A");
        else if (*c == '<' || *c == ',')           sb_append(sb, "$");
        else if (*c == '.')                        sb_append(sb, "D");
        else if (*c != ' ' && *c != '>' && *c != ']') sb_append(sb, "_");
    }
}

int type_param_index(DynamicArray type_params, char *name) {
    for (int i = 0; i < type_params.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)type_params.items)[i];
        if (strcmp(param->name, name) == 0) return i;
    }
    return -1;
}

// Returns true if the (unresolved) type mentions the type parameter with the given name
bool type_mentions_name(Type *type, char *name) {
    if (type == NULL) return false;

    switch (type->kind) {
    case TYPE_NAME:     return strcmp(type->name, name) == 0;
    case TYPE_POINTER:  return type_mentions_name(((TypePointer *)type)->pointer_to, name);
    case TYPE_ARRAY:    return type_mentions_name(((TypeArray *)type)->elem_type, name);
    case TYPE_VARIADIC: return type_mentions_name(((TypeVariadic *)type)->type, name);
    case TYPE_GENERIC_INSTANCE: {
        TypeGenericInstance *instance = (TypeGenericInstance *)type;
        for (int i = 0; i < instance->type_args.count; i++) {
            if (type_mentions_name(((Type **)instance->type_args.items)[i], name)) return true;
        }
        return false;
    }
    case TYPE_FUNCTION: {
        AstFunctionDefn *func = ((TypeFunction *)type)->node;
        for (int i = 0; i < func->parameters.count; i++) {
            if (type_mentions_name(((AstIdentifier **)func->parameters.items)[i]->type, name)) return true;
        }
        for (int i = 0; i < func->return_types.count; i++) {
            if (type_mentions_name(((Type **)func->return_types.items)[i], name)) return true;
        }
        return false;
    }
    default:
        return false;
    }
}

bool type_mentions_any_type_param(Type *type, DynamicArray type_params) {
    for (int i = 0; i < type_params.count; i++) {
        if (type_mentions_name(type, ((AstIdentifier **)type_params.items)[i]->name)) return true;
    }
    return false;
}

// A generic function is not checked itself, only its instances are. Here we only check what can be checked without knowing the type arguments
Type *check_generic_function_defn(Typer *typer, AstFunctionDefn *generic) {
    Parser *parser = typer->parser;

    if (generic->is_lambda) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)generic, "Generic functions must be declared as constants e.g max :: <T>(a, b: T) -> T {...}");
        return NULL;
    }

    for (int i = 0; i < generic->type_params.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)generic->type_params.items)[i];

        AstIdentifier *existing = lookup_from_scope(parser, generic->body->parent, param->name);
        if (existing && (existing->flags & (IDENTIFIER_IS_NAME_OF_STRUCT | IDENTIFIER_IS_NAME_OF_ENUM))) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)param, "Type parameter '%s' shadows the type '%s'", param->name, existing->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "Here is the declaration of '%s'", existing->name);
            return NULL;
        }

        // There is no syntax to give the type arguments explicitly, so each type parameter has to be inferable from the arguments
        bool used_by_parameter = false;
        for (int j = 0; j < generic->parameters.count; j++) {
            AstIdentifier *func_param = ((AstIdentifier **)generic->parameters.items)[j];
            if (type_mentions_name(func_param->type, param->name)) {
                used_by_parameter = true;
                break;
            }
        }
        if (!used_by_parameter) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)param, "Type parameter '%s' of '%s' is not used by any parameter, so its type can never be inferred", param->name, generic->identifier->name);
            report_error_ast(parser, LABEL_HINT, NULL, "Type parameters are inferred from the arguments of a call, so '%s' must appear in the type of a parameter", param->name);
            return NULL;
        }
    }

    if (generic->is_method) {
        bool ok = register_generic_method(typer, generic);
        if (!ok) return NULL;
    }

    TypeFunction *func_type    = ast_allocate(parser, sizeof(TypeFunction));
    func_type->head.head.kind  = AST_TYPE;
    func_type->head.head.start = generic->head.head.start;
    func_type->head.head.end   = generic->head.head.end;
    func_type->head.kind       = TYPE_FUNCTION;
    func_type->head.size       = 8;
    func_type->node            = generic;

    generic->head.head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    generic->head.type = (Type *) func_type;

    return (Type *) func_type;
}

PrimitiveKind canonical_primitive_kind(PrimitiveKind kind) {
    switch (kind) {
    case PRIMITIVE_UINT:  return PRIMITIVE_U32;
    case PRIMITIVE_INT:   return PRIMITIVE_I32;
    case PRIMITIVE_FLOAT: return PRIMITIVE_F32;
    case PRIMITIVE_CHAR:  return PRIMITIVE_U8;
    default:              return kind;
    }
}

// Strict type equality used when inferring type parameters. Unlike types_are_equal(), nothing converts implicitly, but aliases like int and i32 are the same type
bool types_are_identical(Type *a, Type *b) {
    if (is_primitive_type(a->kind) && is_primitive_type(b->kind)) {
        return canonical_primitive_kind(((TypePrimitive *)a)->kind) == canonical_primitive_kind(((TypePrimitive *)b)->kind);
    }
    if (a->kind != b->kind) return false;

    switch (a->kind) {
    case TYPE_POINTER:
        return types_are_identical(((TypePointer *)a)->pointer_to, ((TypePointer *)b)->pointer_to);
    case TYPE_ARRAY: {
        TypeArray *array_a = (TypeArray *)a;
        TypeArray *array_b = (TypeArray *)b;
        if (array_a->array_kind != array_b->array_kind) return false;
        if (array_a->array_kind == ARRAY_FIXED && array_a->capacity != array_b->capacity) return false;
        return types_are_identical(array_a->elem_type, array_b->elem_type);
    }
    case TYPE_STRING:
    case TYPE_ANY:
        return true;
    default:
        return strcmp(type_identity(a), type_identity(b)) == 0;
    }
}

typedef enum BindingKind {
    BINDING_NONE,
    BINDING_TYPED,
    BINDING_UNTYPED_INT,   // Only bound by untyped integer literals so far
    BINDING_UNTYPED_FLOAT, // Only bound by untyped literals with atleast one float literal so far
} BindingKind;

typedef struct TypeBinding {
    Type       *type;
    BindingKind kind;
    Ast        *source; // The argument that gave the binding its type
} TypeBinding;

bool bind_type_param(Typer *typer, AstFunctionDefn *generic, TypeBinding *binding, AstIdentifier *type_param, Type *type, AstExpr *arg, AstIdentifier *param) {
    BindingKind kind = BINDING_TYPED;
    if (is_untyped_type(type)) {
        kind = BINDING_UNTYPED_INT;
    } else if (arg->head.kind == AST_LITERAL && ((AstLiteral *)arg)->kind == LITERAL_FLOAT && arg->type == type) {
        kind = BINDING_UNTYPED_FLOAT;
    }

    if (binding->kind == BINDING_NONE) {
        binding->type   = type;
        binding->kind   = kind;
        binding->source = (Ast *)arg;
        return true;
    }

    if (binding->kind == BINDING_TYPED && kind == BINDING_TYPED) {
        if (!types_are_identical(binding->type, type)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "Conflicting types for type parameter '%s' of '%s'. This argument to '%s' is of type %s, but an earlier argument made '%s' of type %s",
                type_param->name, generic->identifier->name, param->name, text_bold(type_to_str(type)), type_param->name, text_bold(type_to_str(binding->type)));
            report_error_ast(typer->parser, LABEL_NOTE, binding->source, "Here '%s' got the type %s", type_param->name, type_to_str(binding->type));
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Types are not widened when inferring type parameters. Cast the arguments to the same type e.g x as %s", type_to_str(binding->type));
            return false;
        }
        return true;
    }

    // A typed argument decides the type over untyped literals. Between untyped literals a float wins over an integer e.g max(1, 2.5)
    if (kind == BINDING_TYPED || (binding->kind == BINDING_UNTYPED_INT && kind == BINDING_UNTYPED_FLOAT)) {
        binding->type   = type;
        binding->kind   = kind;
        binding->source = (Ast *)arg;
    }
    return true;
}

// Infers the type parameters of a generic function by matching the type of a parameter as written e.g *Map<K, V>, against the type of an argument e.g *Map<int, string>
bool unify_types(Typer *typer, AstFunctionDefn *generic, TypeBinding *bindings, Type *param_type, Type *arg_type, AstExpr *arg, AstIdentifier *param) {
    switch (param_type->kind) {
    case TYPE_NAME: {
        int index = type_param_index(generic->type_params, param_type->name);
        if (index == -1) return true; // Not a type parameter

        AstIdentifier *type_param = ((AstIdentifier **)generic->type_params.items)[index];

        // A fixed array is passed as a slice, so a parameter 'value: T' given a [3]int has T = []int
        if (param && param->type == param_type && arg_type->kind == TYPE_ARRAY && ((TypeArray *)arg_type)->array_kind == ARRAY_FIXED) {
            arg_type = (Type *)generate_slice_of(typer->parser, ((TypeArray *)arg_type)->elem_type);
        }
        return bind_type_param(typer, generic, &bindings[index], type_param, arg_type, arg, param);
    }
    case TYPE_POINTER: {
        if (arg_type->kind != TYPE_POINTER) return true; // The mismatch is reported when checking the arguments of the instance
        Type *pointer_to = ((TypePointer *)arg_type)->pointer_to;
        if (pointer_to->kind == TYPE_VOID) return true;  // e.g null
        return unify_types(typer, generic, bindings, ((TypePointer *)param_type)->pointer_to, pointer_to, arg, param);
    }
    case TYPE_ARRAY: {
        if (arg_type->kind != TYPE_ARRAY) return true;
        return unify_types(typer, generic, bindings, ((TypeArray *)param_type)->elem_type, ((TypeArray *)arg_type)->elem_type, arg, param);
    }
    case TYPE_VARIADIC: {
        return unify_types(typer, generic, bindings, ((TypeVariadic *)param_type)->type, arg_type, arg, param);
    }
    case TYPE_GENERIC_INSTANCE: {
        if (arg_type->kind != TYPE_STRUCT) return true;

        AstStruct *instance = ((TypeStruct *)arg_type)->node;
        if (instance->generic_struct == NULL || strcmp(instance->generic_struct->identifier->name, param_type->name) != 0) return true;

        TypeGenericInstance *param_instance = (TypeGenericInstance *)param_type;
        if (param_instance->type_args.count != instance->type_args.count) return true;

        for (int i = 0; i < param_instance->type_args.count; i++) {
            Type *param_arg = ((Type **)param_instance->type_args.items)[i];
            Type *given_arg = ((Type **)instance->type_args.items)[i];
            bool ok = unify_types(typer, generic, bindings, param_arg, given_arg, arg, param);
            if (!ok) return false;
        }
        return true;
    }
    default:
        return true;
    }
}

// Returns true if values of the struct can be compared member by member with ==. Otherwise 'bad_member' is set to the member that cannot be compared
bool struct_is_comparable(TypeStruct *struct_type, AstIdentifier **bad_member) {
    DynamicArray members = struct_type->node->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        if (member->flags & IDENTIFIER_IS_CONSTANT) continue;

        switch (member->type->kind) {
        case TYPE_BOOL:
        case TYPE_INTEGER:
        case TYPE_FLOAT:
        case TYPE_ENUM:
        case TYPE_POINTER:
        case TYPE_STRING:
            break;
        case TYPE_STRUCT: {
            AstIdentifier *inner = NULL;
            if (!struct_is_comparable((TypeStruct *)member->type, &inner)) {
                *bad_member = member;
                return false;
            }
            break;
        }
        default:
            *bad_member = member;
            return false;
        }
    }
    return true;
}

// Returns the struct that holds the methods of a type. Primitive types and enums don't have members, so they get a struct without
// members just to hold their methods e.g 'hash :: method (x: int) -> u64'. Returns NULL for types that cannot have methods
TypeStruct *method_holder_of(Typer *typer, Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT: return (TypeStruct *)type;
    case TYPE_STRING: {
        if (typer->type_string == NULL) {
            if (!resolve_type(typer, type)) return NULL;
        }
        return typer->type_string ? typer->type_string->struct_defn : ((TypeString *)type)->struct_defn;
    }
    case TYPE_BOOL:
    case TYPE_INTEGER:
    case TYPE_FLOAT: {
        PrimitiveKind kind = canonical_primitive_kind(((TypePrimitive *)type)->kind);
        if (kind == PRIMITIVE_UNTYPED_INT || kind == PRIMITIVE_UNTYPED_FLOAT || kind == PRIMITIVE_VOID) return NULL;

        if (typer->primitive_method_holders[kind] == NULL) {
            typer->primitive_method_holders[kind] = generate_empty_struct_type(typer->parser, primitive_types[kind].name);
        }
        return typer->primitive_method_holders[kind];
    }
    case TYPE_ENUM: {
        TypeEnum *enum_type = (TypeEnum *)type;
        if (enum_type->method_holder == NULL) {
            enum_type->method_holder = generate_empty_struct_type(typer->parser, enum_type->identifier->name);
        }
        return enum_type->method_holder;
    }
    default:
        return NULL;
    }
}

// Adds a method to the methods of its receiver struct, so that it can be called as x.method(...)
bool register_method(Typer *typer, AstFunctionDefn *method, TypeStruct *receiver_struct) {
    if (method->is_registered_method) return true;

    Parser        *parser = typer->parser;
    AstIdentifier *ident  = method->identifier;

    AstIdentifier *member = get_struct_member(receiver_struct->node, ident->name);
    if (member) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)ident, "Method '%s' has the same name as a member of '%s'", ident->name, receiver_struct->head.name);
        report_error_ast(parser, LABEL_NOTE, (Ast *)member, "Here is the member '%s'", member->name);
        return false;
    }

    ident->flags |= IDENTIFIER_IS_STRUCT_MEMBER | IDENTIFIER_IS_STRUCT_METHOD;
    AstIdentifier *existing = add_identifier_to_scope(parser, receiver_struct->node->methods, ident);
    if (existing != NULL) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)ident, "Redeclaration of method '%s' on '%s'", ident->name, receiver_struct->head.name);
        report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "Here is the previous declaration");
        return false;
    }

    method->receiver             = ((AstIdentifier **)method->parameters.items)[0];
    method->receiver_struct      = receiver_struct;
    method->is_registered_method = true;
    return true;
}

// A generic method on a slice e.g 'sort :: method <T>(xs: []T)'. Slices, fixed arrays and collections (structs with elements()) can
// call it as xs.sort(), which is the call sort(xs) of the generic function. See slice_method_call()
bool register_slice_method(Typer *typer, AstFunctionDefn *generic) {
    Parser        *parser   = typer->parser;
    AstIdentifier *receiver = ((AstIdentifier **)generic->parameters.items)[0];
    Type          *elem     = ((TypeArray *)receiver->type)->elem_type;

    int index = elem->kind == TYPE_NAME ? type_param_index(generic->type_params, elem->name) : -1;
    if (index == -1 || generic->type_params.count != 1) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)receiver->type, "The receiver of a method on slices must be []T, where T is the only type parameter of the method");
        report_error_ast(parser, LABEL_HINT, NULL, "e.g sort :: method <T>(xs: []T)");
        return false;
    }

    for (int i = 0; i < typer->slice_methods.count; i++) {
        AstFunctionDefn *existing = ((AstFunctionDefn **)typer->slice_methods.items)[i];
        if (strcmp(existing->identifier->name, generic->identifier->name) == 0) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)generic->identifier, "Redeclaration of method '%s' on slices", generic->identifier->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing->identifier, "Here is the previous declaration");
            return false;
        }
    }

    generic->is_slice_method      = true;
    generic->is_registered_method = true;
    da_append(&typer->slice_methods, generic);
    return true;
}

AstFunctionDefn *find_slice_method(Typer *typer, const char *name) {
    for (int i = 0; i < typer->slice_methods.count; i++) {
        AstFunctionDefn *method = ((AstFunctionDefn **)typer->slice_methods.items)[i];
        if (strcmp(method->identifier->name, name) == 0) return method;
    }
    return NULL;
}

// A generic method e.g 'set :: method <K, V>(m: *Map<K, V>, key: K, value: V)' is added to the methods of the generic struct.
// Its receiver must be the generic struct with exactly the type parameters of the method as type arguments
bool register_generic_method(Typer *typer, AstFunctionDefn *generic) {
    if (generic->is_registered_method) return true;

    Parser        *parser   = typer->parser;
    AstIdentifier *receiver = ((AstIdentifier **)generic->parameters.items)[0];

    Type *receiver_type = receiver->type;
    if (receiver_type && receiver_type->kind == TYPE_POINTER) {
        receiver_type = ((TypePointer *)receiver_type)->pointer_to;
    }

    // A method on []T, e.g 'sort :: method <T>(xs: []T)'
    if (receiver_type && receiver_type == receiver->type && receiver_type->kind == TYPE_ARRAY && ((TypeArray *)receiver_type)->array_kind == ARRAY_SLICE) {
        return register_slice_method(typer, generic);
    }

    if (receiver_type == NULL || receiver_type->kind != TYPE_GENERIC_INSTANCE) {
        report_error_ast(parser, LABEL_ERROR, receiver_type ? (Ast *)receiver_type : (Ast *)receiver, "The receiver of a generic method must be a slice []T or a generic struct, with the type parameters of the method as its type arguments");
        report_error_ast(parser, LABEL_HINT, NULL, "e.g set :: method <K, V>(m: *Map<K, V>, key: K, value: V)");
        return false;
    }

    TypeGenericInstance *instance = (TypeGenericInstance *)receiver_type;

    AstIdentifier *struct_ident = lookup_from_scope(parser, generic->body->parent, instance->head.name);
    if (struct_ident == NULL) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)instance, "Unknown type '%s'", instance->head.name);
        return false;
    }
    if (struct_ident->type == NULL || struct_ident->type->kind != TYPE_STRUCT || ((TypeStruct *)struct_ident->type)->node->type_params.count == 0) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)instance, "'%s' is not a generic struct, so it cannot be given type arguments", instance->head.name);
        return false;
    }

    TypeStruct *generic_struct = (TypeStruct *)struct_ident->type;
    if (instance->type_args.count != generic_struct->node->type_params.count) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)instance, "Generic struct '%s' takes %d type argument%s, but %d %s given", instance->head.name,
            generic_struct->node->type_params.count, generic_struct->node->type_params.count == 1 ? "" : "s",
            instance->type_args.count, instance->type_args.count == 1 ? "was" : "were");
        return false;
    }

    // Each type argument of the receiver is a different type parameter of the method, as methods apply to every instance of the struct
    bool *used = calloc(generic->type_params.count, sizeof(bool));
    for (int i = 0; i < instance->type_args.count; i++) {
        Type *arg = ((Type **)instance->type_args.items)[i];

        int index = arg->kind == TYPE_NAME ? type_param_index(generic->type_params, arg->name) : -1;
        if (index == -1) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)arg, "Type argument '%s' of the receiver must be one of the type parameters of the method '%s'", type_to_str(arg), generic->identifier->name);
            report_error_ast(parser, LABEL_HINT, NULL, "A method applies to every instance of '%s', so it cannot be specialized for some type arguments (no partial specialization)", instance->head.name);
            return false;
        }
        if (used[index]) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)arg, "Type parameter '%s' is used more than once as a type argument of the receiver", arg->name);
            report_error_ast(parser, LABEL_HINT, NULL, "A method applies to every instance of '%s', so each type argument of the receiver must be a different type parameter", instance->head.name);
            return false;
        }
        used[index] = true;
    }
    for (int i = 0; i < generic->type_params.count; i++) {
        if (!used[i]) {
            AstIdentifier *param = ((AstIdentifier **)generic->type_params.items)[i];
            report_error_ast(parser, LABEL_ERROR, (Ast *)param, "Type parameter '%s' of method '%s' is not a type argument of the receiver", param->name, generic->identifier->name);
            report_error_ast(parser, LABEL_NOTE, NULL, "Methods with type parameters of their own are not supported yet");
            return false;
        }
    }
    free(used);

    return register_method(typer, generic, generic_struct);
}

// Adds the methods declared at the top-level of a file to their receiver structs before anything is checked, so that methods can
// be called before they are declared. Methods on other receivers than a struct name, like *List<int>, are added when they are checked
bool register_method_early(Typer *typer, AstFile *file, AstDeclaration *decl) {
    for (int j = 0; j < decl->values.count; j++) {
        AstExpr *value = ((AstExpr **)decl->values.items)[j];
        if (value->head.kind != AST_FUNCTION_DEFN) continue;

        AstFunctionDefn *method = (AstFunctionDefn *)value;
        if (!method->is_method || method->is_extern || method->parameters.count == 0) continue;

        if (is_generic_function(method)) {
            bool ok = register_generic_method(typer, method);
            if (!ok) return false;
            continue;
        }

        AstIdentifier *receiver = ((AstIdentifier **)method->parameters.items)[0];
        Type *receiver_type = receiver->type;
        if (receiver_type && receiver_type->kind == TYPE_POINTER) {
            receiver_type = ((TypePointer *)receiver_type)->pointer_to;
        }
        if (receiver_type == NULL) continue;

        // Methods on primitive types e.g 'hash :: method (x: int) -> u64'
        if (receiver_type->kind == TYPE_BOOL || receiver_type->kind == TYPE_INTEGER || receiver_type->kind == TYPE_FLOAT) {
            TypeStruct *holder = method_holder_of(typer, receiver_type);
            if (holder == NULL) continue;

            bool ok = register_method(typer, method, holder);
            if (!ok) return false;
            continue;
        }

        if (receiver_type->kind != TYPE_NAME) continue;

        AstIdentifier *type_ident = lookup_from_scope(typer->parser, file->scope, receiver_type->name);
        if (type_ident == NULL || type_ident->type == NULL) continue;

        TypeStruct *holder = NULL;
        if ((type_ident->flags & IDENTIFIER_IS_NAME_OF_STRUCT) && type_ident->type->kind == TYPE_STRUCT) {
            if (((TypeStruct *)type_ident->type)->node->type_params.count > 0) continue; // Reported as missing type arguments when checked
            holder = (TypeStruct *)type_ident->type;
        }
        else if ((type_ident->flags & IDENTIFIER_IS_NAME_OF_ENUM) && type_ident->type->kind == TYPE_ENUM) {
            holder = method_holder_of(typer, type_ident->type);
        }
        if (holder == NULL) continue;

        bool ok = register_method(typer, method, holder);
        if (!ok) return false;
    }
    return true;
}

bool register_methods_early(Typer *typer, AstFile *file) {
    for (int i = 0; i < file->statements.count; i++) {
        Ast *stmt = ((Ast **)file->statements.items)[i];

        if (stmt->kind == AST_STRUCT) {
            AstStruct *ast_struct = (AstStruct *)stmt;
            for (int j = 0; j < ast_struct->inside_methods.count; j++) {
                bool ok = register_method_early(typer, file, ((AstDeclaration **)ast_struct->inside_methods.items)[j]);
                if (!ok) return false;
            }
            continue;
        }

        if (stmt->kind != AST_DECLARATION || !(stmt->flags & AST_FLAG_DECL_IS_CONSTANT_FUNCTION_DEFN)) continue;

        bool ok = register_method_early(typer, file, (AstDeclaration *)stmt);
        if (!ok) return false;
    }
    return true;
}

// The methods declared inside a struct are checked as if they were declared right after it
bool check_struct_statement(Typer *typer, AstStruct *ast_struct) {
    bool ok = check_struct_defn(typer, ast_struct);
    if (!ok) return false;

    AstBlock *scope = typer->current_scope;
    for (int i = 0; i < ast_struct->inside_methods.count; i++) {
        AstDeclaration *decl = ((AstDeclaration **)ast_struct->inside_methods.items)[i];

        typer->current_scope = ast_struct->members->parent;
        ok = check_declaration(typer, decl);
        typer->current_scope = scope;
        if (!ok) return false;
    }
    return true;
}

// Returns the instance of a generic function for the given type arguments, making and checking it the first time
AstFunctionDefn *instantiate_generic_function(Typer *typer, AstFunctionDefn *generic, DynamicArray type_args, Ast *site) {
    Parser *parser = typer->parser;

    // A readable name of the instance e.g "max<int>". Instances with the same name are the same instance
    StringBuilder sb = sb_init(32);
    sb_append(&sb, "%s<", generic->identifier->name);
    for (int i = 0; i < type_args.count; i++) {
        sb_append(&sb, "%s", type_to_str(((Type **)type_args.items)[i]));
        if (i + 1 < type_args.count) sb_append(&sb, ", ");
    }
    sb_append(&sb, ">");
    char *name = sb_to_string(&sb);

    StringBuilder key_sb = sb_init(32);
    sb_append(&key_sb, "<");
    for (int i = 0; i < type_args.count; i++) {
        sb_append(&key_sb, "%s", type_identity(((Type **)type_args.items)[i]));
        if (i + 1 < type_args.count) sb_append(&key_sb, ", ");
    }
    sb_append(&key_sb, ">");
    char *key = sb_to_string(&key_sb);

    for (int i = 0; i < generic->instances.count; i++) {
        AstFunctionDefn *existing = ((AstFunctionDefn **)generic->instances.items)[i];
        if (strcmp(existing->instance_key, key) == 0) {
            return existing;
        }
    }

    if (typer->generic_depth >= GENERIC_DEPTH_LIMIT) {
        report_error_ast(parser, LABEL_ERROR, site, "Too many nested instantiations of generic functions (the limit is %d) while instantiating '%s'", GENERIC_DEPTH_LIMIT, name);
        report_error_ast(parser, LABEL_HINT, NULL, "A generic function that calls itself with a new type e.g f<T> calling f<[]T> never stops making new instances");
        typer->generic_depth_limit_reached = true;
        return NULL;
    }

    // Scope binding the type parameters to the type arguments, put between the instance and the scope the generic function is declared in
    AstBlock *scope    = ast_allocate(parser, sizeof(AstBlock));
    scope->head        = generic->body->head;
    scope->kind        = BLOCK_IMPERATIVE;
    scope->parent      = generic->body->parent;
    scope->statements  = da_init(1, sizeof(Ast *));
    scope->identifiers = da_init(type_args.count, sizeof(AstIdentifier *));
    scope->enclosing_function = generic->body->parent ? generic->body->parent->enclosing_function : NULL;

    for (int i = 0; i < generic->type_params.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)generic->type_params.items)[i];

        AstIdentifier *bound = ast_allocate(parser, sizeof(AstIdentifier));
        memcpy(bound, param, sizeof(AstIdentifier));
        bound->type   = ((Type **)type_args.items)[i];
        bound->scope  = scope;
        bound->flags |= IDENTIFIER_IS_CONSTANT | IDENTIFIER_IS_RESOLVED | IDENTIFIER_IS_TYPE_ALIAS;
        da_append(&scope->identifiers, bound);
    }

    AstFunctionDefn *instance = reparse_generic_function(parser, generic, scope);
    if (!instance) return NULL;

    // An instance of a method on slices is a plain function, as slices have no struct to hold methods
    if (generic->is_slice_method) instance->is_method = false;

    AstIdentifier *ident = ast_allocate(parser, sizeof(AstIdentifier));
    memcpy(ident, generic->identifier, sizeof(AstIdentifier));
    ident->type = NULL; // Set when the instance is checked

    instance->identifier       = ident;
    instance->is_lambda        = generic->is_lambda;
    instance->generic_function = generic;
    instance->type_args        = type_args;
    instance->instance_name    = name;
    instance->instance_key     = key;

    // Add the instance before it is checked, so that recursive calls with the same type arguments find it
    da_append(&generic->instances, instance);

    Typer saved_state = *typer;
    typer->current_file        = generic->head.head.file;
    typer->current_scope       = scope;
    typer->enclosing_loop      = NULL;
    typer->inside_declaration  = NULL;
    typer->array_literal_depth = 0;
    typer->generic_depth      += 1;

    Type *ok = check_function_defn(typer, instance);

    DynamicArray stack = typer->enclosing_function_stack; // Might have been reallocated while checking
    typer_restore_state(typer, saved_state);
    typer->enclosing_function_stack = stack;
    typer->inside_declaration       = saved_state.inside_declaration;
    typer->generic_depth            = saved_state.generic_depth;

    if (!ok) {
        // When the depth limit is reached, only the outermost instantiation gets a note
        if (!typer->generic_depth_limit_reached || typer->generic_depth == 0) {
            report_error_ast(parser, LABEL_NOTE, site, "... in %s, instantiated here", name);
        }
        return NULL;
    }

    ident->type = instance->head.type;
    return instance;
}

// Infers the type arguments of a call to a generic function from the arguments and returns the instance to call
AstFunctionDefn *instantiate_generic_function_call(Typer *typer, AstFunctionCall *call, AstFunctionDefn *generic) {
    Parser *parser = typer->parser;

    Type *ok = check_function_defn(typer, generic);
    if (!ok) return NULL;

    int variadic_index = -1;
    for (int i = 0; i < generic->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)generic->parameters.items)[i];
        if (param->type && param->type->kind == TYPE_VARIADIC) {
            variadic_index = i;
            break;
        }
    }

    TypeBinding *bindings = calloc(generic->type_params.count, sizeof(TypeBinding));

    for (int i = 0; i < call->arguments.count; i++) {
        AstArgument *arg = ((AstArgument **)call->arguments.items)[i];

        AstIdentifier *param = NULL;
        if (arg->ident) {
            for (int j = 0; j < generic->parameters.count; j++) {
                AstIdentifier *candidate = ((AstIdentifier **)generic->parameters.items)[j];
                if (strcmp(candidate->name, arg->ident->name) == 0) {
                    param = candidate;
                    break;
                }
            }
        } else if (variadic_index != -1 && i >= variadic_index) {
            param = ((AstIdentifier **)generic->parameters.items)[variadic_index];
        } else if (i < generic->parameters.count) {
            param = ((AstIdentifier **)generic->parameters.items)[i];
        }

        // Arguments that don't match a parameter are reported when checking the call against the instance
        if (param == NULL || param->type == NULL) continue;
        if (!type_mentions_any_type_param(param->type, generic->type_params)) continue;

        // An argument that needs to know its type e.g the struct literal in points.add({1, 2}), is checked against the instance instead,
        // if its type parameters are already known e.g the T of add(value: T) from the receiver
        AstExpr *value = arg->value;
        bool needs_type = (value->head.kind == AST_STRUCT_LITERAL && ((AstStructLiteral *)value)->explicit_type == NULL) ||
                          value->head.kind == AST_ARRAY_LITERAL || value->head.kind == AST_ENUM_LITERAL;
        bool all_bound = needs_type;
        for (int j = 0; j < generic->type_params.count; j++) {
            AstIdentifier *type_param = ((AstIdentifier **)generic->type_params.items)[j];
            if (!all_bound) break;
            if (type_mentions_name(param->type, type_param->name) && bindings[j].kind != BINDING_TYPED) {
                all_bound = false;
            }
        }
        if (all_bound) continue;

        Type *arg_type = check_expression(typer, arg->value, NULL);
        if (!arg_type) return NULL;

        // A collection passed where a slice is wanted is passed as its elements e.g a [..]int for a []T
        Type *param_value_type = param->type->kind == TYPE_VARIADIC ? ((TypeVariadic *)param->type)->type : param->type;
        if (arg_type->kind == TYPE_STRUCT && param_value_type->kind == TYPE_ARRAY) {
            AstExpr *slice = collection_as_slice(typer, arg->value, NULL);
            if (slice) arg_type = slice->type;
        }

        // A spreaded slice or variadic gives the type of its elements to a variadic parameter
        if (arg->value->head.flags & AST_FLAG_EXPR_IS_SPREADED) {
            if (arg_type->kind == TYPE_ARRAY)    arg_type = ((TypeArray *)arg_type)->elem_type;
            if (arg_type->kind == TYPE_VARIADIC) arg_type = ((TypeVariadic *)arg_type)->type;
        }

        bool unified = unify_types(typer, generic, bindings, param->type, arg_type, arg->value, param);
        if (!unified) return NULL;
    }

    DynamicArray type_args = da_init(generic->type_params.count, sizeof(Type *));
    for (int i = 0; i < generic->type_params.count; i++) {
        AstIdentifier *type_param = ((AstIdentifier **)generic->type_params.items)[i];
        TypeBinding binding = bindings[i];

        if (binding.kind == BINDING_NONE) {
            report_error_token(parser, LABEL_ERROR, call->paren_start_token, "Cannot infer the type parameter '%s' of '%s', as no argument gives it a type", type_param->name, generic->identifier->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)generic->identifier, "Here is the definition of '%s'", generic->identifier->name);
            return NULL;
        }

        // Untyped literals get the same type they get everywhere else e.g x := 1
        Type *type = binding.type;
        if (binding.kind == BINDING_UNTYPED_INT)   type = solidify_untyped_type(type);
        if (binding.kind == BINDING_UNTYPED_FLOAT) type = primitive_type(PRIMITIVE_FLOAT);

        if (type->kind == TYPE_VOID) {
            report_error_ast(parser, LABEL_ERROR, binding.source, "Type parameter '%s' of '%s' cannot be void", type_param->name, generic->identifier->name);
            return NULL;
        }

        da_append(&type_args, type);
    }
    free(bindings);

    return instantiate_generic_function(typer, generic, type_args, (Ast *)call);
}

// The default value of a struct member e.g the 30 in 'age: int = 30;', or NULL
AstExpr *member_default_value(AstIdentifier *member) {
    AstDeclaration *decl = member->decl;
    if (decl == NULL || decl->values.count == 0) return NULL;

    for (int i = 0; i < decl->idents.count && i < decl->values.count; i++) {
        if (((AstIdentifier **)decl->idents.items)[i] == member) return ((AstExpr **)decl->values.items)[i];
    }
    return NULL;
}

// Checks an anonymous struct or enum e.g struct { name: string; age: int; } and gives it its name. Anonymous types with the same
// name have the same members, so the first one with a name is the type for all of them
Type *resolve_anonymous_type(Typer *typer, Type *type) {
    Parser *parser = typer->parser;
    StringBuilder sb  = sb_init(64);
    StringBuilder key = sb_init(64); // With the namespaces of the member types
    sb_append(&key, "struct { ");

    if (type->kind == TYPE_STRUCT) {
        TypeStruct *struct_type = (TypeStruct *)type;
        AstStruct  *node        = struct_type->node;
        if (node->head.flags & AST_FLAG_IS_TYPE_CHECKED) return type;

        Typer saved_state = *typer;
        bool ok = check_struct_defn(typer, node);
        DynamicArray stack = typer->enclosing_function_stack;
        typer_restore_state(typer, saved_state);
        typer->enclosing_function_stack = stack;
        typer->inside_declaration       = saved_state.inside_declaration;
        if (!ok) return NULL;

        sb_append(&sb, "struct { ");
        DynamicArray members = node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = ((AstIdentifier **)members.items)[i];
            sb_append(&sb, "%s: %s", member->name, type_to_str(member->type));
            sb_append(&key, "%s: %s", member->name, type_identity(member->type));

            // The default value is a part of the type, so structs with different defaults are different types
            AstExpr *value = member_default_value(member);
            if (value && value->head.file) {
                int start = value->head.start.input_idx;
                int end   = value->head.end.input_idx;
                sb_append(&sb, " = %.*s", end - start, value->head.file->text + start);
                sb_append(&key, " = %.*s", end - start, value->head.file->text + start);
            }
            sb_append(&sb, "; ");
            sb_append(&key, "; ");
        }
        sb_append(&sb, "}");
    } else {
        TypeEnum *enum_type = (TypeEnum *)type;
        AstEnum  *node      = enum_type->node;
        if (node->head.flags & AST_FLAG_IS_TYPE_CHECKED) return type;

        Type *ok = check_enum_defn(typer, node);
        if (!ok) return NULL;

        bool has_values = false;
        for (int i = 0; i < node->enumerators.count; i++) {
            if (((AstEnumerator **)node->enumerators.items)[i]->expr) has_values = true;
        }

        sb_append(&sb, "enum { ");
        for (int i = 0; i < node->enumerators.count; i++) {
            AstEnumerator *etor = ((AstEnumerator **)node->enumerators.items)[i];
            if (has_values) sb_append(&sb, "%s = %d", etor->name, etor->value);
            else            sb_append(&sb, "%s", etor->name);
            if (i + 1 < node->enumerators.count) sb_append(&sb, ", ");
        }
        sb_append(&sb, " }");
    }

    char *name = sb_to_string(&sb);
    char *identity = type->kind == TYPE_STRUCT ? (sb_append(&key, "}"), sb_to_string(&key)) : name;
    for (int i = 0; i < typer->anonymous_types.count; i++) {
        Type *existing = ((Type **)typer->anonymous_types.items)[i];
        if (existing->kind == type->kind && strcmp(type_identity(existing), identity) == 0) {
            return existing;
        }
    }

    type->name = name;
    if (type->kind == TYPE_STRUCT) {
        ((TypeStruct *)type)->identifier->name = name;
        ((TypeStruct *)type)->qualified_name   = identity;
    } else {
        ((TypeEnum *)type)->identifier->name = name;
    }
    da_append(&typer->anonymous_types, type);
    (void)parser;
    return type;
}

Type *resolve_type(Typer *typer, Type *type) {
    if (is_primitive_type(type->kind)) {
        return type;
    }

    if (type->kind == TYPE_ENUM) {
        if (((TypeEnum *)type)->node && ((TypeEnum *)type)->node->is_anonymous) {
            return resolve_anonymous_type(typer, type);
        }
        return type;
    }

    if (type->kind == TYPE_STRUCT && ((TypeStruct *)type)->node && ((TypeStruct *)type)->node->is_anonymous) {
        return resolve_anonymous_type(typer, type);
    }

    if (type->kind == TYPE_STRING) {
        TypeString *type_string = (TypeString *)type;

        if (typer->type_string == NULL) {
            AstIdentifier *type_string_identifier = lookup_from_scope(typer->parser, typer->current_scope, "String");

            if (!type_string_identifier) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)type_string, "Compiler Error: Failed to find the 'String' struct");
                return NULL;
            }

            if (type_string_identifier->type == NULL || type_string_identifier->type->kind != TYPE_STRUCT) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)type_string, "Compiler Error: Type 'String' was unresolved or of a different type in typechecking");
                return NULL;
            }
            
            // Construct a global instance of TypeString with the struct definition of the struct "String" found in the strings.sd package
            TypeString *str = ast_allocate(typer->parser, sizeof(TypeString));
            str->head.head.kind  = AST_TYPE;
            str->head.head.file  = type_string_identifier->head.file;
            str->head.head.start = type_string_identifier->type->head.start;
            str->head.head.end   = type_string_identifier->type->head.end;
            str->head.head.flags = AST_FLAG_COMPILER_GENERATED;
            str->head.kind       = TYPE_STRING;
            str->head.size       = 16;
            str->struct_defn     = (TypeStruct *)type_string_identifier->type;

            typer->type_string = str;
        }

        type_string->struct_defn      = typer->type_string->struct_defn;
        type_defn_string->struct_defn = typer->type_string->struct_defn;

        return (Type *) type_string;
    }

    if (type->kind == TYPE_ANY) {
        if (!typer->inside_extern_block)  {
            TypeAny *any = (TypeAny *)type;
    
            // Look for the Any :: struct {...} in the "runtime" package
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
        AstIdentifier *ident = lookup_type_identifier(typer, type->name, (Ast *)type);
        if (ident == NULL && strchr(type->name, '.')) return NULL; // Reported by lookup_type_identifier
        if (ident == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "Unknown type '%s'", type_to_str(type));
            return NULL;
        }
        report_if_deprecated(typer, ident, (Ast *)type);

        if (ident->flags & IDENTIFIER_IS_AMBIGUOUS) {
            AstIdentifier *first = ((AstIdentifier **)ident->ambiguous.items)[0];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "'%s' is ambiguous. It is declared in more than one of the namespaces you use", type->name);
            for (int i = 0; i < ident->ambiguous.count; i++) {
                AstIdentifier *candidate = ((AstIdentifier **)ident->ambiguous.items)[i];
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)candidate, "In '%s'", namespace_of(candidate));
            }
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Write it with its namespace e.g %s.%s", namespace_of(first), type->name);
            return NULL;
        }

        // A constant that has not been checked yet might be a type alias declared further down e.g 'b: Byte' before 'Byte :: u8;'
        if (ident->decl && (ident->decl->flags & DECLARATION_CONSTANT) && !(ident->flags & IDENTIFIER_IS_CONSTANT_FUNCTION_DEFN)) {
            bool ok = check_declaration_of(typer, ident);
            if (!ok) return NULL;
        }

        if (ident->type == NULL || !identifier_is_type(ident)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "'%s' is not a type", type->name);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(ident), "Here is the declaration of '%s'", ident->name);
            return NULL;
        }

        if (ident->type && ident->type->kind == TYPE_STRUCT && ((TypeStruct *)ident->type)->node->type_params.count > 0) {
            AstStruct *generic = ((TypeStruct *)ident->type)->node;
            AstIdentifier *param = ((AstIdentifier **)generic->type_params.items)[0];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "Generic struct '%s' is missing its type arguments", type->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Give it a type for '%s', e.g %s<int>", param->name, type->name);
            return NULL;
        }

        // A struct declared further down is checked now, so that its members are known e.g 'holder: Outer' before 'Outer :: struct {...}'.
        // While sizing structs, the sizing itself takes care of the order
        if (ident->type && ident->type->kind == TYPE_STRUCT && typer->struct_check_depth == 0) {
            AstStruct *node = ((TypeStruct *)ident->type)->node;
            if (node && !(node->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
                Typer saved_state = *typer;
                bool ok = check_struct_defn(typer, node);
                typer_restore_state(typer, saved_state);
                typer->inside_declaration = saved_state.inside_declaration;
                if (!ok) return NULL;
            }
        }

        return ident->type;
    }

    if (type->kind == TYPE_GENERIC_INSTANCE) {
        TypeStruct *instance = instantiate_generic_struct(typer, (TypeGenericInstance *)(type));
        if (!instance) return NULL;

        return (Type *)(instance);
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

        // Resolve the type of the .data member
        AstIdentifier *memberData = get_struct_member(array->struct_defn->node, "data");
        if (memberData == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)array, "Compiler Error: Failed to resolve .data member on array");
            return NULL;
        }
        ((TypePointer *)memberData->type)->pointer_to = elem_type;

        switch (array->array_kind) {
        case ARRAY_FIXED: {
            assert(array->capacity_expr);

            Type *cap_type = check_expression(typer, array->capacity_expr, NULL);
            if (!cap_type) return NULL;

            AstExpr *const_expr = simplify_expression(typer->const_evaluator, typer->current_scope, array->capacity_expr);
            if (const_expr->head.kind != AST_LITERAL && ((AstLiteral *)(const_expr))->kind != LITERAL_INTEGER) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array->capacity_expr), "Size of the array must be an integer constant");
                return NULL;
            }

            long long capacity = ((AstLiteral *)(const_expr))->as.value.integer;
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
            array->head.size = 32; // Reserve 32 bytes for .data, .count, .capacity and .elemSize
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

    if (type->kind == TYPE_STRUCT) {
        if (type->flags & TYPE_IS_FULLY_SIZED) {
            return type;
        } else {
            XXX;
        }
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *) type, "Compiler Error: Failed to resolve type %s", type_to_str(type));

    return NULL;
}

unsigned long long max_integer_value(TypePrimitive *type) {
    switch (type->kind) {
    case PRIMITIVE_UINT: return U32_MAX;
    case PRIMITIVE_INT:  return S32_MAX;
    case PRIMITIVE_CHAR: return U8_MAX;
    case PRIMITIVE_U8:  return U8_MAX;
    case PRIMITIVE_U16: return U16_MAX;
    case PRIMITIVE_U32: return U32_MAX;
    case PRIMITIVE_U64: return U64_MAX;
    case PRIMITIVE_I8:  return S8_MAX;
    case PRIMITIVE_I16: return S16_MAX;
    case PRIMITIVE_I32: return S32_MAX;
    case PRIMITIVE_I64: return S64_MAX;
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

    // 16 bytes for preserving the destination address + value across a function call
    reserve_temporary_storage(typer->enclosing_function, 16);

    // Reserve space for copying the value on the stack. The type descriptor is a constant, see type_info_global() in ir_lower.c
    reserve_local_storage(typer->enclosing_function, value_type->size);
}

//
// Read-only self
//
// In a 'method' declared inside a struct (not a 'method *'), 'self' is a pointer under the hood, but nothing may be changed through it.
// It is shallow: only the struct itself is read-only, not what its members point to, e.g the elements of a dynamic array member
//

// Returns the method if the expression is its read-only 'self'
AstFunctionDefn *readonly_self_method(Typer *typer, AstExpr *expr) {
    if (!is_literal_identifier(expr)) return NULL;

    AstIdentifier *ident = ((AstLiteral *)expr)->as.value.identifier.resolved_identifier;
    if (ident == NULL) return NULL;

    AstFunctionDefn *func = typer->enclosing_function;
    if (func && func->self_struct && !func->self_is_mutable && func->parameters.count > 0 && ((AstIdentifier **)func->parameters.items)[0] == ident) {
        return func;
    }
    return NULL;
}

// Returns the method if the place is a part of its read-only 'self' e.g self.count, self.pos.x, *self or self.items[i] when items is a fixed size array.
// Places reached through a pointer, like self.next.value or the elements of a dynamic array, are not a part of self
AstFunctionDefn *readonly_self_place(Typer *typer, AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma = (AstMemberAccess *)expr;
        if (ma->access_kind != MEMBER_ACCESS_STRUCT_MEMBER) return NULL;

        AstFunctionDefn *method = readonly_self_method(typer, ma->left);
        if (method) return method; // e.g self.count

        if (ma->left->type && ma->left->type->kind == TYPE_POINTER) return NULL; // e.g self.next.value
        return readonly_self_place(typer, ma->left);                           // e.g self.pos.x or self.items.count
    }
    case AST_ARRAY_ACCESS: {
        AstArrayAccess *access = (AstArrayAccess *)expr;
        Type *type = access->left->type;
        if (type && type->kind == TYPE_ARRAY && ((TypeArray *)type)->array_kind == ARRAY_FIXED) {
            return readonly_self_place(typer, access->left); // The elements of a fixed size array are inside the struct
        }
        return NULL;
    }
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *)expr;
        if (unary->operator == OP_POINTER_DEREFERENCE) return readonly_self_method(typer, unary->expr); // *self
        return NULL;
    }
    default:
        return NULL;
    }
}

void append_place_text(StringBuilder *sb, AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_LITERAL:
        if (is_literal_identifier(expr)) sb_append(sb, "%s", ((AstLiteral *)expr)->as.value.identifier.name);
        else                             sb_append(sb, "...");
        return;
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma = (AstMemberAccess *)expr;
        append_place_text(sb, ma->left);
        sb_append(sb, ".");
        append_place_text(sb, ma->right);
        return;
    }
    case AST_ARRAY_ACCESS:
        append_place_text(sb, ((AstArrayAccess *)expr)->left);
        sb_append(sb, "[...]");
        return;
    case AST_UNARY:
        sb_append(sb, ((AstUnary *)expr)->operator == OP_ADDRESS_OF ? "&" : "*");
        append_place_text(sb, ((AstUnary *)expr)->expr);
        return;
    default:
        sb_append(sb, "...");
        return;
    }
}

void report_readonly_self(Typer *typer, Ast *at, AstFunctionDefn *method, const char *action, AstExpr *place) {
    StringBuilder sb = sb_init(32);
    append_place_text(&sb, place);
    char *place_text = sb_to_string(&sb);

    char *struct_name = method->self_struct->identifier->name;
    char *method_name = method->identifier->name;
    report_error_ast(typer->parser, LABEL_ERROR, at, "Cannot %s %s in the non-mutating method %s.%s", action, place_text, struct_name, method_name);
    report_error_ast(typer->parser, LABEL_HINT, NULL, "self is read-only in a 'method'. Declare it as 'method *' to be able to change self e.g %s :: method * (...)", method_name);
}

// Returns true if calling the method can change its receiver
bool method_can_change_receiver(AstFunctionDefn *method) {
    if (method->self_struct) return method->self_is_mutable;

    AstIdentifier *receiver = method->receiver ? method->receiver : ((AstIdentifier **)method->parameters.items)[0];
    return receiver->type && receiver->type->kind == TYPE_POINTER;
}

// Reports an error if a private declaration is used outside of its namespace (or file, if it has no namespace)
bool report_if_not_visible(Typer *typer, AstIdentifier *ident, Ast *site) {
    if (!(ident->flags & IDENTIFIER_IS_PRIVATE)) return true;

    AstFile *from = typer->current_file;
    AstFile *decl = ident->head.file;
    bool visible = decl == from || (decl && from && decl->ns && decl->ns == from->ns);
    if (visible) return true;

    if (decl && decl->ns) report_error_ast(typer->parser, LABEL_ERROR, site, "'%s' is private to the namespace '%s'", ident->name, decl->ns->name);
    else                  report_error_ast(typer->parser, LABEL_ERROR, site, "'%s' is private to its file", ident->name);
    report_error_ast(typer->parser, LABEL_NOTE, (Ast *)ident, "Here is the declaration of '%s'", ident->name);
    return false;
}

// Warns about the use of a @deprecated declaration
void report_if_deprecated(Typer *typer, AstIdentifier *ident, Ast *site) {
    AstIdentifier *decl = ident->imported_from ? ident->imported_from : ident;
    if (!(decl->flags & IDENTIFIER_IS_DEPRECATED)) return;

    if (decl->deprecated_message) report_error_ast(typer->parser, LABEL_WARNING, site, "'%s' is deprecated: %s", decl->name, decl->deprecated_message);
    else                          report_error_ast(typer->parser, LABEL_WARNING, site, "'%s' is deprecated", decl->name);
}

// The private struct or enum in a type e.g the Slot of *[]Slot, or NULL
AstIdentifier *private_type_in(Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT: {
        AstIdentifier *ident = ((TypeStruct *)type)->identifier;
        if (ident && (ident->flags & IDENTIFIER_IS_PRIVATE)) return ident;
        AstStruct *node = ((TypeStruct *)type)->node;
        if (node && node->generic_struct) {
            for (int i = 0; i < node->type_args.count; i++) {
                AstIdentifier *inner = private_type_in(((Type **)node->type_args.items)[i]);
                if (inner) return inner;
            }
        }
        return NULL;
    }
    case TYPE_ENUM: {
        AstIdentifier *ident = ((TypeEnum *)type)->identifier;
        return ident && (ident->flags & IDENTIFIER_IS_PRIVATE) ? ident : NULL;
    }
    case TYPE_POINTER:  return private_type_in(((TypePointer *)type)->pointer_to);
    case TYPE_ARRAY:    return private_type_in(((TypeArray *)type)->elem_type);
    case TYPE_VARIADIC: return private_type_in(((TypeVariadic *)type)->type);
    default:            return NULL;
    }
}

// A public function cannot take or return a private type, as the callers can't name it
bool check_no_private_types_in_signature(Typer *typer, AstFunctionDefn *func_defn) {
    if (func_defn->is_lambda || func_defn->identifier == NULL) return true;
    if (func_defn->identifier->flags & IDENTIFIER_IS_PRIVATE) return true;

    for (int i = func_defn->is_method ? 1 : 0; i < func_defn->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];
        AstIdentifier *hidden = private_type_in(param->type);
        if (hidden) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)param->decl, "The public function '%s' takes the private type '%s'", func_defn->identifier->name, hidden->name);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)hidden, "Here is the declaration of '%s'", hidden->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Make the function @private too, or the type public");
            return false;
        }
    }
    for (int i = 0; i < func_defn->return_types.count; i++) {
        Type *type = ((Type **)func_defn->return_types.items)[i];
        AstIdentifier *hidden = private_type_in(type);
        if (hidden) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)func_defn->identifier, "The public function '%s' returns the private type '%s'", func_defn->identifier->name, hidden->name);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)hidden, "Here is the declaration of '%s'", hidden->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Make the function @private too, or the type public");
            return false;
        }
    }
    return true;
}

// Collects the names of a qualified name e.g [game, physics, Body] for game.physics.Body. Returns false if it is not only names
bool collect_qualified_names(AstExpr *expr, char **names, int *count, int max) {
    if (expr->head.kind == AST_LITERAL && ((AstLiteral *)expr)->kind == LITERAL_IDENTIFIER) {
        if (*count >= max) return false;
        names[(*count)++] = ((AstLiteral *)expr)->as.value.identifier.name;
        return true;
    }
    if (expr->head.kind == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)expr;
        if (!collect_qualified_names(ma->left, names, count, max)) return false;
        return collect_qualified_names(ma->right, names, count, max);
    }
    return false;
}

// The namespace that a list of names refers to e.g [m] with 'import math as m;', or [game, physics]. NULL if it is not a namespace
Namespace *namespace_of_names(Typer *typer, char **names, int count) {
    AstIdentifier *first = lookup_from_scope(typer->parser, typer->current_scope, names[0]);
    if (first && !(first->flags & IDENTIFIER_IS_NAMESPACE)) return NULL; // A name in scope wins over a namespace with the same name

    StringBuilder sb = sb_init(32);
    sb_append(&sb, "%s", first ? first->ns->name : names[0]);
    for (int i = 1; i < count; i++) sb_append(&sb, ".%s", names[i]);
    char *path = sb_to_string(&sb);
    return find_namespace(typer->parser, path);
}

// Returns the declaration of a qualified name e.g math.sin or m.Vec2, or NULL if the expression is not one.
// Sets 'failed' if it is a qualified name, but there is an error
AstIdentifier *resolve_qualified_name(Typer *typer, AstExpr *expr, bool *failed) {
    *failed = false;

    char *names[16];
    int   count = 0;
    if (!collect_qualified_names(expr, names, &count, 16) || count < 2) return NULL;

    Namespace *ns = namespace_of_names(typer, names, count - 1);
    if (ns == NULL) return NULL;

    AstIdentifier *ident = find_namespace_declaration(ns, names[count - 1]);
    if (ident == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)expr, "'%s' is not declared in the namespace '%s'", names[count - 1], ns->name);
        *failed = true;
        return NULL;
    }
    if (!report_if_not_visible(typer, ident, (Ast *)expr)) {
        *failed = true;
        return NULL;
    }
    return ident;
}

// Looks up the name of a type, which can be qualified e.g Vec2, m.Vec2 or game.physics.Body
AstIdentifier *lookup_type_identifier(Typer *typer, char *name, Ast *site) {
    if (strchr(name, '.') == NULL) {
        return lookup_from_scope(typer->parser, typer->current_scope, name);
    }

    char *copy = strdup(name);
    char *names[16];
    int   count = 0;
    for (char *part = strtok(copy, "."); part && count < 16; part = strtok(NULL, ".")) names[count++] = part;

    Namespace *ns = namespace_of_names(typer, names, count - 1);
    if (ns == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, site, "Unknown namespace in the type '%s'", name);
        return NULL;
    }

    AstIdentifier *ident = find_namespace_declaration(ns, names[count - 1]);
    if (ident == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, site, "'%s' is not declared in the namespace '%s'", names[count - 1], ns->name);
        return NULL;
    }
    if (!report_if_not_visible(typer, ident, site)) return NULL;
    return ident;
}

// An identifier expression that already knows its declaration e.g the 'sin' of math.sin
AstLiteral *make_resolved_identifier_literal(Typer *typer, AstIdentifier *ident, Ast *at) {
    AstLiteral *literal = ast_allocate(typer->parser, sizeof(AstLiteral));
    literal->head.head       = *at;
    literal->head.head.kind  = AST_LITERAL;
    literal->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    literal->kind            = LITERAL_IDENTIFIER;
    literal->as.value.identifier.name   = ident->name;
    literal->as.value.identifier.length = (int)strlen(ident->name);
    literal->as.value.identifier.resolved_identifier = ident;
    return literal;
}

// The name a type is told apart by: the namespace is a part of the name of structs and enums e.g "raylib.Color"
char *type_identity(Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT: return ((TypeStruct *)type)->qualified_name ? ((TypeStruct *)type)->qualified_name : type->name;
    case TYPE_ENUM:   return ((TypeEnum *)type)->qualified_name   ? ((TypeEnum *)type)->qualified_name   : type->name;
    case TYPE_POINTER: {
        StringBuilder sb = sb_init(32);
        sb_append(&sb, "*%s", type_identity(((TypePointer *)type)->pointer_to));
        return sb_to_string(&sb);
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *)type;
        StringBuilder sb = sb_init(32);
        if (array->array_kind == ARRAY_FIXED)        sb_append(&sb, "[%lld]%s", array->capacity, type_identity(array->elem_type));
        else if (array->array_kind == ARRAY_SLICE)   sb_append(&sb, "[]%s", type_identity(array->elem_type));
        else                                         sb_append(&sb, "[..]%s", type_identity(array->elem_type));
        return sb_to_string(&sb);
    }
    default:
        return type_to_str(type);
    }
}

// Returns true if the identifier names a type e.g a struct, an enum, a type alias or a type parameter bound in an instance
bool identifier_is_type(AstIdentifier *ident) {
    return ident->flags & (IDENTIFIER_IS_NAME_OF_STRUCT | IDENTIFIER_IS_NAME_OF_ENUM | IDENTIFIER_IS_TYPE_ALIAS);
}

// Checks the declaration of an identifier from where it is declared, if it has not been checked yet e.g a type alias declared further down
bool check_declaration_of(Typer *typer, AstIdentifier *ident) {
    if (ident->decl == NULL || (ident->decl->head.flags & AST_FLAG_IS_TYPE_CHECKED)) return true;

    Typer saved_state = *typer;
    typer->current_scope = ident->scope ? ident->scope : typer->current_scope;
    typer->current_file  = ident->head.file ? ident->head.file : typer->current_file;

    bool ok = check_declaration(typer, ident->decl);

    DynamicArray stack = typer->enclosing_function_stack; // Might have been reallocated while checking
    typer_restore_state(typer, saved_state);
    typer->enclosing_function_stack = stack;
    typer->inside_declaration       = saved_state.inside_declaration;
    return ok;
}

// Type aliases e.g Byte :: u8; or IntList :: List<int>; An alias is the same type as the type it names
bool check_type_alias(Typer *typer, AstDeclaration *decl) {
    AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[0];

    if (decl->head.flags & AST_FLAG_IS_BEING_CHECKED) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ident, "Type alias '%s' refers to itself", ident->name);
        return false;
    }

    decl->head.flags |= AST_FLAG_IS_BEING_CHECKED;
    Type *type = resolve_type(typer, decl->type);
    decl->head.flags &= ~AST_FLAG_IS_BEING_CHECKED;
    if (!type) return false;

    decl->type   = type;
    ident->type  = type;
    ident->flags |= IDENTIFIER_IS_RESOLVED | IDENTIFIER_IS_TYPE_ALIAS;
    decl->head.flags  |= AST_FLAG_IS_TYPE_CHECKED;
    ident->head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    return true;
}

bool check_declaration(Typer *typer, AstDeclaration *decl) {
    if (decl->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return true;
    }

    // 'Alias :: Point;' names a type if Point is a type, otherwise it is a normal constant e.g 'B :: A;' where 'A :: 5;'
    if ((decl->flags & DECLARATION_CONSTANT) && !(decl->flags & DECLARATION_TYPE_ALIAS) && decl->idents.count == 1 && decl->values.count == 1) {
        AstExpr *value = ((AstExpr **)decl->values.items)[0];
        if (is_literal_identifier(value)) {
            AstIdentifier *named = lookup_from_scope(typer->parser, typer->current_scope, ((AstLiteral *)value)->as.value.identifier.name);
            if (named && identifier_is_type(named)) {
                Type *type_name = type_alloc(&typer->parser->type_table, sizeof(Type));
                type_name->head      = value->head;
                type_name->head.kind = AST_TYPE;
                type_name->kind      = TYPE_NAME;
                type_name->name      = named->name;

                AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[0];
                decl->flags |= DECLARATION_TYPE_ALIAS;
                decl->type   = type_name;
                ident->flags |= IDENTIFIER_IS_TYPE_ALIAS;
            }
        }
    }

    if (decl->flags & DECLARATION_TYPE_ALIAS) {
        return check_type_alias(typer, decl);
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

            if (!types_are_equal(decl->type, value->type)) {
                AstExpr *slice = collection_as_slice(typer, value, decl->type);
                if (slice) {
                    *value_ptr = slice;
                    value = slice;
                }
            }

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
            AstIdentifier *ident = ((AstIdentifier **)decl->idents.items)[i];

            AstExpr **value_ptr = &((AstExpr **)decl->values.items)[i];
            AstExpr *value = *value_ptr;
            Type *expr_type = check_expression(typer, value, NULL);
            if (!expr_type) return false;

            ident->type = expr_type;

            AstExpr *const_expr = simplify_expression(typer->const_evaluator, typer->current_scope, value);
            if (const_expr == NULL) {
                return false;
            }
            if (const_expr->head.kind == AST_LITERAL) {
                // Swap out the current expression for the simplified expression
                // @Speed? @Note - We might in the future cleanup the already allocated expression tree as its no longer needed after this swap.
                //                 We might be taking many cache misses if we leave big gaps in the allocated ast nodes, so something like packing the ast nodes could be
                //                 a thing?
                *value_ptr = const_expr;
            }
            else if (const_expr->head.kind == AST_FUNCTION_DEFN) {
                *value_ptr = const_expr;
            }
            else if (const_expr->head.kind == AST_STRUCT_LITERAL) {
                // Do nothing as we don't fold inner structs
                *value_ptr = const_expr;
            }
            else if (const_expr->head.kind == AST_ARRAY_LITERAL) {
                // Do nothing as we don't fold inner structs
                *value_ptr = const_expr;
            }
            else {
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

    // A read-only self cannot be stored, as the pointer could be used to change self
    for (int i = 0; i < decl->values.count; i++) {
        AstExpr *value = ((AstExpr **)decl->values.items)[i];

        AstFunctionDefn *readonly = readonly_self_method(typer, value);
        if (readonly && value->type && value->type->kind == TYPE_POINTER) {
            report_readonly_self(typer, (Ast *)value, readonly, "store the pointer", value);
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "It could be used to change self. Copy it with *self instead");
            return false;
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

            // A variant of a union without a value e.g 'empty;' has type void
            bool is_union_variant = (decl->flags & DECLARATION_IS_STRUCT_MEMBER) && typer->current_scope && typer->current_scope->belongs_to_struct && typer->current_scope->belongs_to_struct->is_union;
            if (ident->type->kind == TYPE_VOID && !is_union_variant) {
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
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)decl, "Global variables are not yet supported. Only constants");
        return false;
    }

    return true;
}

bool types_are_equal(Type *lhs, Type *rhs) {
    if (lhs->kind == TYPE_NAME || rhs->kind == TYPE_NAME) {
        // Type names should have been resolved at this point
        assert(false && "Comparing an unresolved struct or enum in types_are_equal()"); 
    }

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

    if (lhs->kind == TYPE_STRING && rhs->kind == TYPE_STRING) {
        return true;
    }

    if ((lhs->kind == TYPE_STRUCT && rhs->kind == TYPE_STRUCT) || (lhs->kind == TYPE_ENUM && rhs->kind == TYPE_ENUM)) {
        return lhs == rhs || strcmp(type_identity(lhs), type_identity(rhs)) == 0;
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

    // Allow normal varargs passing
    if (lhs->kind == TYPE_VARIADIC && rhs->kind != TYPE_VARIADIC) {
        // Match on the inner type
        Type *inner_lhs = ((TypeVariadic *) lhs)->type;

        return types_are_equal(inner_lhs, rhs);
    }

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
    typer->checking_place = true;
    Type *lhs_type  = check_expression(typer, assign->lhs, NULL);
    if (!lhs_type) return false;

    AstFunctionDefn *readonly = readonly_self_place(typer, assign->lhs);
    if (!readonly) readonly = readonly_self_method(typer, assign->lhs);
    if (readonly) {
        report_readonly_self(typer, (Ast *)assign->lhs, readonly, "assign to", assign->lhs);
        return false;
    }

    Type *expr_type = check_expression(typer, assign->expr, lhs_type);
    if (!expr_type) return false;

    if (!types_are_equal(lhs_type, expr_type)) {
        AstExpr *slice = collection_as_slice(typer, assign->expr, lhs_type);
        if (slice) {
            assign->expr = slice;
            expr_type    = slice->type;
        }
    }

    readonly = readonly_self_method(typer, assign->expr);
    if (readonly && expr_type->kind == TYPE_POINTER) {
        report_readonly_self(typer, (Ast *)assign->expr, readonly, "store the pointer", assign->expr);
        report_error_ast(typer->parser, LABEL_NOTE, NULL, "It could be used to change self. Copy it with *self instead");
        return false;
    }


    bool lhs_is_constant = false;
    if (assign->lhs->head.kind == AST_MEMBER_ACCESS) {
        AstIdentifier * member = ((AstMemberAccess *)(assign->lhs))->struct_member;
        lhs_is_constant = member->flags & IDENTIFIER_IS_CONSTANT;
    } 
    else if (assign->lhs->head.kind == AST_ARRAY_ACCESS) {
        // @Incomplete: I guess this could be a constant?
        lhs_is_constant = false;
    }
    else if (is_literal_identifier(assign->lhs)) {
        AstIdentifier *ident = ((AstLiteral *)(assign->lhs))->as.value.identifier.resolved_identifier;
        assert(ident); // Is checked in check_expression so no need to check it here also
        lhs_is_constant = ident->flags & IDENTIFIER_IS_CONSTANT;
    } 
    else if (assign->lhs->head.kind == AST_UNARY && ((AstUnary *)assign->lhs)->operator == OP_POINTER_DEREFERENCE) {
        // @Incomplete: I guess this could be a constant?
        lhs_is_constant = false;
    }
    else {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)assign->lhs, "Invalid left hand side of assigment");
        return false;
    }

    if (lhs_is_constant) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Cannot assign new value to constant");
        return false;
    }

    if (assign->op != '=') {
        bool is_integer_only = assign->op != ASSIGN_PLUS_EQUAL && assign->op != ASSIGN_MINUS_EQUAL && assign->op != ASSIGN_TIMES_EQUAL && assign->op != ASSIGN_DIVIDE_EQUAL;
        if (is_integer_only && (lhs_type->kind != TYPE_INTEGER || expr_type->kind != TYPE_INTEGER)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign), "Operator '%s' is only allowed for integer types. Given type was '%s'", token_type_to_str((TokenType) assign->op), type_to_str(lhs_type->kind != TYPE_INTEGER ? lhs_type : expr_type));
            return false;
        }
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

    if (!types_are_equal(lhs_type, expr_type)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(assign->expr), "Assignment type mismatch. Wanted type %s. Got type %s", type_to_str(lhs_type), type_to_str(expr_type));
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

int get_alignment_of_type(Type *type) {
    if (type->kind == TYPE_STRUCT) {
        return ((TypeStruct *)(type))->alignment;
    }
    else if (type->kind == TYPE_STRING) {
        return 8;
    }
    else if (type->kind == TYPE_ANY) {
        return 8;
    }
    else if (type->kind == TYPE_ARRAY) {
        TypeArray *array_type = (TypeArray *) type;

        if (array_type->array_kind == ARRAY_FIXED) {
            return get_alignment_of_type(array_type->elem_type);
        } 
        else if (array_type->array_kind == ARRAY_SLICE || array_type->array_kind == ARRAY_DYNAMIC) {
            return 8;
        } 
        else {
            XXX;
        }
    }
    else {
        return type->size;
    }
}

bool check_struct_members(Typer *typer, AstStruct *ast_struct);

bool check_struct_defn(Typer *typer, AstStruct *ast_struct) {
    typer->struct_check_depth += 1;
    bool ok = check_struct_members(typer, ast_struct);
    typer->struct_check_depth -= 1;
    return ok;
}

bool check_struct_members(Typer *typer, AstStruct *ast_struct) {
    if (ast_struct->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return true;
    }

    if (ast_struct->type_params.count > 0) {
        // Generic structs are not checked themselves, only their instances are. See instantiate_generic_struct()
        ast_struct->head.flags |= AST_FLAG_IS_TYPE_CHECKED;
        return true;
    }

    TypeStruct *struct_type = (TypeStruct *) ast_struct->identifier->type;
    assert(struct_type != NULL && struct_type->head.kind == TYPE_STRUCT);

    typer->current_scope = ast_struct->members;
    DynamicArray members = ast_struct->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];

        // The default values of the members are not inside a function, even if the struct is checked while checking one
        AstFunctionDefn *enclosing_function = typer->enclosing_function;
        bool             checking_default   = typer->checking_member_default;
        typer->enclosing_function      = NULL;
        typer->checking_member_default = true;
        bool ok = check_declaration(typer, member->decl);
        typer->enclosing_function      = enclosing_function;
        typer->checking_member_default = checking_default;
        if (!ok) return false;

        if (member->decl->type == (Type *) struct_type) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *) member->decl, "Illegal recursive definition of type '%s'", struct_type->node->identifier->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Did you mean to give it type *%s?", struct_type->node->identifier->name);
            return false;
        }

        if (member->type->kind == TYPE_STRUCT) {
            if ( !(member->type->flags & TYPE_IS_FULLY_SIZED) && member->type != (Type *) struct_type) {
                AstStruct *non_checked_struct = ((TypeStruct *)member->type)->node;

                // Check for cyclic sizing dependency
                for (int i = 0; i < typer->struct_wait_list.count; i++) {
                    AstIdentifier *waiting_struct = ((AstIdentifier **)typer->struct_wait_list.items)[i];
                    
                    if (non_checked_struct->identifier == waiting_struct) {
                        // Detected a cycle!
                        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)member->type, "Cyclic struct dependency detected. Struct %s and %s are dependent on eachothers types to be sized", waiting_struct->name, ast_struct->identifier->name);
                        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)ast_struct, "");
                        report_error_ast(typer->parser, LABEL_HINT, NULL, "One of the structs needs to be taken by pointer");
                        return NULL;
                    }
                }

                // Add ourself to the struct "needing to be sized" waiting list
                da_append(&typer->struct_wait_list, ast_struct->identifier);

                // Go typecheck the unknown struct
                Typer current_state = *typer;
                bool ok = check_struct_defn(typer, non_checked_struct);
                if (!ok) return NULL;
                typer_restore_state(typer, current_state);

                // Pop ourselves from the waiting list
                da_remove(&typer->struct_wait_list, typer->struct_wait_list.count - 1);
            }
        }

    }
    typer->current_scope = ast_struct->members->parent;

    
    // C-style struct alignment + padding
    int largest_alignment = 0;
    int offset  = 0;

    if (ast_struct->is_union) {
        // The tag, and then the variants on top of each other
        int payload_alignment = 1;
        int payload_size      = 0;
        for (int i = 1; i < members.count; i++) {
            AstIdentifier *member = ((AstIdentifier **)members.items)[i];
            if (member->decl && member->decl->values.count > 0) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)((AstExpr **)member->decl->values.items)[0], "The variants of a union can't have default values");
                return false;
            }
            int alignment = get_alignment_of_type(member->type);
            if (alignment > payload_alignment) payload_alignment = alignment;
            if (member->type->size > payload_size) payload_size = member->type->size;
        }

        int payload_offset = align_up(4, payload_alignment);
        ((AstIdentifier **)members.items)[0]->member_offset = 0;
        for (int i = 1; i < members.count; i++) {
            ((AstIdentifier **)members.items)[i]->member_offset = payload_offset;
        }
        offset            = payload_offset + payload_size;
        largest_alignment = payload_alignment > 4 ? payload_alignment : 4;
    }

    for (int i = 0; i < members.count && !ast_struct->is_union; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];

        int alignment = get_alignment_of_type(member->type);

        offset = align_up(offset, alignment);

        member->member_offset = offset;
        offset               += member->type->size;

        if (alignment > largest_alignment) {
            largest_alignment = alignment;
        }
    }

    ast_struct->head.flags |= AST_FLAG_IS_TYPE_CHECKED;

    struct_type->head.size  = align_up(offset, largest_alignment);
    struct_type->head.flags |= TYPE_IS_FULLY_SIZED;
    struct_type->alignment  = largest_alignment;


    // Check all constant members
    // NOTE: We first check the constant members after the struct has been sized, as
    // the constants might have expressions of the struct type itself. See constants_in_structs.sd
    typer->current_scope = ast_struct->static_members;
    DynamicArray static_members = ast_struct->static_members->identifiers;
    for (int i = 0; i < static_members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)static_members.items)[i];

        if (member->flags & IDENTIFIER_IS_NAME_OF_STRUCT) {
            continue;
        }

        bool ok = check_declaration(typer, member->decl);
        if (!ok) return false;
    }
    typer->current_scope = ast_struct->static_members->parent;

    // The attributes of the members e.g @json.Field(name: "id")
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        for (int j = 0; j < member->attributes.count; j++) {
            AstStructLiteral *value = ((AstStructLiteral **)member->attributes.items)[j];
            if (!check_attribute_value(typer, value)) return false;
        }
    }

    return true;
}

// True if the expression is known when compiling, so that it can be stored in the value of an attribute
bool is_constant_value(AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)expr;
        if (lit->kind != LITERAL_IDENTIFIER) return true;
        AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
        return ident && (ident->flags & IDENTIFIER_IS_CONSTANT) && ident->value && is_constant_value(ident->value);
    }
    case AST_ENUM_LITERAL:
        return true;
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma = (AstMemberAccess *)expr;
        if (ma->access_kind == MEMBER_ACCESS_ENUM_MEMBER) return true;
        if (ma->access_kind == MEMBER_ACCESS_STATIC_STRUCT_MEMBER) return ma->struct_member->value && is_constant_value(ma->struct_member->value);
        return false;
    }
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *)expr;
        return unary->operator == OP_UNARY_MINUS && unary->expr->head.kind == AST_LITERAL && is_constant_value(unary->expr);
    }
    case AST_CAST:
        return is_constant_value(((AstCast *)expr)->expr);
    case AST_STRUCT_LITERAL: {
        AstStructLiteral *lit = (AstStructLiteral *)expr;
        for (int i = 0; i < lit->initializers.count; i++) {
            AstStructInitializer *init = ((AstStructInitializer **)lit->initializers.items)[i];
            if (!is_constant_value(init->value)) return false;
        }
        return true;
    }
    default:
        return false;
    }
}

// The value of an attribute on a struct member e.g @json.Field(name: "id") is a struct literal of a struct marked with @attribute,
// with values that are known when compiling, as it is stored in the program for reflection
bool check_attribute_value(Typer *typer, AstStructLiteral *value) {
    Type *type = check_expression(typer, (AstExpr *)value, NULL);
    if (!type) return false;

    if (type->kind != TYPE_STRUCT || !((TypeStruct *)type)->node->is_attribute) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)value, "'%s' is not an attribute", type_to_str(type));
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Mark the struct with @attribute to use its values as attributes");
        return false;
    }

    for (int i = 0; i < value->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)value->initializers.items)[i];
        if (!is_constant_value(init->value)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)init->value, "The arguments of an attribute must be known when compiling");
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Use literals, enum values and constants");
            return false;
        }
    }

    // Members without an argument get their default value, which must also be known when compiling
    DynamicArray members = ((TypeStruct *)type)->node->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        AstExpr *fallback = member->decl && member->decl->values.count > 0 ? ((AstExpr **)member->decl->values.items)[0] : NULL;
        if (fallback && !is_constant_value(fallback)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)fallback, "The default values of an attribute struct must be known when compiling");
            return false;
        }
    }
    return true;
}

void push_enclosing_loop(Typer *typer, TokenType loop_type, AstFor *for_loop, AstWhile *while_loop) {
    EnclosingLoop *loop = ast_allocate(typer->parser, sizeof(EnclosingLoop));
    loop->parent = typer->enclosing_loop;
    loop->type = loop_type;
    if (loop_type == TOKEN_FOR) {
        loop->for_loop = for_loop;
    } else {
        loop->while_loop = while_loop;
    }

    typer->enclosing_loop = loop;
}

EnclosingLoop *pop_enclosing_loop(Typer *typer) {
    if (typer->enclosing_loop && typer->enclosing_loop->parent) {
        typer->enclosing_loop = typer->enclosing_loop->parent;
        return typer->enclosing_loop;
    }
    return NULL;
}

AstExpr *generate_member_of_identifier(Typer *typer, char *ident_name, char *member_name, Ast *at) {
    Parser *parser = typer->parser;

    AstLiteral *ident = ast_allocate(parser, sizeof(AstLiteral));
    ident->head.head       = *at;
    ident->head.head.kind  = AST_LITERAL;
    ident->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    ident->kind            = LITERAL_IDENTIFIER;
    ident->as.value.identifier.name   = ident_name;
    ident->as.value.identifier.length = (int)strlen(ident_name);

    AstLiteral *member = ast_allocate(parser, sizeof(AstLiteral));
    member->head.head       = *at;
    member->head.head.kind  = AST_LITERAL;
    member->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    member->kind            = LITERAL_IDENTIFIER;
    member->as.value.identifier.name   = member_name;
    member->as.value.identifier.length = (int)strlen(member_name);

    AstMemberAccess *ma = ast_allocate(parser, sizeof(AstMemberAccess));
    ma->head.head       = *at;
    ma->head.head.kind  = AST_MEMBER_ACCESS;
    ma->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    ma->left            = (AstExpr *)ident;
    ma->right           = (AstExpr *)member;
    return (AstExpr *)ma;
}

// Turns 'for key, value in m {...}' into 'for *entry in m.elements() { key := entry.key; value := entry.value; ... }'
bool desugar_key_value_for(Typer *typer, AstFor *ast_for) {
    Parser *parser = typer->parser;

    if (ast_for->is_by_pointer) {
        report_error_token(parser, LABEL_ERROR, ast_for->asterix, "Cannot loop by pointer over the keys and values of a map");
        report_error_ast(parser, LABEL_HINT, NULL, "Loop over the entries to change the values e.g for *entry in m { entry.value += 1; }");
        return false;
    }

    AstIdentifier *key   = ast_for->iterator;
    AstIdentifier *value = ast_for->index;

    char *entry_name = malloc(32);
    snprintf(entry_name, 32, "$entry%d", typer->next_hidden_local_id++);

    AstIdentifier *entry = ast_allocate(parser, sizeof(AstIdentifier));
    entry->head.kind  = AST_IDENTIFIER;
    entry->head.file  = ast_for->head.file;
    entry->head.start = ast_for->head.start;
    entry->head.end   = ast_for->head.start;
    entry->name       = entry_name;
    entry->scope      = ast_for->body;
    da_append(&ast_for->body->identifiers, entry);

    AstIdentifier *names[2] = { key, value };
    char *members[2]        = { "key", "value" };
    for (int i = 0; i < 2; i++) {
        AstDeclaration *decl = ast_allocate(parser, sizeof(AstDeclaration));
        decl->head.kind  = AST_DECLARATION;
        decl->head.file  = names[i]->head.file;
        decl->head.start = names[i]->head.start;
        decl->head.end   = names[i]->head.end;
        decl->head.flags = AST_FLAG_COMPILER_GENERATED;
        decl->idents     = da_init(1, sizeof(AstIdentifier *));
        decl->values     = da_init(1, sizeof(AstExpr *));
        decl->flags      = DECLARATION_INFER | DECLARATION_IS_COMPILER_GENERATED;

        AstExpr *member = generate_member_of_identifier(typer, entry_name, members[i], (Ast *)names[i]);
        da_append(&decl->idents, names[i]);
        da_append(&decl->values, member);
        names[i]->decl = decl;

        da_insert(&ast_for->body->statements, decl, i);
    }

    ast_for->iterator      = entry;
    ast_for->index         = NULL;
    ast_for->is_by_pointer = true; // The entries are not copied
    ast_for->kind          = FOR_WITH_NAMED_ITERATOR;
    return true;
}

bool check_for(Typer *typer, AstFor *ast_for) {
    push_enclosing_loop(typer, TOKEN_FOR, ast_for, NULL);

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

        ast_for->iterator->type = primitive_type(PRIMITIVE_I32);
        ast_for->iterator->flags |= IDENTIFIER_IS_RESOLVED;

        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_I32);
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

        // A collection is iterated through the slice its elements() method gives e.g for x in xs
        if (type->kind == TYPE_STRUCT || (type->kind == TYPE_POINTER && ((TypePointer *)type)->pointer_to->kind == TYPE_STRUCT)) {
            TypeStruct *collection = (TypeStruct *)(type->kind == TYPE_POINTER ? ((TypePointer *)type)->pointer_to : type);
            if (!find_method(collection, "elements")) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_for->iterable), "Cannot iterate through expression of type %s", type_to_str(type));
                report_error_ast(typer->parser, LABEL_HINT, NULL, "Give %s a method 'elements :: method () -> []T' to make it iterable", collection->head.name);
                return NULL;
            }

            AstFunctionCall *call = generate_method_call(typer, ast_for->iterable, "elements", NULL, 0);
            type = check_expression(typer, (AstExpr *)call, NULL);
            if (!type) return NULL;
            ast_for->iterable = (AstExpr *)call;

            // A map-like collection gives a key and a value to 'for key, value in m' instead of an element and an index
            if (ast_for->index && find_method(collection, "set") && type->kind == TYPE_ARRAY) {
                Type *entry_type = ((TypeArray *)type)->elem_type;
                if (entry_type->kind == TYPE_STRUCT && get_struct_member(((TypeStruct *)entry_type)->node, "key") && get_struct_member(((TypeStruct *)entry_type)->node, "value")) {
                    bool ok = desugar_key_value_for(typer, ast_for);
                    if (!ok) return NULL;
                }
            }
        }

        if (type->kind != TYPE_ARRAY && type->kind != TYPE_VARIADIC && type->kind != TYPE_STRING) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ast_for->iterable), "Cannot iterate through expression of type %s", type_to_str(type));
            return NULL;
        }

        if (ast_for->is_by_pointer && type->kind == TYPE_ARRAY && ((TypeArray *)type)->array_kind == ARRAY_FIXED) {
            AstFunctionDefn *readonly = readonly_self_place(typer, ast_for->iterable);
            if (readonly) {
                report_readonly_self(typer, (Ast *)ast_for->iterable, readonly, "iterate by pointer over", ast_for->iterable);
                report_error_ast(typer->parser, LABEL_NOTE, NULL, "The elements are inside self, so the pointers could be used to change self. Iterate by value instead e.g for v in ...");
                return NULL;
            }
        }

        Type *iterator_type = NULL;
        if (type->kind == TYPE_ARRAY)    iterator_type = ((TypeArray *)type)->elem_type;
        if (type->kind == TYPE_VARIADIC) iterator_type = ((TypeVariadic *)type)->type;
        if (type->kind == TYPE_STRING)   iterator_type = primitive_type(PRIMITIVE_CHAR);

        if (ast_for->is_by_pointer) {
            // Wrap the iterator in a pointer type
            TypePointer *ptr = type_alloc(&typer->parser->type_table, sizeof(TypePointer));
            ptr->head.head.kind  = AST_TYPE;
            ptr->head.head.start = ast_for->asterix.start;
            ptr->head.head.end   = ast_for->asterix.end;
            ptr->head.kind       = TYPE_POINTER;
            ptr->head.size       = 8;
            ptr->pointer_to      = iterator_type;

            iterator_type = (Type *) ptr;
        }

        ast_for->iterator->type = iterator_type;
        ast_for->iterator->flags |= IDENTIFIER_IS_RESOLVED;

        if (ast_for->index) {
            ast_for->index->type = primitive_type(PRIMITIVE_I32);
            ast_for->index->flags |= IDENTIFIER_IS_RESOLVED;
        }

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index
        assert(typer->enclosing_function != NULL);
        reserve_local_storage(typer->enclosing_function, ast_for->iterator->type->size + 24);
    }

    bool ok = check_block(typer, ast_for->body);
    if (!ok) return false;

    pop_enclosing_loop(typer);

    return true;
}

bool check_while(Typer *typer, AstWhile *ast_while) {
    push_enclosing_loop(typer, TOKEN_WHILE, NULL, ast_while);

    Type *cond_type = check_expression(typer, ast_while->condition, NULL);
    if (!cond_type) return false;

    if (cond_type->kind != TYPE_BOOL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ast_while->condition, "Type mistmatch. Expeced a bool, but got type %s", type_to_str(cond_type));
        return false;
    }

    bool ok = check_block(typer, ast_while->body);
    if (!ok) return false;

    pop_enclosing_loop(typer);

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

        if (!types_are_equal(wanted_type, return_value->type)) {
            AstExpr *slice = collection_as_slice(typer, return_value, wanted_type);
            if (slice) {
                ((AstExpr **)ast_return->values.items)[i] = slice;
                return_value = slice;
            }
        }

        AstFunctionDefn *readonly = readonly_self_method(typer, return_value);
        if (readonly && return_value->type->kind == TYPE_POINTER) {
            report_readonly_self(typer, (Ast *)return_value, readonly, "return the pointer", return_value);
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "It could be used to change self. Return a copy with *self instead");
            return false;
        }
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

        // @Incomplete: Like arguments, return values may both widen (i32 to i64) and narrow (i64 to i32), as existing code depends on narrowing
        if (!types_are_equal(given, wanted) && !types_are_equal(wanted, given)) {
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

//
// switch
//
// A switch is turned into a hidden local holding the value it switches on and a chain of ifs (AstSwitch.lowered), which is what is
// checked and lowered. So a case value can be anything that can be compared with ==
//

AstBinary *make_binary(Typer *typer, TokenType op, AstExpr *left, AstExpr *right, Ast *at) {
    AstBinary *binary       = ast_allocate(typer->parser, sizeof(AstBinary));
    binary->head.head.kind  = AST_BINARY;
    binary->head.head.file  = at->file;
    binary->head.head.start = at->start;
    binary->head.head.end   = at->end;
    binary->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    binary->operator        = op;
    binary->left            = left;
    binary->right           = right;
    return binary;
}

// The condition of one value of a case
AstExpr *case_condition(Typer *typer, AstSwitch *sw, AstIdentifier *subject, AstExpr *value) {
    Ast *at = (Ast *)value;

    // case 1..10 or case 1...10
    if (value->head.kind == AST_RANGE_EXPR) {
        AstRangeExpr *range = (AstRangeExpr *)value;
        AstExpr *low  = (AstExpr *)make_binary(typer, TOKEN_GREATER_EQUAL, (AstExpr *)make_resolved_identifier_literal(typer, subject, at), range->start, at);
        AstExpr *high = (AstExpr *)make_binary(typer, range->inclusive ? TOKEN_LESS_EQUAL : '<', (AstExpr *)make_resolved_identifier_literal(typer, subject, at), range->end, at);
        return (AstExpr *)make_binary(typer, TOKEN_LOGICAL_AND, low, high, at);
    }

    (void)sw;
    return (AstExpr *)make_binary(typer, TOKEN_DOUBLE_EQUAL, (AstExpr *)make_resolved_identifier_literal(typer, subject, at), value, at);
}

// subject.member, for the members the compiler reads itself e.g the tag of a union
AstExpr *member_of_local(Typer *typer, AstIdentifier *local, char *member_name, Ast *at) {
    AstLiteral *member = ast_allocate(typer->parser, sizeof(AstLiteral));
    member->head.head       = *at;
    member->head.head.kind  = AST_LITERAL;
    member->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    member->kind            = LITERAL_IDENTIFIER;
    member->as.value.identifier.name   = member_name;
    member->as.value.identifier.length = (int)strlen(member_name);

    AstMemberAccess *ma = ast_allocate(typer->parser, sizeof(AstMemberAccess));
    ma->head.head       = *at;
    ma->head.head.kind  = AST_MEMBER_ACCESS;
    ma->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    ma->left            = (AstExpr *)make_resolved_identifier_literal(typer, local, at);
    ma->right           = (AstExpr *)member;
    return (AstExpr *)ma;
}

// The variant of a case of a switch on a union e.g .circle
AstIdentifier *case_variant(Typer *typer, TypeStruct *union_type, AstExpr *value) {
    if (value->head.kind != AST_ENUM_LITERAL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)value, "A case of a switch on a union is a variant of the union e.g .%s", ((AstIdentifier **)union_type->node->members->identifiers.items)[1]->name);
        return NULL;
    }
    char *name = ((AstEnumLiteral *)value)->identifier->name;
    AstIdentifier *variant = union_variant(union_type, name);
    if (variant == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)value, "'%s' is not a variant of %s", name, type_to_str((Type *)union_type));
        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)union_type->node, "Here is the definition of %s", type_to_str((Type *)union_type));
        return NULL;
    }
    return variant;
}

// Declares the name of 'switch v in shape' at the start of a case: the value of the variant, or a pointer to it when switching on a pointer
bool declare_case_binding(Typer *typer, AstSwitch *sw, AstCase *c, AstIdentifier *subject, AstIdentifier *variant, bool by_pointer) {
    Parser *parser = typer->parser;
    Ast    *at     = (Ast *)c;

    AstExpr *value = NULL;
    if (variant == NULL) {
        value = (AstExpr *)make_resolved_identifier_literal(typer, subject, at); // More than one variant: the union itself
    } else {
        value = member_of_local(typer, subject, variant->name, at);
        if (by_pointer) {
            AstUnary *address = ast_allocate(parser, sizeof(AstUnary));
            address->head.head       = value->head;
            address->head.head.kind  = AST_UNARY;
            address->head.head.flags = AST_FLAG_COMPILER_GENERATED;
            address->operator        = OP_ADDRESS_OF;
            address->expr            = value;
            value = (AstExpr *)address;
        }
    }

    AstIdentifier *binding = make_identifier_from_string(parser, sw->binding.as_value.value.identifier.name, NULL);
    binding->head.file  = sw->head.head.file;
    binding->head.start = sw->binding.start;
    binding->head.end   = sw->binding.end;
    binding->scope      = c->block;

    AstDeclaration *decl = ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.kind  = AST_DECLARATION;
    decl->head.file  = sw->head.head.file;
    decl->head.start = c->head.start;
    decl->head.end   = c->head.start;
    decl->head.flags = AST_FLAG_COMPILER_GENERATED;
    decl->idents     = da_init(1, sizeof(AstIdentifier *));
    decl->values     = da_init(1, sizeof(AstExpr *));
    decl->flags      = DECLARATION_INFER | DECLARATION_IS_COMPILER_GENERATED;
    da_append(&decl->idents, binding);
    da_append(&decl->values, value);
    binding->decl = decl;

    if (find_identifier_in_scope(c->block, binding->name)) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)find_identifier_in_scope(c->block, binding->name), "'%s' is already the name of the value of the switch", binding->name);
        return false;
    }
    da_append(&c->block->identifiers, binding);
    da_insert(&c->block->statements, decl, 0);
    return true;
}

// The value of a case value that is a constant enum value or integer, for finding values that are handled twice
bool case_constant(AstExpr *value, int64_t *out) {
    if (value->head.kind == AST_ENUM_LITERAL) {
        AstEnumLiteral *lit = (AstEnumLiteral *)value;
        if (lit->enum_member == NULL) return false;
        *out = lit->enum_member->value;
        return true;
    }
    if (value->head.kind == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)value;
        if (ma->access_kind != MEMBER_ACCESS_ENUM_MEMBER || ma->enum_member == NULL || ma->enum_member->head.kind == AST_IDENTIFIER) return false;
        *out = ma->enum_member->value;
        return true;
    }
    if (value->head.kind == AST_LITERAL && ((AstLiteral *)value)->kind == LITERAL_INTEGER) {
        *out = ((AstLiteral *)value)->as.value.integer;
        return true;
    }
    if (value->head.kind == AST_UNARY && ((AstUnary *)value)->operator == OP_UNARY_MINUS && case_constant(((AstUnary *)value)->expr, out)) {
        *out = -*out;
        return true;
    }
    return false;
}

// switch v in shape { case .circle { ... } }: each case tests the tag, and declares v as the value of its variant
bool check_union_switch(Typer *typer, AstSwitch *sw, AstIdentifier *subject, TypeStruct *union_type, bool by_pointer, AstBlock *lowered, AstBlock *scope_before) {
    Parser *parser = typer->parser;
    DynamicArray variants = union_type->node->members->identifiers;
    bool *handled = calloc(variants.count, sizeof(bool));

    AstIf *first = NULL;
    for (int i = 0; i < sw->cases.count; i++) {
        AstCase *c = ((AstCase **)sw->cases.items)[i];
        if (c->is_else) {
            if (first) first->else_block = c->block;
            else       da_append(&lowered->statements, c->block);
            break;
        }

        AstExpr       *condition = NULL;
        AstIdentifier *only      = NULL;
        for (int j = 0; j < c->values.count; j++) {
            AstExpr *value = ((AstExpr **)c->values.items)[j];
            AstIdentifier *variant = case_variant(typer, union_type, value);
            if (!variant) { typer->current_scope = scope_before; return false; }

            if (handled[variant->member_index]) {
                report_error_ast(parser, LABEL_ERROR, (Ast *)value, "The variant '%s' is handled by an earlier case of the switch", variant->name);
                typer->current_scope = scope_before;
                return false;
            }
            handled[variant->member_index] = true;
            only = variant;

            AstExpr *tag  = union_kind_value(typer, variant, (Ast *)value);
            AstExpr *test = (AstExpr *)make_binary(typer, TOKEN_DOUBLE_EQUAL, member_of_local(typer, subject, "$tag", (Ast *)value), tag, (Ast *)value);
            condition = condition ? (AstExpr *)make_binary(typer, TOKEN_LOGICAL_OR, condition, test, (Ast *)value) : test;
        }

        if (sw->binding.type != TOKEN_NONE) {
            AstIdentifier *variant = c->values.count == 1 ? only : NULL;
            if (!(variant && variant->type->kind == TYPE_VOID)) {
                if (!declare_case_binding(typer, sw, c, subject, variant, by_pointer)) { typer->current_scope = scope_before; return false; }
            }
        }

        AstIf *ast_if      = ast_allocate(parser, sizeof(AstIf));
        ast_if->head.kind  = AST_IF;
        ast_if->head.file  = c->head.file;
        ast_if->head.start = c->head.start;
        ast_if->head.end   = c->head.end;
        ast_if->head.flags = AST_FLAG_COMPILER_GENERATED;
        ast_if->condition  = condition;
        ast_if->then_block = c->block;
        ast_if->else_ifs   = da_init(1, sizeof(AstIf *));

        if (first == NULL) {
            first = ast_if;
            da_append(&lowered->statements, first);
        } else {
            da_append(&first->else_ifs, ast_if);
        }
    }

    bool ok = check_block(typer, lowered);
    typer->current_scope = scope_before;
    if (!ok) return false;

    bool has_else = sw->cases.count > 0 && ((AstCase **)sw->cases.items)[sw->cases.count - 1]->is_else;
    if (!has_else) {
        StringBuilder missing = sb_init(32);
        int missing_count = 0;
        for (int i = 1; i < variants.count; i++) {
            if (handled[i]) continue;
            if (missing_count > 0) sb_append(&missing, ", ");
            sb_append(&missing, ".%s", ((AstIdentifier **)variants.items)[i]->name);
            missing_count++;
        }
        if (missing_count > 0) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)sw->subject, "The switch doesn't handle %s of %s", sb_to_string(&missing), type_to_str((Type *)union_type));
            report_error_ast(parser, LABEL_HINT, NULL, "Add a case for %s, or an else", missing_count == 1 ? "it" : "them");
            return false;
        }
    }
    free(handled);
    return true;
}

// choice := switch x { case 1 => "icecream"; else => "mango"; }. The switch gives the type that is wanted, or the type of its first case
Type *check_switch_expression(Typer *typer, AstSwitch *sw, Type *ctx_type) {
    if (sw->head.head.flags & AST_FLAG_IS_TYPE_CHECKED) return sw->result_type;

    if (ctx_type && ctx_type->kind != TYPE_ANY && !is_untyped_type(ctx_type)) {
        sw->result_type = ctx_type;
    }
    if (!check_switch(typer, sw)) return NULL;

    sw->head.head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    sw->head.type = sw->result_type;
    return sw->result_type;
}

// The value of a case of a switch expression has the type of the switch
bool check_switch_arm(Typer *typer, AstSwitchArm *arm) {
    AstSwitch *sw = arm->owner;

    Type *type = check_expression(typer, arm->value, sw->result_type);
    if (!type) return false;

    if (sw->result_type == NULL) {
        // The first case decides, like 'x := value;'
        if (is_untyped_type(type) && type->kind == TYPE_INTEGER)    type = primitive_type(PRIMITIVE_INT);
        else if (is_untyped_type(type) && type->kind == TYPE_FLOAT) type = primitive_type(PRIMITIVE_FLOAT);
        if (type->kind == TYPE_VOID) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arm->value, "The case of a switch expression must give a value");
            return false;
        }
        sw->result_type = type;
        return true;
    }

    if (!leads_to_integer_overflow(typer, sw->result_type, arm->value)) return false;

    if (!types_are_equal(sw->result_type, arm->value->type)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arm->value, "This case gives a %s, but the switch gives a %s", type_to_str(arm->value->type), type_to_str(sw->result_type));
        return false;
    }
    return true;
}

bool check_switch(Typer *typer, AstSwitch *sw) {
    Parser *parser = typer->parser;

    for (int i = 0; i < sw->cases.count; i++) {
        AstCase *c = ((AstCase **)sw->cases.items)[i];
        if (c->is_else && i != sw->cases.count - 1) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)c, "The else of a switch must be its last case");
            return false;
        }
    }

    AstBlock *scope_before = typer->current_scope;
    parser->current_scope  = typer->current_scope;
    AstBlock *lowered      = new_block(parser, BLOCK_IMPERATIVE);
    parser->current_scope  = scope_before;
    lowered->head.file          = sw->head.head.file;
    lowered->head.start         = sw->head.head.start;
    lowered->head.end           = sw->head.head.end;
    lowered->enclosing_function = scope_before->enclosing_function;
    sw->lowered = lowered;
    typer->current_scope = lowered;

    // The value is evaluated once, into a hidden local
    AstIdentifier *subject = NULL;
    if (sw->subject) {
        char *name = malloc(32);
        snprintf(name, 32, "$switch%d", typer->next_hidden_local_id++);

        subject = ast_allocate(parser, sizeof(AstIdentifier));
        subject->head.kind  = AST_IDENTIFIER;
        subject->head.file  = sw->head.head.file;
        subject->head.start = sw->subject->head.start;
        subject->head.end   = sw->subject->head.end;
        subject->name       = name;
        subject->scope      = lowered;

        AstDeclaration *decl = ast_allocate(parser, sizeof(AstDeclaration));
        decl->head.kind  = AST_DECLARATION;
        decl->head.file  = sw->head.head.file;
        decl->head.start = sw->subject->head.start;
        decl->head.end   = sw->subject->head.end;
        decl->head.flags = AST_FLAG_COMPILER_GENERATED;
        decl->idents     = da_init(1, sizeof(AstIdentifier *));
        decl->values     = da_init(1, sizeof(AstExpr *));
        decl->flags      = DECLARATION_INFER | DECLARATION_IS_COMPILER_GENERATED;
        da_append(&decl->idents, subject);
        da_append(&decl->values, sw->subject);
        subject->decl = decl;

        da_append(&lowered->identifiers, subject);
        da_append(&lowered->statements, decl);
        if (!check_statement(typer, (Ast *)decl)) return false;
    }

    // A switch on a union, or a pointer to one
    TypeStruct *union_type = NULL;
    bool        by_pointer = false;
    if (subject) {
        Type *type = subject->type;
        if (type->kind == TYPE_POINTER) {
            type = ((TypePointer *)type)->pointer_to;
            by_pointer = true;
        }
        if (type->kind == TYPE_STRUCT && ((TypeStruct *)type)->node->is_union) {
            union_type = (TypeStruct *)type;
        } else {
            by_pointer = false;
        }
    }

    if (union_type) {
        return check_union_switch(typer, sw, subject, union_type, by_pointer, lowered, scope_before);
    }

    if (sw->binding.type != TOKEN_NONE) {
        report_error_token(parser, LABEL_ERROR, sw->binding, "Only a switch on a union can give its value a name");
        report_error_ast(parser, LABEL_HINT, NULL, "e.g switch v in shape { case .circle { ... } }");
        return false;
    }

    // The chain of ifs
    AstIf *first = NULL;
    for (int i = 0; i < sw->cases.count; i++) {
        AstCase *c = ((AstCase **)sw->cases.items)[i];
        if (c->is_else) {
            if (first) first->else_block = c->block;
            else       da_append(&lowered->statements, c->block);
            break;
        }

        AstExpr *condition = NULL;
        for (int j = 0; j < c->values.count; j++) {
            AstExpr *value = ((AstExpr **)c->values.items)[j];
            AstExpr *test  = case_condition(typer, sw, subject, value);
            if (!test) return false;
            condition = condition ? (AstExpr *)make_binary(typer, TOKEN_LOGICAL_OR, condition, test, (Ast *)value) : test;
        }

        AstIf *ast_if      = ast_allocate(parser, sizeof(AstIf));
        ast_if->head.kind  = AST_IF;
        ast_if->head.file  = c->head.file;
        ast_if->head.start = c->head.start;
        ast_if->head.end   = c->head.end;
        ast_if->head.flags = AST_FLAG_COMPILER_GENERATED;
        ast_if->condition  = condition;
        ast_if->then_block = c->block;
        ast_if->else_ifs   = da_init(1, sizeof(AstIf *));

        if (first == NULL) {
            first = ast_if;
            da_append(&lowered->statements, first);
        } else {
            da_append(&first->else_ifs, ast_if);
        }
    }

    bool ok = check_block(typer, lowered);
    typer->current_scope = scope_before;
    if (!ok) return false;

    // Values that are handled twice, and the values of an enum that are not handled
    bool has_else = sw->cases.count > 0 && ((AstCase **)sw->cases.items)[sw->cases.count - 1]->is_else;
    DynamicArray seen = da_init(8, sizeof(int64_t));
    for (int i = 0; i < sw->cases.count; i++) {
        AstCase *c = ((AstCase **)sw->cases.items)[i];
        for (int j = 0; j < c->values.count && subject; j++) {
            AstExpr *value = ((AstExpr **)c->values.items)[j];
            int64_t constant;
            if (!case_constant(value, &constant)) continue;
            for (int k = 0; k < seen.count; k++) {
                if (((int64_t *)seen.items)[k] == constant) {
                    report_error_ast(parser, LABEL_ERROR, (Ast *)value, "This value is handled by an earlier case of the switch");
                    return false;
                }
            }
            da_append(&seen, constant);
        }
    }

    if (sw->is_expression && !has_else && !(subject && subject->type->kind == TYPE_ENUM)) {
        report_error_ast(parser, LABEL_ERROR, (Ast *)sw, "A switch that gives a value needs an else, so that it has a value when no case matches");
        return false;
    }

    if (subject && subject->type->kind == TYPE_ENUM && !has_else) {
        AstEnum *node = ((TypeEnum *)subject->type)->node;
        StringBuilder missing = sb_init(32);
        int missing_count = 0;
        for (int i = 0; i < node->enumerators.count; i++) {
            AstEnumerator *etor = ((AstEnumerator **)node->enumerators.items)[i];
            bool handled = false;
            for (int k = 0; k < seen.count; k++) {
                if (((int64_t *)seen.items)[k] == etor->value) handled = true;
            }
            if (handled) continue;
            if (missing_count > 0) sb_append(&missing, ", ");
            sb_append(&missing, ".%s", etor->name);
            missing_count++;
        }
        if (missing_count > 0) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)sw->subject, "The switch doesn't handle %s of %s", sb_to_string(&missing), type_to_str(subject->type));
            report_error_ast(parser, LABEL_HINT, NULL, "Add a case for %s, or an else", missing_count == 1 ? "it" : "them");
            return false;
        }
    }
    da_free(&seen);
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
        AstIf *else_if = da_get_deref(ast_if->else_ifs, i);
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
    case AST_STRUCT:            return check_struct_statement(typer, (AstStruct *)(stmt));
    case AST_ENUM:              return check_enum_defn(typer, (AstEnum *)stmt);
    case AST_FOR:               return check_for(typer, (AstFor *)(stmt));
    case AST_WHILE:             return check_while(typer, (AstWhile *)(stmt));
    case AST_BREAK_OR_CONTINUE: return check_break_or_continue(typer, (AstBreakOrContinue *)(stmt));
    case AST_EXPR_STMT:         return check_expr_stmt(typer, (AstExprStmt *)(stmt));
    case AST_RETURN:            return check_return(typer, (AstReturn *)(stmt));
    case AST_ASSERT:            return check_assert(typer, (AstAssert *)(stmt));
    case AST_IF:                return check_if(typer, (AstIf *)(stmt));
    case AST_SWITCH:            return check_switch(typer, (AstSwitch *)(stmt));
    case AST_SWITCH_ARM:        return check_switch_arm(typer, (AstSwitchArm *)(stmt));
    default:
        XXX;
    }
}

bool check_break_or_continue(Typer *typer, AstBreakOrContinue *boc) {
    if (!typer->enclosing_loop) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(boc), "%s is only allowed inside for-loops or while-loops", boc->token.type ==TOKEN_BREAK ? "break" : "continue");
        return false;
    }

    boc->enclosing_loop = typer->enclosing_loop;

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

    // A collection is passed as its elements e.g to fmt.print("%", xs)
    AstExpr *slice = collection_as_slice(typer, expr, NULL);
    if (slice) expr = slice;

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

int compare_argument_index(const void *e1, const void *e2) {
    AstArgument *a = *((AstArgument **)e1);
    AstArgument *b = *((AstArgument **)e2);

    if (a->param_index == b->param_index) {
        return a->vararg_index - b->vararg_index;
    }

    return a->param_index - b->param_index;
}

int compare_struct_initializers(const void *e1, const void *e2) {
    AstStructInitializer *a = *((AstStructInitializer **)e1);
    AstStructInitializer *b = *((AstStructInitializer **)e2);

    return a->member->member_index - b->member->member_index;
}

AstExpr *generate_literal_bool(Parser *parser, bool value) {
    AstLiteral *lit          = ast_allocate(parser, sizeof(AstLiteral));
    lit->head.head.kind      = AST_LITERAL;
    lit->head.head.file      = parser->current_file;
    lit->head.head.flags     = AST_FLAG_COMPILER_GENERATED;
    lit->head.type           = primitive_type(PRIMITIVE_BOOL);
    lit->kind                = LITERAL_BOOLEAN;
    lit->as.value.boolean    = value;
    return (AstExpr *)(lit);
}

AstArgument *generate_argument_from_expr(Typer *typer, AstExpr *expr) {
    AstArgument *arg = ast_allocate(typer->parser, sizeof(AstArgument));
    arg->head.kind   = AST_ARGUMENT;
    arg->head.file   = expr->head.file;
    arg->head.start  = expr->head.start;
    arg->head.end    = expr->head.end;
    arg->head.flags |= AST_FLAG_COMPILER_GENERATED;
    arg->value       = expr;

    return arg;
}

AstUnary *generate_address_of_value(Typer *typer, AstExpr *value) {
    AstUnary *addr_of        = ast_allocate(typer->parser, sizeof(AstUnary));
    addr_of->head.head       = value->head;
    addr_of->head.head.kind  = AST_UNARY;
    addr_of->head.head.flags = value->head.flags | AST_FLAG_COMPILER_GENERATED;
    addr_of->head.type       = (Type *) generate_pointer_to_type(typer->parser, value->type);
    addr_of->operator        = OP_ADDRESS_OF;
    addr_of->expr            = value;
    return addr_of;
}

bool rewrite_as_method_call(Typer *typer, AstFunctionCall *func_call, AstMemberAccess *ma) {
    AstExpr       *recv_arg = ma->left;
    AstIdentifier *recv_param = func_call->func_defn->receiver;

    // @Note: In this block, we don't have full type information on the parameters, so we can only check shallowly if the receiver arg and param matches.

    if ((recv_arg->type->kind == TYPE_POINTER && recv_param->type->kind == TYPE_POINTER) || (recv_arg->type->kind != TYPE_POINTER && recv_param->type->kind != TYPE_POINTER)) {
        // Pointer vs Pointer
        // Value   vs Value
        AstArgument *arg = generate_argument_from_expr(typer, (AstExpr *) ma->left);
        da_insert(&func_call->arguments, arg, 0);
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

        AstArgument *arg = generate_argument_from_expr(typer, (AstExpr *)deref);
        da_insert(&func_call->arguments, arg, 0);
        return true;
    }

    if (recv_arg->type->kind != TYPE_POINTER && recv_param->type->kind == TYPE_POINTER) {
        // Insert an address-of to match
        AstUnary *addr_of = generate_address_of_value(typer, recv_arg);
        Type *type = check_unary(typer, addr_of, recv_param->type);
        if (!type) return false;

        AstArgument *arg = generate_argument_from_expr(typer, (AstExpr *)addr_of);
        da_insert(&func_call->arguments, arg, 0);
        return true;
    }

    // Let the check_function_call deal with the actual type mismatch
    return true;
}


// xs.sort() on a slice, a fixed array or a collection (a struct with elements()) is the call sort(xs) of the method on slices,
// unless the type has a member or method of that name itself
bool rewrite_slice_method_call(Typer *typer, AstFunctionCall *call) {
    AstMemberAccess *ma = (AstMemberAccess *)call->expression;
    if (!is_literal_identifier(ma->right)) return true;

    char *name = ((AstLiteral *)ma->right)->as.value.identifier.name;
    AstFunctionDefn *method = find_slice_method(typer, name);
    if (method == NULL) return true;

    Type *type = check_expression(typer, ma->left, NULL);
    if (!type) return false;

    if (type->kind == TYPE_STRUCT) {
        AstStruct *node = ((TypeStruct *)type)->node;
        if (get_struct_member(node, name)) return true;
        if (get_struct_method(node, name)) return true;
        if (node->generic_struct && get_struct_method(node->generic_struct, name)) return true;

        bool is_collection = get_struct_method(node, "elements") || (node->generic_struct && get_struct_method(node->generic_struct, "elements"));
        if (!is_collection) return true;
    }
    else if (type->kind != TYPE_ARRAY) {
        return true;
    }

    call->expression = (AstExpr *)make_resolved_identifier_literal(typer, method->identifier, (Ast *)ma->right);
    AstArgument *receiver = generate_argument_from_expr(typer, ma->left);
    da_insert(&call->arguments, receiver, 0);
    return true;
}

Type *check_function_call(Typer *typer, AstFunctionCall *call) {
    if (typer->enclosing_function == NULL) {
        if (typer->checking_member_default) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "The default value of a struct member cannot call a function yet");
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Use a constant, a literal or an expression of those");
            return NULL;
        }
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call), "Function calls are not allowed at top-level yet");
        return NULL;
    }

    if (call->head.head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return call->head.type;
    }

    // A qualified function e.g math.sin(x) is called like any other function
    if (call->expression->head.kind == AST_MEMBER_ACCESS) {
        bool failed = false;
        AstIdentifier *qualified = resolve_qualified_name(typer, call->expression, &failed);
        if (failed) return NULL;
        if (qualified) call->expression = (AstExpr *)make_resolved_identifier_literal(typer, qualified, (Ast *)call->expression);
    }

    if (call->expression->head.kind == AST_MEMBER_ACCESS) {
        if (!rewrite_slice_method_call(typer, call)) return NULL;
    }

    typer->checking_callee = is_literal_identifier(call->expression);
    Type *expr_type = check_expression(typer, call->expression, NULL);
    typer->checking_callee = false;
    if (!expr_type) return NULL;

    if (call->expression->type->kind != TYPE_FUNCTION) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->expression), "Expression is not a callable function. Type of expression is %s", type_to_str(call->expression->type));
        return NULL;
    }

    AstFunctionDefn *func_defn = ((TypeFunction *)call->expression->type)->node;

    call->func_defn = func_defn;

    // Handle member calls
    if (call->expression->head.kind == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)call->expression;

        if (func_defn->is_method) {
            // We are indeed a method call
            call->is_method_call = true;

            if (func_defn->identifier) {
                if (!report_if_not_visible(typer, func_defn->identifier, (Ast *)ma->right)) return NULL;
                report_if_deprecated(typer, func_defn->identifier, (Ast *)ma->right);
            }

            // A method that can change its receiver, called on an element of a collection e.g groups["a"].add(1), changes the element
            // in the collection. So the element is a place (indexRef), not a copy of it (index)
            if (ma->left->head.kind == AST_ARRAY_ACCESS && ma->left->replacement && method_can_change_receiver(func_defn)) {
                AstArrayAccess *element = (AstArrayAccess *)ma->left;
                if (element->head.replacement->head.kind == AST_FUNCTION_CALL) {
                    Type *collection = element->left->type;
                    if (!check_collection_index(typer, element, (TypeStruct *)collection, true)) return NULL;
                    element->head.type = element->head.replacement->type;
                }
            }

            AstFunctionDefn *readonly = readonly_self_method(typer, ma->left);
            if (!readonly) readonly = readonly_self_place(typer, ma->left);
            if (readonly && method_can_change_receiver(func_defn)) {
                StringBuilder sb = sb_init(32);
                sb_append(&sb, "call the method '%s', which can change its receiver, on", func_defn->identifier->name);
                report_readonly_self(typer, (Ast *)ma, readonly, sb_to_string(&sb), ma->left);
                return NULL;
            }

            // Rewrite the method call into a plain function call, with the 0'th
            // argument being the left-hand side of the member access
            // e.g
            // `foo.bar(x)` -> `bar(foo, x)`
            bool ok = rewrite_as_method_call(typer, call, ma);
            if (!ok) return NULL;
        }
        else {
            // No, just a regular member call
            call->is_member_call = true;
        }
    }

    // if (call->is_member_call) {
    //     assert(call->belongs_to_struct);

    //     AstIdentifier *func_member = get_struct_member(call->belongs_to_struct, call->identifer->name);
    //     if (!func_member) {
    //         report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined member function '%s'", text_bold(call->identifer->name));
    //         return NULL;
    //     }

    //     func_ident = func_member;
    // }
    // else if (call->is_method_call) {
    //     assert(call->belongs_to_struct);

    //     AstIdentifier *method = get_struct_method(call->belongs_to_struct, call->identifer->name);
    //     if (!method) {
    //         report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined method '%s'", text_bold(call->identifer->name));
    //         return NULL;
    //     }

    //     func_ident = method;
    // } else {
    //     // Look in the current local scope
    //     func_ident = lookup_from_scope(typer->parser, typer->current_scope, call->identifer->name);

    //     if (func_ident == NULL) {
    //         report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined function '%s'", text_bold(call->identifer->name));
    //         return NULL;
    //     }

    //     if (!is_non_constant_identifier_found_inside_current_function(typer, func_ident)) {
    //         report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(call->identifer), "Undefined function '%s'", text_bold(call->identifer->name));
    //         return NULL;
    //     }
    // }

    // if (func_ident->decl && !(func_ident->decl->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
    //     report_error_ast(typer->parser, LABEL_ERROR, (Ast *) func_ident, "Illegal shadowing. Declaration of %s shadows the name of function %s(...)", func_ident->name, func_ident->name);
    //     report_error_ast(typer->parser, LABEL_HINT, (Ast *) func_ident, "Use a name other than %s to declare the identifier", func_ident->name);
    //     return NULL;
    // }


    if (is_generic_function(func_defn)) {
        AstFunctionDefn *instance = instantiate_generic_function_call(typer, call, func_defn);
        if (!instance) return NULL;

        func_defn              = instance;
        call->func_defn        = instance;
        call->expression->type = instance->head.type;
    }

    // Make sure that the function definition is fully resolved
    if (!(func_defn->head.head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
        Typer temp = *typer;
        
        // @Incomplete: I don't think this resetting of the function stack is valid when we now have the possibility
        // of making inner functions
        temp.enclosing_function_stack = da_init(2, sizeof(AstFunctionDefn *));

        Type *ok = check_function_defn(typer, func_defn);
        if (!ok) return NULL;

        typer_restore_state(typer, temp);
    }

    // @Cleanup: Set a bool in the parser and just set a flag on the function call node
    bool has_spreaded_vararg = false;
    int spreaded_arg_index     = -1;
    for (int i = 0; i < call->arguments.count; i++) {
        AstArgument *arg   = ((AstArgument **)(call->arguments.items))[i];
        if (arg->value->head.kind == AST_UNARY && ((AstUnary *)arg->value)->operator == OP_SPREAD) {
            has_spreaded_vararg = true;
            spreaded_arg_index = i;
        }
    }

    //
    // Resolve argument positions
    //
    for (int arg_index = 0; arg_index < call->arguments.count; arg_index++) {
        AstArgument *arg = ((AstArgument **)call->arguments.items)[arg_index];

        // Non-positional arguments
        if (!arg->ident) {
            if (func_defn->is_variadic && arg_index >= func_defn->variadic_parameter_index) {
                arg->param_index = func_defn->variadic_parameter_index;
                arg->is_vararg = true;
                arg->vararg_index = arg_index;
            } else {
                arg->param_index = arg_index;
            }
            continue;
        }

        // Positional arguments
        bool matched_parameter = false;
        for (int param_index = 0; param_index < func_defn->parameters.count; param_index++) {
            AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[param_index];
            
            if (strcmp(arg->ident->name, param->name) == 0) {

                for (int k = 0; k < arg_index; k++) {
                    AstArgument *prev_arg = ((AstArgument **)call->arguments.items)[k];
                    if (!prev_arg->ident) continue;
                    if (prev_arg->param_index == param_index) {
                        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "Positional argument '%s' is specified twice", arg->ident->name);
                        report_error_ast(typer->parser, LABEL_NOTE, (Ast *)prev_arg, "Here is the first occurence");
                        return NULL;
                    }
                }

                if (param_index < arg_index) {
                    if (func_defn->is_variadic) {
                        // Skip the check
                    } else {
                        AstArgument *prev_arg = ((AstArgument **)call->arguments.items)[param_index];
                        if (!prev_arg->ident) {
                            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg, "Positional argument '%s' is already given", arg->ident->name);
                            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)prev_arg, "Here");
                            return NULL;
                        }
                    }
                }

                arg->param_index = param_index;
                matched_parameter = true;
                break;
            }
        }
        if (!matched_parameter) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)arg->ident, "Unknown argument '%s'", arg->ident->name);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(func_defn->identifier), "Here is the definition of %s", func_defn->identifier->name);
            return NULL;
        }
    }

    //
    // Check that all the required parameters are given
    //
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];

        // Skip default values
        if (param->value) continue;

        // Skip variadic arguments which is optional
        if (func_defn->is_variadic && i >= func_defn->variadic_parameter_index) continue;

        bool is_given = false;
        for (int j = 0; j < call->arguments.count; j++) {
            AstArgument *arg = ((AstArgument **)(call->arguments.items))[j];
            if (arg->param_index == i) {
                is_given = true;
                break;
            }
        }

        if (!is_given) {
            report_error_token(typer->parser, LABEL_ERROR, call->paren_start_token, "Missing required parameter '%s' of type %s", param->name, text_bold(type_to_str(param->type)));
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)param->decl, "", param->name);
            return NULL;
        }
    }

    //
    // Fillout default parameter values if any
    //
    if (func_defn->has_default_parameters) {
        for (int param_index = 0; param_index < func_defn->parameters.count; param_index++) {
            AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[param_index];

            // Skip required parameters
            if (!param->value) continue;


            bool add_default_arg = false;

            if (!func_defn->is_variadic) {

                // Search for an argument that is providing this default value
                add_default_arg = true;

                for (int j = 0; j < call->arguments.count; j++) {
                    AstArgument *arg = ((AstArgument **)call->arguments.items)[j];
                    if (arg->param_index == param_index) {
                        add_default_arg = false;
                        break;
                    }
                }
            } else {

                if (!call->has_positional_arguments) {
                    // If we haven't provided any positional arguments then we know that we haven't provided any
                    // default arguments
                    add_default_arg = true;
                } else {

                    // Search for a positional argument after the varargs that is providing the default value
                    add_default_arg = true;

                    for (int j = call->first_positional_argument_index; j < call->arguments.count; j++) {
                        AstArgument *arg = ((AstArgument **)call->arguments.items)[j];
                        if (arg->param_index == param_index) {
                            add_default_arg = false;
                            break;
                        }
                    }
                }
            }

            if (add_default_arg) {
                AstArgument *default_arg = ast_allocate(typer->parser, sizeof(AstArgument));
                default_arg->head.kind   = AST_ARGUMENT;
                default_arg->head.file   = call->head.head.file;
                default_arg->head.start  = call->head.head.end;
                default_arg->head.end    = call->head.head.end;
                default_arg->value       = param->value;
                default_arg->param_index = param_index;
                
                da_append(&call->arguments, default_arg);
            }
            
        }
    }

    // Sort the argument list if it has any positional arguments
    if (call->has_positional_arguments) {
        qsort(call->arguments.items, call->arguments.count, sizeof(AstArgument *), compare_argument_index);
    }

    //
    // Typecheck the arguments against the parameter list
    //
    if (func_defn->is_variadic) {

        // Typecheck the arguments up until the varargs arg begins
        AstIdentifier *var_param = ((AstIdentifier **)func_defn->parameters.items)[func_defn->variadic_parameter_index];
        assert(var_param->type->kind == TYPE_VARIADIC);
        TypeVariadic *vararg_type = (TypeVariadic *) var_param->type;

        for (int i = 0; i < call->arguments.count; i++) {
            AstArgument   *arg = ((AstArgument **)(call->arguments.items))[i];
            AstIdentifier *param = ((AstIdentifier **)(func_defn->parameters.items))[arg->param_index];

            Type *wanted_type = param->type;

            // A vararg is checked against the type of the variadic so that untyped literals take that type e.g 100 in f(a, 100)
            Type *ctx_type = wanted_type;
            bool  is_spread = arg->value->head.kind == AST_UNARY && ((AstUnary *)arg->value)->operator == OP_SPREAD;
            if (wanted_type->kind == TYPE_VARIADIC && !is_spread) {
                ctx_type = ((TypeVariadic *)wanted_type)->type;
            }

            Type *given_type = check_expression(typer, arg->value, ctx_type);
            if (!given_type) return NULL;

            if (!types_are_equal(given_type, ctx_type) && ctx_type->kind != TYPE_ANY) {
                AstExpr *slice = collection_as_slice(typer, arg->value, ctx_type);
                if (slice) {
                    arg->value = slice;
                    given_type = slice->type;
                }
            }

            if (wanted_type->kind == TYPE_VARIADIC) {
                // Unwrap the variadic to match against its type
                wanted_type = ((TypeVariadic *)wanted_type)->type;
            }
            
            // Match a spreaded vararg or slice against their inner type
            if (arg->value->head.flags & AST_FLAG_EXPR_IS_SPREADED) {
                if (arg->value->type->kind == TYPE_VARIADIC) {
                    TypeVariadic *variadic = (TypeVariadic *) arg->value->type;
                    given_type = variadic->type;
                } else if (arg->value->type->kind == TYPE_ARRAY) {
                    TypeArray *array = (TypeArray *) arg->value->type;
                    given_type = array->elem_type;
                }
            }

            if (arg->value->type->kind != TYPE_ANY && wanted_type->kind == TYPE_ANY) {
                if (!func_defn->is_extern) {
                    if (arg->value->type->kind != TYPE_VARIADIC) {
                        if (arg->value->type->kind == TYPE_ARRAY && ((TypeArray *)arg->value->type)->elem_type->kind == TYPE_ANY) {
                            // @Incomplete: This should probably check for nested arrays too
                            // Skip
                        } else {
                            AstCast *any_cast = generate_cast_to_any(typer, arg->value);
                            if (!any_cast) return NULL;
                            arg->value = (AstExpr *) any_cast;
                            given_type = any_cast->cast_to;
                        }
                    }
                } else {
                    // Let the given type be coerced into the type any
                    given_type = wanted_type;
                }
            }

            if (!types_are_equal(wanted_type, given_type) && !types_are_equal(given_type, wanted_type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Wanted type %s, got type %s", text_bold(type_to_str(wanted_type)), text_bold(type_to_str(given_type)));
                if (given_type->kind == TYPE_VARIADIC && ((TypeVariadic *)given_type)->type->kind == wanted_type->kind) {
                    report_error_ast(typer->parser, LABEL_HINT, (Ast *)(arg), "Use ... to spread the variadic argument into the parameter");
                }
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(param->decl), "Here is the definition of '%s'", param->name);
                return NULL;
            }
        }

        if (!func_defn->is_extern) {
            if (has_spreaded_vararg) {
                // Use the provided slice or variadic as the underlying slice
                AstArgument *spreaded_arg = ((AstArgument **)call->arguments.items)[spreaded_arg_index];

                if (spreaded_arg->value->type->kind != TYPE_ARRAY && spreaded_arg->value->type->kind != TYPE_VARIADIC) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)spreaded_arg->value, "Compiler Error: Expected spreaded argument to be of type slice, variadic or any. Got type %s", type_to_str(spreaded_arg->value->type));
                    return NULL;
                }

                if (spreaded_arg->value->type->kind == TYPE_VARIADIC) {
                    TypeArray *slice_type = generate_slice_of(typer->parser, vararg_type->type);    
                    spreaded_arg->value->type = (Type *)slice_type;
                }

                // Build up the lowered argument list
                call->lowered_arguments = da_init(func_defn->variadic_parameter_index + 1, sizeof(AstExpr *));
    
                // Add the user arguments
                for (int i = 0; i < call->arguments.count; i++) {
                    AstArgument *arg = ((AstArgument **)call->arguments.items)[i];
                    da_append(&call->lowered_arguments, arg->value);
                }
            } else {
                // Generate a slice argument to hold the variadic arguments
                TypeArray *slice_type = generate_slice_of(typer->parser, vararg_type->type);
                
                AstArrayLiteral *slice = ast_allocate(typer->parser, sizeof(AstArrayLiteral));
                slice->head.head.kind = AST_ARRAY_LITERAL;
                slice->head.head.flags = AST_FLAG_COMPILER_GENERATED;
                slice->head.type = (Type *)slice_type;
                slice->expressions = da_init(2, sizeof(AstExpr *));

                for (int i = 0; i < call->arguments.count; i++) {
                    AstArgument *arg = ((AstArgument **)call->arguments.items)[i];
                    if (!arg->is_vararg) continue;

                    if (arg->value->type->kind != TYPE_ANY && vararg_type->type->kind == TYPE_ANY) {
                        AstCast *any_cast = generate_cast_to_any(typer, arg->value);
                        if (!any_cast) return NULL;
                        arg->value = (AstExpr *) any_cast;
                    }

                    da_append(&slice->expressions, arg->value);
                    
                    // Reserve space for the caller to allocate each variadic argument
                    reserve_temporary_storage(typer->enclosing_function, arg->value->type->size);
                }

                // Reserve space for the underlying array holding the arguments
                // reserve_local_storage(typer->enclosing_function, arg->value->type->size);

                slice_type->count = slice->expressions.count;

                // Build up the lowered argument list
                call->lowered_arguments = da_init(func_defn->parameters.count, sizeof(AstExpr *));
    
                // Add the user arguments
                for (int i = 0; i < call->arguments.count; i++) {
                    AstArgument *arg = ((AstArgument **)call->arguments.items)[i];

                    // Add the variadic slice argument
                    if (i == func_defn->variadic_parameter_index) {
                        da_append(&call->lowered_arguments, slice);

                        // There used to be a continue here, but that causes any default arguments not to be included
                        // continue;
                    }

                    // Skip over varargs
                    if (arg->is_vararg) continue;

                    da_append(&call->lowered_arguments, arg->value);
                }
            }
        } else {
            // Build up the lowered argument list
            call->lowered_arguments = da_init(func_defn->variadic_parameter_index + 1, sizeof(AstExpr *));

            // Add all the arguments and mark the variadic arguments
            for (int i = 0; i < call->arguments.count; i++) {
                AstArgument *arg = ((AstArgument **)call->arguments.items)[i];
                if (i >= func_defn->variadic_parameter_index) {
                    arg->value->head.flags |= AST_FLAG_IS_C_VARARG;
                }

                da_append(&call->lowered_arguments, arg->value);
            }
        }

    } 
    else {
        // Non-variadic

        call->lowered_arguments = da_init(call->arguments.count, sizeof(AstExpr *));

        for (int i = 0; i < call->arguments.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)(func_defn->parameters.items))[i];

            AstArgument *arg = ((AstArgument **)(call->arguments.items))[i];
            Type *resolved = check_expression(typer, arg->value, param->type);
            if (!resolved) return NULL;

            if (!types_are_equal(arg->value->type, param->type)) {
                AstExpr *slice = collection_as_slice(typer, arg->value, param->type); // e.g passing a [..]int where a []int is wanted
                if (slice) arg->value = slice;
            }

            if (!func_defn->is_extern && arg->value->type->kind != TYPE_ANY && param->type->kind == TYPE_ANY) {
                AstCast *any_cast = generate_cast_to_any(typer, arg->value);
                if (!any_cast) return NULL;
                arg->value = (AstExpr *) any_cast;
            }

            // @Incomplete: Arguments are allowed to both widen (i32 to i64) and narrow (i64 to i32), as existing code depends on narrowing
            if (!types_are_equal(param->type, arg->value->type) && !types_are_equal(arg->value->type, param->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(arg), "Type mismatch. Wanted type %s, got type %s", text_bold(type_to_str(param->type)), text_bold(type_to_str(arg->value->type)));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(param->decl), "Here is the definition of '%s'", param->name);
                return NULL;
            }

            da_append(&call->lowered_arguments, arg->value);
        }
    }
    

    // A read-only self cannot be passed where a pointer is wanted, as it could be used to change self
    for (int i = call->is_method_call ? 1 : 0; i < call->arguments.count; i++) {
        AstArgument *arg = ((AstArgument **)call->arguments.items)[i];

        AstFunctionDefn *readonly = readonly_self_method(typer, arg->value);
        if (!readonly || arg->value->type == NULL || arg->value->type->kind != TYPE_POINTER) continue;
        if (arg->param_index < 0 || arg->param_index >= func_defn->parameters.count) continue;

        Type *param_type = ((AstIdentifier **)func_defn->parameters.items)[arg->param_index]->type;
        if (param_type->kind == TYPE_VARIADIC) param_type = ((TypeVariadic *)param_type)->type;
        if (param_type->kind != TYPE_POINTER) continue;

        report_readonly_self(typer, (Ast *)arg->value, readonly, "pass the pointer", arg->value);
        report_error_ast(typer->parser, LABEL_NOTE, NULL, "It could be used to change self. Pass a copy with *self instead");
        return NULL;
    }

    if (func_defn->return_types.count > 1) {
        call->head.type = (Type *) make_tuple_type_from_type_list(typer, func_defn->return_types);
    } else {
        call->head.type = ((Type **)func_defn->return_types.items)[0];
    }
    call->head.head.flags |= AST_FLAG_IS_TYPE_CHECKED;

    //
    // Allocate space for arguments and potentially for the return value in the calling function
    // @Note - We try and follow the msvc x86 calling convention ???
    // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#parameter-passing 
    //

    if (func_defn->is_variadic) {
        // Reserve space for the created array + slice to it
        int num_varargs = call->arguments.count - func_defn->variadic_parameter_index;
        if (call->has_positional_arguments) {
            num_varargs = call->first_positional_argument_index - func_defn->variadic_parameter_index;
        }

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

    // Reserve space for a pushed function pointer
    reserve_temporary_storage(typer->enclosing_function, 8);

    return call->head.type;
}

int get_unique_symbol_id(Typer *typer) {
    typer->next_unique_symbol_id += 1;
    return typer->next_unique_symbol_id;
}

Type *check_function_defn(Typer *typer, AstFunctionDefn *func_defn) {
    if (func_defn->head.head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return func_defn->head.type;
    }

    if (is_generic_function(func_defn)) {
        return check_generic_function_defn(typer, func_defn);
    }

    // Check the parameters
    push_enclosing_function(typer, func_defn);

    // Change the scope to the function body, as that is where the parameters currently live.
    AstBlock *scope_before_function_entry = typer->current_scope;
    typer->current_scope = func_defn->body;

    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func_defn->parameters.items)[i];
        bool ok = check_declaration(typer, param->decl);
        if (!ok) return NULL;

        if (param->type->kind == TYPE_ARRAY && ((TypeArray *)param->type)->array_kind == ARRAY_FIXED) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)param, "Use of fixed size array as parameter. Passing a fixed size array is always done by slice (data + count)");

            TypeArray *array_type = (TypeArray *) param->type;
            array_type->array_kind = ARRAY_SLICE;
            report_error_ast(typer->parser, LABEL_HINT, (Ast *)array_type, "Make the parameter a slice type: %s", type_to_str((Type *)array_type));
            return NULL;
        }

        if (param->type->kind == TYPE_VARIADIC) {
            func_defn->is_variadic = true;
            func_defn->variadic_parameter_index = i;

            if (i != func_defn->parameters.count - 1 && !func_defn->has_default_parameters) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *) param, "Variadic parameters must appear as the last normal parameter");
                return NULL;
            }
        }
    }

    // Check the return type/s
    for (int i = 0; i < func_defn->return_types.count; i++) {
        Type **return_type_ptr = &((Type **)func_defn->return_types.items)[i];
        Type *resolved_type = resolve_type(typer, *return_type_ptr);
        if (!resolved_type) return NULL;
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
            return NULL;
        }
    }

    if (func_defn->is_extern) {
        // Limit what C calls can have as a function signature
        if (func_defn->return_types.count > 1) {
            Type *last_return_type = ((Type **)func_defn->return_types.items)[func_defn->return_types.count - 1];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)last_return_type, "Only 1 return type is allowed for external C functions");
            return NULL;
        }
        if (func_defn->is_method) {
            AstIdentifier *receiver = ((AstIdentifier **)func_defn->parameters.items)[0];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)receiver, "Methods are not allowed for external C functions");
            return NULL;
        }
    }

    TypeFunction *func_type = ast_allocate(typer->parser, sizeof(TypeFunction));
    func_type->head.head.kind  = AST_TYPE;
    func_type->head.head.start = func_defn->head.head.start;
    func_type->head.head.end   = func_defn->head.head.end;
    func_type->head.kind       = TYPE_FUNCTION;
    func_type->head.size       = 8;
    func_type->node            = func_defn;


    // Do method specific stuff
    if (func_defn->is_method) {
        func_defn->receiver = ((AstIdentifier**)func_defn->parameters.items)[0];
        Type *receiver_type = func_defn->receiver->type;

        // The receiver is a struct, a string, a primitive or an enum, or a pointer to one of those
        Type *receiver_value_type = receiver_type->kind == TYPE_POINTER ? ((TypePointer *)receiver_type)->pointer_to : receiver_type;
        TypeStruct *receiver_struct = method_holder_of(typer, receiver_value_type);
        if (receiver_struct == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)receiver_type, "Receiver type must be a struct, a string, a primitive type or an enum, or a pointer to one of those. Got type %s", text_bold(type_to_str(receiver_type)));
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "Methods on arrays and pointers to pointers are not supported yet");
            return NULL;
        }

        // Add the function as a member of the receiver struct
        if (!func_defn->is_registered_method) {
            bool ok = register_method(typer, func_defn, receiver_struct);
            if (!ok) return NULL;
        }

        func_defn->receiver_struct = receiver_struct;
    }


    if (!check_no_private_types_in_signature(typer, func_defn)) return NULL;

    // Mark the function as fully typed, when we have typechecked the function signature
    // @Note: We mark it typechecked here so that if the function recurses on itself, then the recursive call knows that it shouldn't typecheck the function definition again
    func_defn->head.head.flags |= AST_FLAG_IS_TYPE_CHECKED;
    func_defn->head.type = (Type *) func_type;

    // Check the body
    bool ok = check_block(typer, func_defn->body);
    if (!ok) return NULL;

    // Close function scope
    pop_enclosing_function(typer);
    typer->current_scope = scope_before_function_entry;

    // Do a narrow scan for a return statement
    ok = check_if_function_needs_a_return(typer, func_defn);
    if (!ok) return NULL;


    // Assign a symbol name to the function
    if (func_defn->is_lambda) {

        if (func_defn->is_method) {
            XXX;
        } else {

            // @Cleanup @Speed: Use a string buffer located in Parser to allocate the strings instead of malloc
            func_defn->symbol_name = malloc(64);

            int id = get_unique_symbol_id(typer);
            sprintf(func_defn->symbol_name, "lambda_%d", id);
        }

    } else {
        if (func_defn->is_method) {
            // Instances of generic structs have names like 'Pair<int, *float>' that are not valid asm symbols, so they are mangled e.g Pair$int$Pfloat.foo
            StringBuilder sb = sb_init(32);
            append_mangled_name(&sb, func_defn->receiver_struct->identifier->name);
            sb_append(&sb, ".%s", func_defn->identifier->name);
            func_defn->symbol_name = sb_to_string(&sb);
        } else {
            func_defn->symbol_name = func_defn->identifier->name;
        }

        // Functions of a namespace get it as a prefix e.g game$physics$collide, so that namespaces can have functions with the same name
        AstFile *file = func_defn->head.head.file;
        if (file && file->ns && !func_defn->is_extern) {
            StringBuilder sb = sb_init(32);
            for (char *c = file->ns->name; *c; c++) sb_append(&sb, "%c", *c == '.' ? '$' : *c);
            sb_append(&sb, "$%s", func_defn->symbol_name);
            func_defn->symbol_name = sb_to_string(&sb);
        }
    }

    if (func_defn->generic_function && !func_defn->is_method) {
        // Instances get a symbol named after their type arguments e.g max$i64 or set$i32$string. Methods are already named after their receiver
        StringBuilder sb = sb_init(32);
        sb_append(&sb, "%s", func_defn->symbol_name);
        for (int i = 0; i < func_defn->type_args.count; i++) {
            sb_append(&sb, "$");
            append_mangled_name(&sb, type_to_str(((Type **)func_defn->type_args.items)[i]));
        }
        func_defn->symbol_name = sb_to_string(&sb);
    }

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
                lowered_var_param->scope = param->scope;
                param->ident_override = lowered_var_param;

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

    return (Type *) func_type;
}

Type *check_enum_defn(Typer *typer, AstEnum *ast_enum) {
    TypeEnum *enum_defn = (TypeEnum *) ast_enum->identifier->type;
    assert(enum_defn != NULL);

    if (ast_enum->head.flags & AST_FLAG_IS_TYPE_CHECKED) {
        return (Type *) enum_defn;
    }

    ast_enum->is_being_checked = true;
    Type *result = check_enum_values(typer, ast_enum);
    ast_enum->is_being_checked = false;
    return result;
}

Type *check_enum_values(Typer *typer, AstEnum *ast_enum) {
    TypeEnum *enum_defn = (TypeEnum *) ast_enum->identifier->type;
    int min_value = 0;
    int max_value = 0;
    int auto_increment_value = 0;

    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        if (etor->expr) {
            Type *ok = check_expression(typer, etor->expr, (Type *)(enum_defn)); // Type definition of the enum gets to be the ctx type so we can refer to enum members inside the enum with the shorthand syntax .ENUM_MEMBER
            if (!ok) return NULL;

            AstExpr *const_expr = simplify_expression(typer->const_evaluator, typer->current_scope, etor->expr); // @ScopeRefactoring - We still need to give enums their own scope. This probably doesn't work!!!
            if (const_expr->head.kind != AST_LITERAL) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Value must be a constant expression");
                return NULL;
            }
            if (const_expr->type->kind != TYPE_INTEGER) { // @Robustness - Check that the value of the integer does not overflow the backing type if made explicit. Maybe if the value goes beyond the default int we promote the integer type to fit the largest value???
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(etor->expr), "Enum value must have type int, but value is of type %s", type_to_str(etor->expr->type));
                return NULL;
            }

            etor->value = ((AstLiteral *)(const_expr))->as.value.integer;
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

        if (strcmp(etor->name, "count") == 0) {
            Ast *site = (Ast *) etor->expr;
            if (!site) site = (Ast *) etor;
            report_error_ast(typer->parser, LABEL_ERROR, site, "Enum value 'count' is reserved for the number of enum values contained in the enum definition");
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "If you are trying to do C-style count to get the number of enum values, simply use %s.count", ast_enum->identifier->name);
            return NULL;
        }
        if (strcmp(etor->name, "names") == 0) {
            Ast *site = (Ast *) etor->expr;
            if (!site) site = (Ast *) etor;
            report_error_ast(typer->parser, LABEL_ERROR, site, "Enum value 'names' is reserved for the list of enum value names");
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Please pick a different name for the enum value");
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

AstIdentifier *get_static_enum_member(TypeEnum *enum_defn, char *name) {
    if (strcmp(name, "count") == 0) {
        return enum_defn->count;
    }
    if (strcmp(name, "names") == 0) {
        return enum_defn->names;
    }
    if (strcmp(name, "values") == 0) {
        return enum_defn->values;
    }

    return NULL;
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
    // A variant without a value of the union that is wanted e.g 'return .none;' for an Option<int>
    if (!literal->explicit_type && lhs_type && lhs_type->kind == TYPE_STRUCT && ((TypeStruct *)lhs_type)->node->is_union) {
        TypeStruct    *union_type = (TypeStruct *)lhs_type;
        AstIdentifier *variant    = union_variant(union_type, literal->identifier->name);
        if (variant == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "'%s' is not a variant of %s", literal->identifier->name, type_to_str(lhs_type));
            return NULL;
        }
        if (variant->type->kind != TYPE_VOID) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "The variant '%s' needs a value", variant->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "e.g %s{%s: ...}", type_to_str(lhs_type), variant->name);
            return NULL;
        }
        AstStructLiteral *value = make_union_literal(typer, union_type, variant, (Ast *)literal);
        Type *type = check_expression(typer, (AstExpr *)value, lhs_type);
        if (!type) return NULL;
        literal->head.replacement = (AstExpr *)value;
        return type;
    }

    if (literal->explicit_type) {
        // A value of an anonymous enum e.g enum { open, closed }.open
        Type *resolved = resolve_type(typer, literal->explicit_type);
        if (!resolved) return NULL;
        literal->explicit_type = resolved;
        lhs_type = resolved;
    }

    if (lhs_type == NULL || lhs_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Couldn't infer type of enum value from the context");
        return NULL;
    }

    TypeEnum *enum_defn = (TypeEnum *)(lhs_type);
    char     *enum_name = literal->identifier->name;


    // Try get a static enum member
    // @EnumRefactor @Cleanup: I think we want enum definitions to have a proper scope. All the enum values
    //           could just be constant struct members
    AstIdentifier *static_member = get_static_enum_member(enum_defn, enum_name);
    if (static_member) {
        literal->enum_member = (AstEnumerator *) static_member;
        return static_member->type;
    }

    AstEnumerator *found = find_enum_member(enum_defn->node, enum_name);
    if (!found) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "'%s' is not a member of enum '%s'", enum_name, enum_defn->identifier->name);

        if (enum_defn->node->enumerators.count <= 5) {
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enum_defn), "Here is the definition of '%s'", enum_defn->identifier->name);
        }

        return NULL;
    }

    // An enum declared later or in another file is checked now, e.g for the default value of a member 'style: Style = .plain;'
    if (!(found->head.flags & AST_FLAG_IS_TYPE_CHECKED) && !enum_defn->node->is_being_checked) {
        Typer saved_state = *typer;
        typer->current_file  = enum_defn->node->head.file ? enum_defn->node->head.file : typer->current_file;
        typer->current_scope = enum_defn->identifier->scope ? enum_defn->identifier->scope : typer->current_scope;
        Type *ok = check_enum_defn(typer, enum_defn->node);
        typer_restore_state(typer, saved_state);
        if (!ok) return NULL;
    }

    if (!(found->head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Enum member used before being declared", enum_name, enum_defn->identifier->name);
        return NULL;
    }

    literal->enum_member = found;

    return lhs_type;
}

//
// Collection protocol
//
// The syntax for collections works for any struct that has these methods, so collections like List and Map are written in Saad:
//
//   c[k]                    c.index(k) if c has it and c[k] is read, otherwise *c.indexRef(k)
//   c[k] = v, &c[k], ...    *c.indexRef(k)
//   for x in c              for x in c.elements(), where elements() returns a slice
//   c where a []T is wanted c.elements()
//   [a, b, c]               c.reserve(3), c.add(a), c.add(b), ... when a collection type is wanted
//   [k1: v1, k2: v2]        c.reserve(2), c.set(k1, v1), ... The type is Map<K, V> from the standard library if no collection type is wanted
//

// Finds a method on a struct, also on the generic struct of an instance
AstIdentifier *find_method(TypeStruct *struct_type, char *name) {
    AstIdentifier *method = get_struct_method(struct_type->node, name);
    if (!method && struct_type->node->generic_struct) {
        method = get_struct_method(struct_type->node->generic_struct, name);
    }
    return method;
}

// Makes the AST of a call 'receiver.name(args...)', as if it was written where the receiver is
AstFunctionCall *generate_method_call(Typer *typer, AstExpr *receiver, char *name, AstExpr **args, int arg_count) {
    Parser *parser = typer->parser;

    AstLiteral *method_name = ast_allocate(parser, sizeof(AstLiteral));
    method_name->head.head      = receiver->head;
    method_name->head.head.kind = AST_LITERAL;
    method_name->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    method_name->kind           = LITERAL_IDENTIFIER;
    method_name->as.value.identifier.name   = name;
    method_name->as.value.identifier.length = (int)strlen(name);

    AstMemberAccess *ma = ast_allocate(parser, sizeof(AstMemberAccess));
    ma->head.head       = receiver->head;
    ma->head.head.kind  = AST_MEMBER_ACCESS;
    ma->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    ma->left            = receiver;
    ma->right           = (AstExpr *)method_name;

    AstFunctionCall *call = ast_allocate(parser, sizeof(AstFunctionCall));
    call->head.head       = receiver->head;
    call->head.head.kind  = AST_FUNCTION_CALL;
    call->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    call->expression      = (AstExpr *)ma;
    call->arguments       = da_init(arg_count > 0 ? arg_count : 1, sizeof(AstArgument *));
    call->paren_start_token.start = receiver->head.end;
    call->paren_start_token.end   = receiver->head.end;
    for (int i = 0; i < arg_count; i++) {
        AstArgument *arg = generate_argument_from_expr(typer, args[i]);
        da_append(&call->arguments, arg);
    }
    return call;
}

// c[k] on a collection
Type *check_collection_index(Typer *typer, AstArrayAccess *array_ac, TypeStruct *collection, bool place) {
    AstIdentifier *index     = find_method(collection, "index");
    AstIdentifier *index_ref = find_method(collection, "indexRef");

    if (!index && !index_ref) {
        report_error_range(typer->parser, array_ac->left->head.start, array_ac->open_bracket.start, "Cannot index into expression of type %s", type_to_str((Type *)collection));
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Give %s a method 'index :: method (key: K) -> V' to read with c[k], or 'indexRef :: method (key: K) -> *V' to also change c[k]", collection->head.name);
        return NULL;
    }
    if (place && !index_ref) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)array_ac, "Cannot change %s[...], as %s only has an 'index' method to read it", collection->head.name, collection->head.name);
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Give it a method 'indexRef :: method (key: K) -> *V' to be able to change c[k]");
        return NULL;
    }

    bool use_ref = place || !index;
    AstFunctionCall *call = generate_method_call(typer, array_ac->left, use_ref ? "indexRef" : "index", &array_ac->index_expr, 1);

    AstExpr *replacement = (AstExpr *)call;
    if (use_ref) {
        AstUnary *deref        = ast_allocate(typer->parser, sizeof(AstUnary));
        deref->head.head       = array_ac->head.head;
        deref->head.head.kind  = AST_UNARY;
        deref->head.head.flags = AST_FLAG_COMPILER_GENERATED;
        deref->operator        = OP_POINTER_DEREFERENCE;
        deref->expr            = (AstExpr *)call;
        replacement = (AstExpr *)deref;
    }

    Type *type = check_expression(typer, replacement, NULL);
    if (!type) return NULL;

    array_ac->head.replacement = replacement;
    return type;
}

// Returns the call c.elements() if 'expr' is a collection and its elements are wanted as the slice 'wanted', otherwise NULL
AstExpr *collection_as_slice(Typer *typer, AstExpr *expr, Type *wanted) {
    if (expr->type == NULL || expr->type->kind != TYPE_STRUCT) return NULL;
    if (wanted && !(wanted->kind == TYPE_ARRAY && ((TypeArray *)wanted)->array_kind == ARRAY_SLICE) && wanted->kind != TYPE_ANY) return NULL;
    if (!find_method((TypeStruct *)expr->type, "elements")) return NULL;

    AstFunctionCall *call = generate_method_call(typer, expr, "elements", NULL, 0);
    Type *type = check_expression(typer, (AstExpr *)call, NULL);
    if (!type) return NULL;
    if (wanted && wanted->kind != TYPE_ANY && !types_are_equal(wanted, type)) return NULL;
    return (AstExpr *)call;
}

// Returns an instance of a collection from the standard library e.g Map<string, int>
TypeStruct *instantiate_standard_collection(Typer *typer, char *name, Type *arg0, Type *arg1, Ast *site) {
    Parser *parser = typer->parser;
    if (parser->collections_file == NULL) {
        report_error_ast(parser, LABEL_ERROR, site, "Compiler Error: The standard collections are not loaded");
        return NULL;
    }

    TypeGenericInstance *instance = type_alloc(&parser->type_table, sizeof(TypeGenericInstance));
    instance->head.head      = *site;
    instance->head.head.kind = AST_TYPE;
    instance->head.kind      = TYPE_GENERIC_INSTANCE;
    instance->head.name      = name;
    instance->is_standard    = true;
    instance->type_args      = da_init(2, sizeof(Type *));
    da_append(&instance->type_args, arg0);
    if (arg1) da_append(&instance->type_args, arg1);

    return instantiate_generic_struct(typer, instance);
}

// [a, b, c] or [k: v, ...] as a collection, built in a hidden local by calling the collection's add() or set() method for each element
Type *check_collection_literal(Typer *typer, AstArrayLiteral *lit, TypeStruct *collection) {
    Parser *parser = typer->parser;

    if (typer->enclosing_function == NULL) {
        if (typer->checking_member_default) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)lit, "The default value of a struct member cannot be a collection literal yet");
            report_error_ast(parser, LABEL_HINT, NULL, "Leave the default out to start with an empty collection");
            return NULL;
        }
        report_error_ast(parser, LABEL_ERROR, (Ast *)lit, "Collection literals can only be used inside functions");
        return NULL;
    }

    if (collection == NULL) {
        // A map literal without a wanted type is a Map<K, V> from the standard library, with K and V from the first entry
        if (lit->keys.count == 0) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)lit, "Cannot infer the type of the empty map literal [:]");
            report_error_ast(parser, LABEL_HINT, NULL, "Give the variable a type e.g ages: Map<string, int> = [:];");
            return NULL;
        }

        Type *key_type   = check_expression(typer, ((AstExpr **)lit->keys.items)[0], NULL);
        Type *value_type = check_expression(typer, ((AstExpr **)lit->expressions.items)[0], NULL);
        if (!key_type || !value_type) return NULL;
        if (is_untyped_type(key_type))   key_type   = solidify_untyped_type(key_type);
        if (is_untyped_type(value_type)) value_type = solidify_untyped_type(value_type);

        collection = instantiate_standard_collection(typer, "Map", key_type, value_type, (Ast *)lit);
        if (!collection) return NULL;
    }

    char *add_name = lit->is_map_literal ? "set" : "add";
    if (!find_method(collection, add_name)) {
        if (lit->is_map_literal) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)lit, "A map literal cannot be a %s", type_to_str((Type *)collection));
            report_error_ast(parser, LABEL_HINT, NULL, "Give %s a method 'set :: method * (key: K, value: V)' to be able to make it from a literal like [k: v]", collection->head.name);
        } else {
            report_error_ast(parser, LABEL_ERROR, (Ast *)lit, "An array literal cannot be a %s", type_to_str((Type *)collection));
            report_error_ast(parser, LABEL_HINT, NULL, "Give %s a method 'add :: method * (value: T)' to be able to make it from a literal like [a, b, c]", collection->head.name);
        }
        return NULL;
    }

    // The hidden local holding the collection
    char *name = malloc(32);
    snprintf(name, 32, "$collection%d", typer->next_hidden_local_id++);

    AstIdentifier *ident = ast_allocate(parser, sizeof(AstIdentifier));
    ident->head.kind  = AST_IDENTIFIER;
    ident->head.file  = lit->head.head.file;
    ident->head.start = lit->head.head.start;
    ident->head.end   = lit->head.head.start;
    ident->name       = name;
    ident->type       = (Type *)collection;
    ident->scope      = typer->current_scope;
    ident->flags      = IDENTIFIER_IS_RESOLVED;

    AstDeclaration *decl = ast_allocate(parser, sizeof(AstDeclaration));
    decl->head.kind  = AST_DECLARATION;
    decl->head.file  = lit->head.head.file;
    decl->head.start = lit->head.head.start;
    decl->head.end   = lit->head.head.start;
    decl->head.flags = AST_FLAG_COMPILER_GENERATED | AST_FLAG_IS_TYPE_CHECKED;
    decl->idents     = da_init(1, sizeof(AstIdentifier *));
    decl->values     = da_init(1, sizeof(AstExpr *));
    decl->type       = (Type *)collection;
    decl->flags      = DECLARATION_TYPED_NO_EXPR | DECLARATION_IS_COMPILER_GENERATED;
    da_append(&decl->idents, ident);
    ident->decl = decl;
    da_append(&typer->current_scope->identifiers, ident);

    AstLiteral *value = ast_allocate(parser, sizeof(AstLiteral));
    value->head.head      = lit->head.head;
    value->head.head.kind = AST_LITERAL;
    value->head.head.start = lit->head.head.start;
    value->head.head.end   = lit->head.head.start;
    value->kind           = LITERAL_IDENTIFIER;
    value->as.value.identifier.name   = name;
    value->as.value.identifier.length = (int)strlen(name);

    lit->collection_decl  = decl;
    lit->collection_value = (AstExpr *)value;
    lit->collection_calls = da_init(lit->expressions.count + 1, sizeof(AstFunctionCall *));

    Type *type = check_expression(typer, (AstExpr *)value, NULL);
    if (!type) return NULL;

    if (lit->expressions.count > 0 && find_method(collection, "reserve")) {
        AstLiteral *count = ast_allocate(parser, sizeof(AstLiteral));
        count->head.head      = lit->head.head;
        count->head.head.kind = AST_LITERAL;
        count->kind           = LITERAL_INTEGER;
        count->as.value.integer = lit->expressions.count;

        AstExpr *args[1] = { (AstExpr *)count };
        AstFunctionCall *call = generate_method_call(typer, (AstExpr *)value, "reserve", args, 1);
        if (!check_expression(typer, (AstExpr *)call, NULL)) return NULL;
        da_append(&lit->collection_calls, call);
    }

    for (int i = 0; i < lit->expressions.count; i++) {
        AstExpr *element = ((AstExpr **)lit->expressions.items)[i];

        AstFunctionCall *call = NULL;
        if (lit->is_map_literal) {
            AstExpr *args[2] = { ((AstExpr **)lit->keys.items)[i], element };
            call = generate_method_call(typer, (AstExpr *)value, "set", args, 2);
        } else {
            AstExpr *args[1] = { element };
            call = generate_method_call(typer, (AstExpr *)value, "add", args, 1);
        }
        if (!check_expression(typer, (AstExpr *)call, NULL)) return NULL;
        da_append(&lit->collection_calls, call);
    }

    return (Type *)collection;
}

Type *check_array_literal(Typer *typer, AstArrayLiteral *array_lit, Type *ctx_type) {
    if (ctx_type && ctx_type->kind == TYPE_ANY) {
        // Strip the context type
        ctx_type = NULL;
    }

    // A literal of a collection type e.g 'xs: [..]int = [1, 2, 3];' or 'ages: Map<string, int> = ["alice": 31];'
    if (ctx_type && ctx_type->kind == TYPE_STRUCT) {
        return check_collection_literal(typer, array_lit, (TypeStruct *)ctx_type);
    }
    if (array_lit->is_map_literal) {
        return check_collection_literal(typer, array_lit, NULL);
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
            if (!any_cast) return NULL;
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
    bool place = typer->current_place;

    typer->checking_place = place;
    Type *type_being_accessed = check_expression(typer, array_ac->left, NULL);
    if (!type_being_accessed) return NULL;

    if (type_being_accessed->kind == TYPE_STRUCT) {
        return check_collection_index(typer, array_ac, (TypeStruct *)type_being_accessed, place);
    }

    if (type_being_accessed->kind != TYPE_ARRAY && type_being_accessed->kind != TYPE_POINTER && type_being_accessed->kind != TYPE_VARIADIC && type_being_accessed->kind != TYPE_STRING) {
        report_error_range(typer->parser, array_ac->left->head.start, array_ac->open_bracket.start, "Cannot index into expression of type %s", type_to_str(type_being_accessed));
        return NULL;
    }

    Type *elem_type = NULL;

    if (type_being_accessed->kind == TYPE_ARRAY) {
        TypeArray *type_array = (TypeArray *) type_being_accessed;

        elem_type = type_array->elem_type;
    }

    if (type_being_accessed->kind == TYPE_VARIADIC) {
        // Treat the variadic as a slice
        TypeVariadic *variadic = (TypeVariadic *) type_being_accessed;
        array_ac->left->type = (Type *) variadic->slice;

        elem_type = variadic->type;
    }

    if (type_being_accessed->kind == TYPE_STRING) {
        array_ac->left->type = (Type *) type_defn_string_as_slice;
        elem_type = primitive_type(PRIMITIVE_CHAR);
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
        array->head.head.start = array_ac->left->head.start;
        array->head.head.end   = array_ac->left->head.end;
        array->head.kind       = TYPE_ARRAY;
        array->array_kind      = ARRAY_SLICE;
        array->elem_type       = ptr->pointer_to;
        array->struct_defn     = struct_defn;
        array->capacity_expr   = NULL;
        array->capacity        = 0;
        array->count           = 0;

        array_ac->left->type = (Type *) array;
        array_ac->head.head.flags |= AST_FLAG_IS_ARRAY_INDEX_INTO_POINTER;

        elem_type = ptr->pointer_to;
    }

    Type *index_expr_type = check_expression(typer, array_ac->index_expr, primitive_type(PRIMITIVE_U32));
    if (!index_expr_type) return NULL;

    if (index_expr_type->kind != TYPE_INTEGER && index_expr_type->kind != TYPE_ENUM) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(array_ac->index_expr), "Array index must be an integer or enum type, got type %s", type_to_str(index_expr_type));
        return NULL;
    }

    // Reserve temporary space for the array literal if its
    // used in a quick array lookup like:  [1, 2, 3][0];
    if (array_ac->left->head.kind == AST_ARRAY_LITERAL) {
        reserve_temporary_storage(typer->enclosing_function, array_ac->left->type->size);
    }

    return elem_type;
}

Type *check_typeof(Typer *typer, AstTypeof *ast_typeof) {
    Type *expr_type = check_expression(typer, ast_typeof->expr, NULL);
    if (!expr_type) return NULL;

    return (Type *) type_defn_string;
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
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ast_new->expr, "Arrays cannot be newed. A dynamic array is a list");
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Make a list from a literal instead e.g xs: [..]int = [1, 2, 3];");
        return NULL;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ast_new->expr, "Invalid expression to new. Only arrays and structs can be newed");
    return NULL;
}

Type *check_expression(Typer *typer, AstExpr *expr, Type *ctx_type) {
    Type *result = NULL;

    // Only array and member accesses care about being used as a place. Everything below them is checked as a value, unless they say otherwise
    typer->current_place  = typer->checking_place;
    typer->checking_place = false;

    if      (expr->head.kind == AST_LITERAL)        result = check_literal(typer, (AstLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_BINARY)         result = check_binary(typer, (AstBinary *)(expr), ctx_type);
    else if (expr->head.kind == AST_UNARY)          result = check_unary(typer, (AstUnary *)(expr), ctx_type);
    else if (expr->head.kind == AST_CAST)           result = check_cast(typer, (AstCast *)(expr), ctx_type);
    else if (expr->head.kind == AST_FUNCTION_CALL)  result = check_function_call(typer, (AstFunctionCall *)(expr));
    else if (expr->head.kind == AST_FUNCTION_DEFN)  result = check_function_defn(typer, (AstFunctionDefn *)(expr));
    else if (expr->head.kind == AST_STRUCT_LITERAL) result = check_struct_literal(typer, (AstStructLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_SWITCH) result = check_switch_expression(typer, (AstSwitch *)(expr), ctx_type);
    else if (expr->head.kind == AST_ARRAY_LITERAL)  result = check_array_literal(typer, (AstArrayLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_ENUM_LITERAL)   result = check_enum_literal(typer, (AstEnumLiteral *)(expr), ctx_type);
    else if (expr->head.kind == AST_MEMBER_ACCESS)  result = check_member_access(typer, (AstMemberAccess *)(expr));
    else if (expr->head.kind == AST_ARRAY_ACCESS)   result = check_array_access(typer, (AstArrayAccess *)(expr));
    else if (expr->head.kind == AST_TYPEOF)         result = check_typeof(typer, (AstTypeof *)(expr));
    else if (expr->head.kind == AST_SIZEOF)         result = check_sizeof(typer, (AstSizeof *)(expr));
    else if (expr->head.kind == AST_NEW)            result = check_new(typer, (AstNew *)(expr), ctx_type);
    else if (expr->head.kind == AST_RANGE_EXPR)     result = primitive_type(PRIMITIVE_I64); // @Investigate - Shouldn't this be checked for both sides being integers???
    else if (expr->head.kind == AST_SEMICOLON_EXPR) result = primitive_type(PRIMITIVE_VOID);
    else {
        printf("%s:%d: compiler-error: Unhandled cases in 'check_expression()'. Expression was of type %s", __FILE__, __LINE__, ast_to_str((Ast *)expr));
        exit(1);
    }

    if (result == NULL) {
        return NULL;
    }

    expr->type = result;
    return result;
}

Type *check_member_access(Typer *typer, AstMemberAccess *ma) {
    // A qualified name e.g math.PI or m.Color
    bool failed = false;
    AstIdentifier *qualified = resolve_qualified_name(typer, (AstExpr *)ma, &failed);
    if (failed) return NULL;
    if (qualified) {
        AstLiteral *literal = make_resolved_identifier_literal(typer, qualified, (Ast *)ma);
        Type *type = check_expression(typer, (AstExpr *)literal, NULL);
        if (!type) return NULL;
        ma->head.replacement = (AstExpr *)literal;
        return type;
    }

    typer->checking_place = typer->current_place; // e.g xs[i].x = 5 changes the element in the collection, so xs[i] is a place too
    Type *type_lhs = check_expression(typer, ma->left, NULL);
    if (!type_lhs) return NULL;

    //
    // 1'st case : Member access on enum
    // @Cleanup - I think this bit of code can be better put elsewhere in a unary dot as that is what we are checking for here. e.g .ENUM
    //            That way, we wouldn't need to have an access kind for the member
    //
    AstExpr *lhs = ma->left->replacement ? ma->left->replacement : ma->left; // e.g m.Color in m.Color.Red
    if (type_lhs->kind == TYPE_ENUM && lhs->head.kind == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) {
        char *ident_name    = ((AstLiteral *)(lhs))->as.value.identifier.name;
        TypeEnum *enum_defn = ((TypeEnum *)(type_lhs));
        AstIdentifier *lhs_ident = ((AstLiteral *)(lhs))->as.value.identifier.resolved_identifier;
        bool names_enum = strcmp(ident_name, enum_defn->identifier->name) == 0 || (lhs_ident && (lhs_ident->flags & IDENTIFIER_IS_TYPE_ALIAS)); // e.g Hue.Red with Hue :: Color;
        if (names_enum) {

            AstExpr *rhs = ma->right;
            if (rhs->head.kind == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
                char *name = ((AstLiteral *)(rhs))->as.value.identifier.name;

                // Try get one of the static members on type enum
                // @EnumRefactor
                AstIdentifier *static_member = get_static_enum_member(enum_defn, name);
                if (static_member) {
                    ma->access_kind = MEMBER_ACCESS_ENUM_MEMBER;
                    ma->enum_member = (AstEnumerator *) static_member;
                    return static_member->type;
                }

                // Normal case
                AstEnumerator *enum_member = find_enum_member(enum_defn->node, name);
                if (!enum_member) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "%s is not a member of enum %s", name, enum_defn->identifier->name);
                    report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(enum_defn), "Here is the definition of %s", enum_defn->identifier->name);
                    return NULL;
                }

                ma->access_kind = MEMBER_ACCESS_ENUM_MEMBER;
                ma->enum_member = enum_member;

                return (Type *)(enum_defn);
            }
        }
    }

    bool is_static_member_access = false;
    if (type_lhs->kind == TYPE_STRUCT && lhs->head.kind == AST_LITERAL && ((AstLiteral *)(lhs))->kind == LITERAL_IDENTIFIER) {
        char *ident_name = ((AstLiteral *)(lhs))->as.value.identifier.name;
        AstIdentifier *ident = ((AstLiteral *)(lhs))->as.value.identifier.resolved_identifier;
        if (!ident) ident = lookup_from_scope(typer->parser, typer->current_scope, ident_name);
        assert(ident && "Typechecked the name of a struct, but didn't find it again while resolving member access");

        if (ident->flags & (IDENTIFIER_IS_NAME_OF_STRUCT | IDENTIFIER_IS_TYPE_ALIAS)) {
            is_static_member_access = true;
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
    else if (type_lhs->kind == TYPE_STRING) {
        TypeString *str = (TypeString *)(type_lhs);
        struct_type = str->struct_defn;
        valid_lhs = true;
    }

    if (!valid_lhs) {
        // Methods on primitive types and enums e.g x.hash() with 'hash :: method (x: int) -> u64'
        Type *value_type = type_lhs->kind == TYPE_POINTER ? ((TypePointer *)type_lhs)->pointer_to : type_lhs;
        TypeStruct *holder = method_holder_of(typer, value_type);
        if (holder && is_literal_identifier(ma->right)) {
            char *name = ((AstLiteral *)ma->right)->as.value.identifier.name;
            AstIdentifier *method = get_struct_method(holder->node, name);
            if (method) {
                ma->struct_type   = holder;
                ma->access_kind   = MEMBER_ACCESS_STRUCT_MEMBER;
                ma->struct_member = method;
                return method->type;
            }

            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ma->right), "Type %s has no method '%s'", type_to_str(type_lhs), name);
            return NULL;
        }

        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(ma->left), "Cannot access into expression of type %s", type_to_str(type_lhs));
        return NULL;
    }

    ma->struct_type = struct_type;

    AstFunctionCall *func_call = NULL;
    AstIdentifier   *member = NULL;
    char            *member_name = NULL;
    AstExpr         *rhs = ma->right;

    // Get out the member
    if (rhs->head.kind == AST_LITERAL && ((AstLiteral *)(rhs))->kind == LITERAL_IDENTIFIER) {
        AstLiteral *ident = (AstLiteral *) rhs;
        member_name = ident->as.value.identifier.name;

        // Check if its a method. Instances of generic structs also have the methods of their generic struct
        AstIdentifier *method = get_struct_method(struct_type->node, member_name);
        if (!method && struct_type->node->generic_struct) {
            method = get_struct_method(struct_type->node->generic_struct, member_name);
        }
        if (method) {
            member = method;

            // if (method->type->kind != TYPE_FUNCTION) {
            //     report_error_ast(typer->parser, LABEL_ERROR, (Ast *) rhs, "Member %s is not a callable function", method->name);
            //     return NULL;
            // }

            // AstFunctionDefn *func_defn = ((TypeFunction *)method->type)->node;
            // func_call->func_defn = func_defn;
            // func_call->is_method_call = true;
            // func_call->belongs_to_struct = struct_type->node;
    
            // // Rewrite the method call into a plain function call, with the 0'th
            // // argument being the left-hand side of the member access
            // // e.g
            // // `foo.bar(x)` -> `bar(foo, x)`
            // bool ok = rewrite_as_method_call(typer, func_call, ma);
            // if (!ok) return NULL;
    
            // Type *func_type = check_function_call(typer, func_call);
            // if (!func_type) return NULL;
    
            ma->access_kind = MEMBER_ACCESS_STRUCT_MEMBER;
        }
        else if (is_static_member_access && struct_type->node->is_union && union_variant(struct_type, member_name)) {
            // A variant without a value e.g Shape.empty
            AstIdentifier *variant = union_variant(struct_type, member_name);
            if (variant->type->kind != TYPE_VOID) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)rhs, "The variant '%s' needs a value", member_name);
                report_error_ast(typer->parser, LABEL_HINT, NULL, "e.g %s{%s: ...}", type_to_str((Type *)struct_type), member_name);
                return NULL;
            }
            AstStructLiteral *literal = make_union_literal(typer, struct_type, variant, (Ast *)ma);
            Type *type = check_expression(typer, (AstExpr *)literal, (Type *)struct_type);
            if (!type) return NULL;
            ma->head.replacement = (AstExpr *)literal;
            return type;
        }
        else if (is_static_member_access) {
            member = find_identifier_in_scope(struct_type->node->static_members, member_name);
            ma->access_kind = MEMBER_ACCESS_STATIC_STRUCT_MEMBER;
        } 
        else if (struct_type->node->is_union && strcmp(member_name, "kind") == 0) {
            // shape.kind is the tag: which variant the union holds. It changes with the variant, so it can't be set
            if (typer->current_place && !(ma->head.head.flags & AST_FLAG_COMPILER_GENERATED)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)rhs, "The kind of a union can't be set. It changes when the union is given another variant");
                report_error_ast(typer->parser, LABEL_HINT, NULL, "e.g shape = %s{...}", type_to_str((Type *)struct_type));
                return NULL;
            }
            member = ((AstIdentifier **)struct_type->node->members->identifiers.items)[0];
            ma->access_kind = MEMBER_ACCESS_STRUCT_MEMBER;
        }
        else {
            member = get_struct_member(struct_type->node, member_name);
            ma->access_kind = MEMBER_ACCESS_STRUCT_MEMBER;

            // The variants of a union are read with a switch, which knows which one the union holds
            if (member && struct_type->node->is_union && !(ma->head.head.flags & AST_FLAG_COMPILER_GENERATED)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)rhs, "The variant '%s' of a union is read with a switch", member_name);
                report_error_ast(typer->parser, LABEL_HINT, NULL, "e.g switch v in value { case .%s { ... v ... } }", member_name);
                return NULL;
            }
        }

        ma->struct_member = member;
    } 
    // else if (rhs->head.kind == AST_FUNCTION_CALL) {
    //     func_call = (AstFunctionCall *) rhs;

    //     // Method or function member call
    //     member_name = func_call->identifer->name;

    //     // Check if its a normal member of the struct (member function)
    //     member = get_struct_member(struct_type->node, member_name);
        
    //     if (member) {
    //         // Function member call e.g `myStruct.foo()`

    //         // We mark the function call as a lambda, as the actuall function that will be called is unknown
    //         // at compile time
    //         func_call->is_lambda_call = true;
    //         func_call->is_member_call = true;
    //         func_call->belongs_to_struct = struct_type->node;

    //         Type *func_type = check_function_call(typer, func_call);
    //         if (!func_type) return NULL;

    //         ma->flags |= MEMBER_ACCESS_FLAGS_RHS_IS_FUNCTION_CALL;
    //         ma->access_kind = MEMBER_ACCESS_STRUCT_MEMBER;
    //         ma->struct_member = member;
    //         ma->function_call = func_call;
    //     } else {
    //         // Check if its a method call
    //         AstIdentifier *method = get_struct_method(struct_type->node, member_name);
    //         if (method) {
    //             member = method;
    //             XXX;
    //         } else {
    //             // Fallthrough to 'not a member'
    //         }
    //     }

    // } 
    else {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "Invalid member expression");
        return NULL;
    }

    if (!member) {

        if (is_static_member_access) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "'%s' is not a static member of '%s'", member_name, type_to_str((Type *)struct_type));
        } else {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(rhs), "'%s' is not a member of '%s'", member_name, type_to_str((Type *)struct_type));
        }

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

    if (!member->type) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)member, "Compiler Error: Member has no type in 'check_member_access()'");
        return NULL;
    }

    return member->type;
}

//
// Unions
//

// The variant of a union with the name, or NULL. The hidden tag is not a variant
AstIdentifier *union_variant(TypeStruct *union_type, char *name) {
    AstIdentifier *member = get_struct_member(union_type->node, name);
    if (member == NULL || member->member_index == 0) return NULL;
    return member;
}

// The value of the kind (the tag) of a union for a variant e.g .circle of enum { circle, rect, empty }
AstExpr *union_kind_value(Typer *typer, AstIdentifier *variant, Ast *at) {
    AstEnumLiteral *kind  = ast_allocate(typer->parser, sizeof(AstEnumLiteral));
    kind->head.head.kind  = AST_ENUM_LITERAL;
    kind->head.head.file  = at->file;
    kind->head.head.start = at->start;
    kind->head.head.end   = at->end;
    kind->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    kind->identifier      = make_identifier_from_string(typer->parser, variant->name, NULL);
    kind->explicit_type   = ((AstIdentifier **)variant->scope->identifiers.items)[0]->type; // The type of the tag
    return (AstExpr *)kind;
}

// The initializer of the hidden tag for a variant e.g $tag: .circle. The first variant is 0, so a zeroed union holds it
AstStructInitializer *union_tag_initializer(Typer *typer, AstIdentifier *variant, Ast *at) {
    AstExpr *tag = union_kind_value(typer, variant, at);

    AstStructInitializer *init = ast_allocate(typer->parser, sizeof(AstStructInitializer));
    init->head.kind  = AST_STRUCT_INITIALIZER;
    init->head.file  = at->file;
    init->head.start = at->start;
    init->head.end   = at->end;
    init->designator = make_identifier_from_string(typer->parser, "$tag", NULL);
    init->value      = (AstExpr *)tag;
    return init;
}

// A union literal gives one variant its value e.g Shape{circle: Circle{2}}. The tag of the variant is added to it
bool check_union_literal(Typer *typer, AstStructLiteral *literal, TypeStruct *union_type) {
    if (literal->initializers.count == 0) {
        return true; // The first variant, zeroed
    }

    AstStructInitializer *init = ((AstStructInitializer **)literal->initializers.items)[0];
    if (literal->initializers.count > 1 || init->designator == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "A union holds one variant, so its literal gives one variant a value");
        report_error_ast(typer->parser, LABEL_HINT, NULL, "e.g %s{%s: ...}", type_to_str((Type *)union_type), ((AstIdentifier **)union_type->node->members->identifiers.items)[1]->name);
        return false;
    }

    AstIdentifier *variant = union_variant(union_type, init->designator->name);
    if (variant == NULL) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)init->designator, "'%s' is not a variant of %s", init->designator->name, type_to_str((Type *)union_type));
        return false;
    }
    if (variant->type->kind == TYPE_VOID) {
        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)init, "The variant '%s' has no value", variant->name);
        report_error_ast(typer->parser, LABEL_HINT, NULL, "Write it as %s.%s", type_to_str((Type *)union_type), variant->name);
        return false;
    }

    AstStructInitializer *tag = union_tag_initializer(typer, variant, (Ast *)init);
    da_append(&literal->initializers, tag);
    return true;
}

// The value of a variant without a value e.g Shape.empty
AstStructLiteral *make_union_literal(Typer *typer, TypeStruct *union_type, AstIdentifier *variant, Ast *at) {
    AstStructLiteral *literal = ast_allocate(typer->parser, sizeof(AstStructLiteral));
    literal->head.head.kind  = AST_STRUCT_LITERAL;
    literal->head.head.file  = at->file;
    literal->head.head.start = at->start;
    literal->head.head.end   = at->end;
    literal->head.head.flags = AST_FLAG_COMPILER_GENERATED;
    literal->explicit_type   = NULL; // union_type is given when it is checked, as an explicit struct type means it was checked already
    (void)union_type;
    literal->initializers    = da_init(1, sizeof(AstStructInitializer *));
    AstStructInitializer *tag = union_tag_initializer(typer, variant, at);
    da_append(&literal->initializers, tag);
    return literal;
}

Type *check_struct_literal(Typer *typer, AstStructLiteral *literal, Type *ctx_type) {
    TypeStruct *struct_defn = NULL;
    if (literal->explicit_type) {
        // Explicit type is used
        Type *type = literal->explicit_type;

        // The struct literal of an anonymous struct e.g struct { name: string; age: int; } {"ada", 36}
        if (type->kind == TYPE_STRUCT && ((TypeStruct *)type)->node->is_anonymous && !(literal->head.head.flags & AST_FLAG_IS_TYPE_CHECKED)) {
            Type *resolved = resolve_type(typer, type);
            if (!resolved) return NULL;

            literal->head.head.flags |= AST_FLAG_IS_TYPE_CHECKED;
            literal->explicit_type    = resolved;
            type = NULL; // Check the initializers below
            struct_defn = (TypeStruct *)resolved;
        }
        else if (type->kind == TYPE_STRUCT) {
            // Explicit type have already been resolved @Note - Currently this only happens in check_array_literal where the first expression is typechecked, and then all the expressions are again typechecked against that first expression, so we might end here.
            return literal->explicit_type;
        }

        else if (type->kind == TYPE_NAME || type->kind == TYPE_GENERIC_INSTANCE) {
            Type *resolved = resolve_type(typer, type);
            if (!resolved) return NULL;

            if (resolved->kind != TYPE_STRUCT) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(type), "'%s' is not the name of a struct. Type of '%s' is '%s'", resolved->name, resolved->name, type_to_str(type));
                return NULL;
            }
            
            literal->explicit_type = resolved;
            struct_defn = (TypeStruct *)(resolved);
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

    if (struct_defn->node->is_union && !(literal->head.head.flags & AST_FLAG_COMPILER_GENERATED)) {
        if (!check_union_literal(typer, literal, struct_defn)) return NULL;
    }

    DynamicArray members = struct_defn->node->members->identifiers;
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
                if (!any_cast) return NULL;
                init->value = (AstExpr *) any_cast;
            }

            if (!types_are_equal(member_type, init->value->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign member '%s' of type '%s' to value of type '%s'", member->name, type_to_str(member_type), type_to_str(init->value->type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(struct_defn->node), "Here is the definition of '%s'", struct_defn->identifier->name);
                return NULL;
            }

            bool ok = leads_to_integer_overflow(typer, member_type, init->value);
            if (!ok) return NULL;

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
                if (!any_cast) return NULL;
                init->value = (AstExpr *) any_cast;
            }

            if (!types_are_equal(member_type, init->value->type)) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(init->value), "Type mismatch. Trying to assign to member '%s' of type '%s' to value of type '%s'", member->name, type_to_str(member_type), type_to_str(init->value->type));
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(member), "Here is the member '%s' in %s", member->name, struct_defn->identifier->name);
                return NULL;
            }

            bool ok = leads_to_integer_overflow(typer, member_type, init->value);
            if (!ok) return NULL;

            init->member = member;
            curr_member_index += 1;
        }
    }

    // Sort the initializers by the member index
    qsort(literal->initializers.items, literal->initializers.count, sizeof(AstStructInitializer *), compare_struct_initializers);

    // Reserve space for the struct literal
    if (typer->enclosing_function) {
        reserve_temporary_storage(typer->enclosing_function, struct_defn->head.size);
    }

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
        // Structs can be compared member by member with == and !=
        if (binary->operator != TOKEN_DOUBLE_EQUAL && binary->operator != TOKEN_NOT_EQUAL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Binary operation between two structs is not yet supported! Only == and != are");
            return NULL;
        }
        if (strcmp(ti_lhs->name, ti_rhs->name) != 0) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Cannot compare a %s with a %s", text_bold(type_to_str(ti_lhs)), text_bold(type_to_str(ti_rhs)));
            return NULL;
        }

        if (((TypeStruct *)ti_lhs)->node->is_union) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Unions can't be compared with %s", binary->operator == TOKEN_DOUBLE_EQUAL ? "==" : "!=");
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Use a switch to see which variant it holds");
            return NULL;
        }

        AstIdentifier *bad_member = NULL;
        if (!struct_is_comparable((TypeStruct *)ti_lhs, &bad_member)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Cannot compare %s with %s, as its member '%s' of type %s cannot be compared",
                text_bold(type_to_str(ti_lhs)), binary->operator == TOKEN_DOUBLE_EQUAL ? "==" : "!=", bad_member->name, type_to_str(bad_member->type));
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "Numbers, bools, enums, pointers, strings and structs of those can be compared");
            return NULL;
        }
        return primitive_type(PRIMITIVE_BOOL);
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
        if (lhs == TYPE_BOOL && rhs == TYPE_BOOL) {
            if (binary->operator == TOKEN_DOUBLE_EQUAL || binary->operator == TOKEN_NOT_EQUAL) {
                return primitive_type(PRIMITIVE_BOOL);   
            }
        }

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

        if (lhs == TYPE_STRING && rhs == TYPE_STRING)   return primitive_type(PRIMITIVE_BOOL);

        report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Type '%s' and '%s' are not comparable with operator '%s'", type_to_str(ti_lhs), type_to_str(ti_rhs), token_type_to_str(binary->operator));
        return NULL;
    }

    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(binary), "Type '%s' and '%s' is not compatible with operator %s\n", type_to_str(ti_lhs), type_to_str(ti_rhs), token_type_to_str(binary->operator));
    return NULL;
}

Type *check_unary(Typer *typer, AstUnary *unary, Type *ctx_type) {
    typer->checking_place = unary->operator == OP_ADDRESS_OF; // &xs[i] is the address of the element in the collection
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
        else if (unary->head.head.flags & AST_FLAG_COMPILER_GENERATED) {
            // e.g the receiver of make_map().set(k, v). The value is put in a temporary, and its address is passed
            lvalue = true;
        }

        if (!(unary->head.head.flags & AST_FLAG_COMPILER_GENERATED)) {
            AstFunctionDefn *readonly = readonly_self_place(typer, unary->expr);
            if (!readonly) readonly = readonly_self_method(typer, unary->expr);
            if (readonly) {
                StringBuilder sb = sb_init(32);
                append_place_text(&sb, unary->expr);

                report_readonly_self(typer, (Ast *)unary, readonly, "take the address of", unary->expr);
                report_error_ast(typer->parser, LABEL_NOTE, NULL, "The pointer could be used to change self. Copy the value into a local instead e.g copy := %s", sb_to_string(&sb));
                return NULL;
            }
        }

        if (!lvalue) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(unary->expr), "Expression is not an addressable value (lvalue)");
            return NULL;
        }

        TypePointer *ptr = generate_pointer_to_type(typer->parser, expr_type);
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
    // Note: bool -> float is intentionally not allowed. Cast to an integer first

    if (from->kind == TYPE_POINTER && to->kind == TYPE_INTEGER) return true;    // address to a u64 number
    if (from->kind == TYPE_POINTER && to->kind == TYPE_BOOL) return true;       // null pointer = false, otherwise = true

    // if (from->kind == TYPE_POINTER && to->kind == TYPE_STRING) {
    //     TypePointer *from_ptr = (TypePointer *)from;
    //     if (from_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)from_ptr->pointer_to)->kind == PRIMITIVE_U8) {
    //         return true;     // allow string to *u8
    //     }
    //     if (from_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)from_ptr->pointer_to)->kind == PRIMITIVE_CHAR) {
    //         return true;     // allow string to *char
    //     }
    // }
    if (from->kind == TYPE_STRING && to->kind == TYPE_POINTER) {
        TypePointer *to_ptr = (TypePointer *)to;
        if (to_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)to_ptr->pointer_to)->kind == PRIMITIVE_U8) {
            return true;     // allow string to *u8
        }
        if (to_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)to_ptr->pointer_to)->kind == PRIMITIVE_CHAR) {
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
        if (expr_type->kind == TYPE_BOOL && wanted_type->kind == TYPE_FLOAT) {
            report_error_ast(typer->parser, LABEL_NOTE, NULL, "Booleans can not be casted directly to floats. Cast to an integer first, e.g. `(value as int) as %s`", type_to_str(wanted_type));
        }
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

bool is_non_constant_identifier_found_inside_current_function(Typer *typer, AstIdentifier *ident) {
    if (!(ident->flags & IDENTIFIER_IS_CONSTANT) 
    && typer->enclosing_function && typer->enclosing_function != ident->scope->enclosing_function) 
    {
        return false;
    }

    return true;
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
    case LITERAL_BOOLEAN:   return (ctx_type && ctx_type->kind == TYPE_BOOL)    ? ctx_type : primitive_type(PRIMITIVE_BOOL);
    case LITERAL_NULL:      return (Type *) type_defn_null_ptr;
    case LITERAL_STRING: {
        if (type_defn_string->struct_defn == NULL) {
            Type *ok = resolve_type(typer, (Type *) type_defn_string);
            if (!ok) return NULL;
        }
        if (typer->enclosing_function) {
            reserve_temporary_storage(typer->enclosing_function, 16);
        }
        return (ctx_type && ctx_type->kind == TYPE_STRING) ? ctx_type : (Type *) type_defn_string;
    }
    case LITERAL_IDENTIFIER: {
        char   *ident_name   = literal->as.value.identifier.name;
        AstIdentifier *ident = NULL;
        if (literal->as.value.identifier.resolved_identifier) {
            // Already resolved e.g the declaration of a qualified name like math.sin
            ident = literal->as.value.identifier.resolved_identifier;
        }
        else if (typer->current_scope->belongs_to_struct) {
            ident = lookup_from_scope(typer->parser, typer->current_scope->belongs_to_struct->static_members, ident_name);
        } else {
            ident = lookup_from_scope(typer->parser, typer->current_scope, ident_name);
        }

        if (ident == NULL) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Undeclared identifier '%s'", ident_name);

            // Inside a method declared in a struct, the members are only reached through self
            for (AstFunctionDefn *func = typer->enclosing_function; func; func = func->parent_function) {
                if (func->self_struct && get_struct_member(func->self_struct, ident_name)) {
                    report_error_ast(typer->parser, LABEL_HINT, NULL, "'%s' is a member of '%s'. Members are reached through self, e.g self.%s", ident_name, func->self_struct->identifier->name, ident_name);
                    break;
                }
            }
            return NULL;
        }

        if (ident->flags & IDENTIFIER_IS_NAMESPACE) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "'%s' is a namespace, not a value", ident->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Use one of its declarations e.g %s.name", ident->name);
            return NULL;
        }
        if (ident->flags & IDENTIFIER_IS_AMBIGUOUS) {
            AstIdentifier *first = ((AstIdentifier **)ident->ambiguous.items)[0];
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "'%s' is ambiguous. It is declared in more than one of the namespaces you use", ident->name);
            for (int i = 0; i < ident->ambiguous.count; i++) {
                AstIdentifier *candidate = ((AstIdentifier **)ident->ambiguous.items)[i];
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)candidate, "In '%s'", namespace_of(candidate));
            }
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Write it with its namespace e.g %s.%s", namespace_of(first), ident->name);
            return NULL;
        }

        // A constant of another file might not have been checked yet. A name from an import or another file of the namespace is a copy of the declared one
        AstIdentifier *declared = ident->imported_from ? ident->imported_from : ident;
        // Constants at the top of a file can be used anywhere, also before they are declared, e.g by a function above them
        AstFile *used_in   = literal->head.head.file ? literal->head.head.file : typer->current_file;
        bool     top_level = declared->head.file && declared->scope == declared->head.file->scope;
        if (declared->head.file && (declared->head.file != used_in || top_level) && declared->decl && (declared->decl->flags & DECLARATION_CONSTANT) && !(declared->flags & IDENTIFIER_IS_CONSTANT_FUNCTION_DEFN)) {
            bool ok = check_declaration_of(typer, declared);
            if (!ok) return NULL;
            if (ident->imported_from) {
                ident->type  = ident->imported_from->type;
                ident->value = ident->imported_from->value;
                ident->flags |= ident->imported_from->flags & IDENTIFIER_IS_RESOLVED;
            }
        }

        if (!is_non_constant_identifier_found_inside_current_function(typer, ident)) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)literal, "Undeclared identifier '%s'", ident->name);
            report_error_ast(typer->parser, LABEL_NOTE, (Ast *)typer->enclosing_function, "If you intended to use identifier '%s' inside this function then specify it as a parameter", ident->name, typer->enclosing_function->identifier->name);
            return NULL;
        }

        bool needs_ordering_check = true;

        if (ident->flags & IDENTIFIER_IS_CONSTANT_FUNCTION_DEFN) {
            needs_ordering_check = false;
        }

        // Types can be used before they are declared e.g Hue.Red before 'Hue :: Color;'
        if (ident->flags & IDENTIFIER_IS_TYPE_ALIAS) {
            needs_ordering_check = false;

            bool ok = check_declaration_of(typer, ident);
            if (!ok) return NULL;
        }

        if (needs_ordering_check) {
            bool unresolved = !(ident->flags && IDENTIFIER_IS_RESOLVED);
            if (is_a_before_b((Ast *) literal, (Ast *) ident) || unresolved) {
                report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "'%s' is used before its declared", ident->name);
                report_error_ast(typer->parser, LABEL_NOTE, (Ast *)(ident), "'%s' is being declared here", ident->name);
                return NULL;
            }
        }

        if (!ident->type) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Compiler Error: '%s' is unresolved", ident->name);
            return NULL;
        }

        if (ident->type->kind == TYPE_FUNCTION && is_generic_function(((TypeFunction *)ident->type)->node) && !typer->checking_callee) {
            report_error_ast(typer->parser, LABEL_ERROR, (Ast *)(literal), "Generic function '%s' can only be called, not used as a value", ident->name);
            report_error_ast(typer->parser, LABEL_HINT, NULL, "Wrap it in a non-generic function that calls it with the types you need, e.g (a, b: int) -> int { return %s(a, b); }", ident->name);
            return NULL;
        }
        typer->checking_callee = false;

        report_if_deprecated(typer, ident, (Ast *)literal);

        // Save the resolved identifier on the literal
        literal->as.value.identifier.resolved_identifier = ident;

        // Try and transform the constant identifier into a constant literal
        if (ident->flags & IDENTIFIER_IS_CONSTANT) {
            if (is_primitive_type(ident->type->kind)) {

                if (ident->value->head.kind != AST_LITERAL) {
                    report_error_ast(typer->parser, LABEL_ERROR, (Ast *)ident, "Compiler Error: Got a non literal when trying to transform constant identifier to a basic literal");
                    return NULL;
                }

                if (ident->type->kind == TYPE_INTEGER && ctx_type && ctx_type->kind == TYPE_FLOAT) {
                    AstLiteral *lit_value = (AstLiteral *) ident->value;

                    double float_value = (double) lit_value->as.value.integer;
    
                    literal->kind              = LITERAL_FLOAT;
                    literal->as.value.floating = float_value;
                    literal->head.type         = ctx_type;

                    return ctx_type;
                }
            }
        }

        return ident->type;
    }}

    XXX;
}

void push_enclosing_function(Typer *typer, AstFunctionDefn *func_defn) {
    da_append(&typer->enclosing_function_stack, func_defn);
    typer->enclosing_function = func_defn;

    da_append(&typer->current_file->flattened_function_defns, func_defn);
}

// @FutureMe @Beware: This keeping track of an enclosing function stack might need to be tested thougholy as we can jump
// to a completely different function unrelated to the current function stack f.x when typechecking a function call.
// In this case we should either remember the enclosing function stack in restore_typer_state to account for this.
void pop_enclosing_function(Typer *typer) {
    da_remove(&typer->enclosing_function_stack, -1);
    if (typer->enclosing_function_stack.count == 0) {
        typer->enclosing_function = NULL;
    } else {
        typer->enclosing_function = da_get_deref(typer->enclosing_function_stack, -1);
    }
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
    return find_identifier_in_scope(struct_defn->members, name);
}

AstIdentifier *get_struct_method(AstStruct *struct_defn, char *name) {
    return find_identifier_in_scope(struct_defn->methods, name);
}

bool is_arithmetic_operator(TokenType op) {
    if (op == '+') return true;
    if (op == '-') return true;
    if (op == '*') return true;
    if (op == '/') return true;
    if (op == '%') return true;
    return false;
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
    if (op == '!')                 return true;
    return false;
}

bool is_literal_identifier(AstExpr *expr) {
    return expr && expr->head.kind == AST_LITERAL && ((AstLiteral *)expr)->kind == LITERAL_IDENTIFIER;
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