#include "ast.h"

#define TABLE_IMPLEMENTATION
#define STRING_BUILDER_IMPLEMENTATION
#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "lib/table.h"
#include "lib/string_builder.h"
#include "lib/dynamic_array.h"

TypePointer *t_nil_ptr = &(TypePointer){
    .head = {.kind = TYPE_POINTER, .size = 8},
    .pointer_to = (Type *)&(TypePrimitive){.kind = PRIMITIVE_VOID, .head = {.kind = TYPE_VOID, .size = 0}}
};

bool compare_user_types(const void *key, const void *item) {
    Type *ti = (*(Type **)(item));
    return strcmp((const char *)(key), ti->as.name) == 0;
}

TypeTable type_table_init(void) {
    TypeTable type_table  = {0};
    type_table.user_types = hash_table_init(32, sizeof(Type *), compare_user_types);
    type_table.types      = arena_init(sizeof(Type) * 128);
    return type_table;
}

Type *type_lookup(TypeTable *tt, char *name) {
    Type **found = hash_table_get(&tt->user_types, name);
    if (found != NULL) {
        return *found;
    }
    return NULL;
}

Type *type_add_user_defined(TypeTable *tt, Type *type) {
    Type *existing = (Type *)(hash_table_get(&tt->user_types, type->as.name));
    if (existing) {
        return existing;
    }

    hash_table_add(&tt->user_types, type->as.name, &type);
    return NULL;
}

void *type_alloc(TypeTable *tt, size_t size) {
    return arena_allocate(&tt->types, size);
}

char *type_to_str(Type *type) {
    switch (type->kind) {
    case TYPE_INVALID:
    case TYPE_VOID:
    case TYPE_BOOL:
    case TYPE_INTEGER:
    case TYPE_FLOAT:
    case TYPE_STRING: {
        TypePrimitive *prim = (TypePrimitive *)(type);
        return prim->name;
    }
    case TYPE_POINTER: {
        TypePointer *ptr = (TypePointer *)(type);
        StringBuilder sb = sb_init(32);
        sb_append(&sb, "*%s", type_to_str(ptr->pointer_to));

        return sb_to_string(&sb); // @Leak
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *)(type);
        StringBuilder sb = sb_init(32);
        sb_append(&sb, "[]%s", type_to_str(array->elem_type));

        return sb_to_string(&sb); // @Leak
    }
    case TYPE_NAME:      return type->as.name;
    case TYPE_STRUCT:    return type->as.name;
    case TYPE_ENUM:      return type->as.name;
    case TYPE_FUNCTION: {
        TypeFunction *func = (TypeFunction *)(type);
        StringBuilder sb = sb_init(32);

        sb_append(&sb, "(");
        for (int i = 0; i < func->node->parameters.count; i++) {
            AstDeclaration *param = ((AstDeclaration **)func->node->parameters.items)[i];
            sb_append(&sb, "%s", type_to_str(param->type));
            if (i + 1 < func->node->parameters.count) {
                sb_append(&sb, ", ");
            }
        }
        sb_append(&sb, ")");

        sb_append(&sb, " -> %s", type_to_str(func->node->return_type));
        return sb_to_string(&sb); // @Leak
    }
    }

    printf("internal compiler error: Unknown type kind: %d in type_to_str()", type->kind);
    exit(1);
}

bool is_primitive_type(TypeKind kind) {
    if (kind == TYPE_VOID)    return true;
    if (kind == TYPE_BOOL)    return true;
    if (kind == TYPE_INTEGER) return true;
    if (kind == TYPE_FLOAT)   return true;
    if (kind == TYPE_STRING)  return true;

    return false;
}

bool is_signed_integer(Type *type) {
    assert(type->kind == TYPE_INTEGER);

    TypePrimitive *prim = (TypePrimitive *)(type);
    switch (prim->kind) {
    case PRIMITIVE_INT:
    case PRIMITIVE_S8:
    case PRIMITIVE_S16:
    case PRIMITIVE_S32:
    case PRIMITIVE_S64:
        return true;
    default:
        return false;
    }
}