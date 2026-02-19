#include "ast.h"

#define TABLE_IMPLEMENTATION
#define STRING_BUILDER_IMPLEMENTATION
#include "lib/table.h"
#include "lib/string_builder.h"


const char *ast_to_str(Ast *ast) {
    switch (ast->kind) {
    case AST_ERR:                return "AST_ERR";
    case AST_FILE:               return "AST_FILE";
    case AST_BLOCK:              return "AST_BLOCK";
    case AST_DECLARATION:        return "AST_DECLARATION";
    case AST_ASSIGNMENT:         return "AST_ASSIGNMENT";
    case AST_DIRECTIVE:          return "AST_DIRECTIVE";
    case AST_IMPORT:             return "AST_IMPORT";
    case AST_EXTERN:             return "AST_EXTERN";
    case AST_PRINT:              return "AST_PRINT";
    case AST_ASSERT:             return "AST_ASSERT";
    case AST_TYPEOF:             return "AST_TYPEOF";
    case AST_SIZEOF:             return "AST_SIZEOF";
    case AST_STRUCT:             return "AST_STRUCT";
    case AST_STRUCT_LITERAL:     return "AST_STRUCT_LITERAL";
    case AST_STRUCT_INITIALIZER: return "AST_STRUCT_INITIALIZER";
    case AST_ARRAY_LITERAL:      return "AST_ARRAY_LITERAL";
    case AST_ENUM:               return "AST_ENUM";
    case AST_ENUMERATOR:         return "AST_ENUMERATOR";
    case AST_ENUM_LITERAL:       return "AST_ENUM_LITERAL";
    case AST_FUNCTION_DEFN:      return "AST_FUNCTION_DEFN";
    case AST_FUNCTION_CALL:      return "AST_FUNCTION_CALL";
    case AST_ARGUMENT:           return "AST_ARGUMENT";
    case AST_IF:                 return "AST_IF";
    case AST_RETURN:             return "AST_RETURN";
    case AST_FOR:                return "AST_FOR";
    case AST_WHILE:              return "AST_WHILE";
    case AST_BREAK_OR_CONTINUE:  return "AST_BREAK_OR_CONTINUE";
    case AST_EXPR:               return "AST_EXPR";
    case AST_EXPR_STMT:          return "AST_EXPR_STMT";
    case AST_RANGE_EXPR:         return "AST_RANGE_EXPR";
    case AST_SEMICOLON_EXPR:     return "AST_SEMICOLON_EXPR";
    case AST_BINARY:             return "AST_BINARY";
    case AST_UNARY:              return "AST_UNARY";
    case AST_CAST:               return "AST_CAST";
    case AST_NEW:                return "AST_NEW";
    case AST_LITERAL:            return "AST_LITERAL";
    case AST_SUBSCRIPT:          return "AST_SUBSCRIPT";
    case AST_ACCESSOR:           return "AST_ACCESSOR";
    case AST_MEMBER_ACCESS:      return "AST_MEMBER_ACCESS";
    case AST_ARRAY_ACCESS:       return "AST_ARRAY_ACCESS";
    case AST_IDENTIFIER:         return "AST_IDENTIFIER";
    case AST_TYPE:               return "AST_TYPE";
    }
    printf("%s:%d: compiler-error: Could not give the name of AST node with type id %d\n", __FILE__, __LINE__, ast->kind);
    exit(1);
}

// @Note - maybe make each of these into a global variable instead with prefix t_<TYPE>
TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
    {.kind = PRIMITIVE_INVALID, .name = "invalid",   .head = {.kind = TYPE_INVALID, .size = 0}},
    {.kind = PRIMITIVE_UINT,    .name = "uint",      .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U8,      .name = "u8",        .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_U16,     .name = "u16",       .head = {.kind = TYPE_INTEGER, .size = 2}},
    {.kind = PRIMITIVE_U32,     .name = "u32",       .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U64,     .name = "u64",       .head = {.kind = TYPE_INTEGER, .size = 8}},
    {.kind = PRIMITIVE_INT,     .name = "int",       .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_S8,      .name = "s8",        .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_S16,     .name = "s16",       .head = {.kind = TYPE_INTEGER, .size = 2}},
    {.kind = PRIMITIVE_S32,     .name = "s32",       .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_S64,     .name = "s64",       .head = {.kind = TYPE_INTEGER, .size = 8}},
    {.kind = PRIMITIVE_FLOAT,   .name = "float",     .head = {.kind = TYPE_FLOAT,   .size = 4}},
    {.kind = PRIMITIVE_F32,     .name = "f32",       .head = {.kind = TYPE_FLOAT,   .size = 4}},
    {.kind = PRIMITIVE_F64,     .name = "f64",       .head = {.kind = TYPE_FLOAT,   .size = 8}},
    {.kind = PRIMITIVE_CHAR,    .name = "char",      .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_BOOL,    .name = "bool",      .head = {.kind = TYPE_BOOL,    .size = 1}},
    {.kind = PRIMITIVE_VOID,    .name = "void",      .head = {.kind = TYPE_VOID,    .size = 0}},
    {.kind = PRIMITIVE_UNTYPED_INT,   .name = "number", .head = {.kind = TYPE_INTEGER, .size = 4}}, // Should probably match input - Alexander
    {.kind = PRIMITIVE_UNTYPED_FLOAT, .name = "untyped(float)", .head = {.kind = TYPE_FLOAT, .size = 4}},
};


Type *primitive_type(PrimitiveKind kind) {
    return (Type *)(&primitive_types[kind]);
}

const char *directive_names[] = {
    "invalid",
    "import",
    "extern"
};

TypePointer *type_defn_null_ptr = &(TypePointer){
    .head = {.kind = TYPE_POINTER, .size = 8},
    .pointer_to = (Type *)&(TypePrimitive){.kind = PRIMITIVE_VOID, .name = "void", .head = {.kind = TYPE_VOID, .size = 0}}
};

TypeString *type_defn_string = &(TypeString){
    .head = {.kind = TYPE_STRING, .size = 16},
    .struct_defn = NULL
};

TypeArray *type_defn_string_as_slice = &(TypeArray){
    .head = {
        .kind = TYPE_ARRAY, 
        .size = 16
    },
    .array_kind = ARRAY_SLICE,
    .elem_type = (Type *) &primitive_types[PRIMITIVE_CHAR],
    .struct_defn = NULL,
    .capacity_expr = NULL,
    .capacity = 0,
    .count = 0,
};

bool compare_user_types(const void *key, const void *item) {
    Type *ti = (*(Type **)(item));
    return strcmp((const char *)(key), ti->name) == 0;
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
    Type *existing = (Type *)(hash_table_get(&tt->user_types, type->name));
    if (existing) {
        return existing;
    }

    hash_table_add(&tt->user_types, type->name, &type);
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
    case TYPE_FLOAT: {
        TypePrimitive *prim = (TypePrimitive *)(type);
        return prim->name;
    }
    case TYPE_STRING: {
        return "string";
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

        switch (array->array_kind) {
        case ARRAY_FIXED: {
            sb_append(&sb, "[%d]%s", array->capacity, type_to_str(array->elem_type));
            break;
        }
        case ARRAY_SLICE: {
            sb_append(&sb, "[]%s", type_to_str(array->elem_type));
            break;
        }
        case ARRAY_DYNAMIC: {
            sb_append(&sb, "[..]%s", type_to_str(array->elem_type));
            break;
        }
        }

        return sb_to_string(&sb); // @Leak
    }
    case TYPE_NAME:      return type->name;
    case TYPE_STRUCT:    return type->name;
    case TYPE_ENUM:      return "enum";
    case TYPE_FUNCTION: {
        TypeFunction *func = (TypeFunction *)(type);
        StringBuilder sb = sb_init(32);

        if (func->node->is_method) {
            sb_append(&sb, "method ");    
        }

        sb_append(&sb, "(");
        for (int i = 0; i < func->node->parameters.count; i++) {
            AstIdentifier *param = ((AstIdentifier **)func->node->parameters.items)[i];
            sb_append(&sb, "%s", type_to_str(param->type));
            if (i + 1 < func->node->parameters.count) {
                sb_append(&sb, ", ");
            }
        }
        sb_append(&sb, ")");

        if (func->node->return_types.count == 1) {
            Type *return_type = ((Type **)func->node->return_types.items)[0];
            sb_append(&sb, " -> %s", type_to_str(return_type));
        } else {
            sb_append(&sb, " -> (");
            for (int i = 0; i < func->node->return_types.count; i++) {
                Type *return_type = ((Type **)func->node->return_types.items)[i];
                sb_append(&sb, "%s", type_to_str(return_type));
            }
            sb_append(&sb, ")");
        }
        return sb_to_string(&sb); // @Leak
    }
    case TYPE_TUPLE: {
        TypeTuple *tuple = (TypeTuple *)type;

        if (tuple->types.count == 1) {
            Type *t = ((Type **)tuple->types.items)[0];
            return type_to_str(t);
        }

        StringBuilder sb = sb_init(32);

        sb_append(&sb, "(");
        for (int i = 0; i < tuple->types.count; i++) {
            Type *t = ((Type **)tuple->types.items)[i];

            char *type_str = type_to_str(t);
            sb_append(&sb, "%s", type_str);
            
            if (i != tuple->types.count - 1) {
                sb_append(&sb, ", ");
            }

        }
        sb_append(&sb, ")");
        return sb_to_string(&sb); // @Leak
    }
    case TYPE_VARIADIC: {
        TypeVariadic *variadic = (TypeVariadic *)type;

        StringBuilder sb = sb_init(32);
        sb_append(&sb, "...");
        char *type_str = type_to_str(variadic->type);
        sb_append(&sb, "%s", type_str);
        
        return sb_to_string(&sb); // @Leak
    }
    case TYPE_ANY: {
        return "any";
    }
    }

    printf("Internal Compiler Error: Unknown type kind: %d in type_to_str()", type->kind);
    exit(1);
}

bool is_primitive_type(TypeKind kind) {
    if (kind == TYPE_VOID)    return true;
    if (kind == TYPE_BOOL)    return true;
    if (kind == TYPE_INTEGER) return true;
    if (kind == TYPE_FLOAT)   return true;

    return false;
}


bool is_signed_integer(Type *type) {
    if (type->kind != TYPE_INTEGER) return false;

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

bool is_unsigned_integer(Type *type) {
    if (type->kind != TYPE_INTEGER) return false;

    TypePrimitive *prim = (TypePrimitive *)(type);
    switch (prim->kind) {
    case PRIMITIVE_CHAR:
    case PRIMITIVE_UINT:
    case PRIMITIVE_U8:
    case PRIMITIVE_U16:
    case PRIMITIVE_U32:
    case PRIMITIVE_U64:
        return true;
    default:
        return false;
    }
}