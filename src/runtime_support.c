//////////////////////////////////
// Runtime support file that is linked with the user program executable
//////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

// @Important: This has to match the TypeKind in ast.h
typedef enum TypeKind {
    TYPE_VOID,
    TYPE_BOOL, 
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FUNCTION,
    TYPE_TUPLE,
    TYPE_VARIADIC,
    TYPE_ANY,
} TypeKind;

typedef enum PrimitiveKind {
    // PRIMITIVE_INVALID,

    PRIMITIVE_UINT, // alias for u32
    PRIMITIVE_U8,
    PRIMITIVE_U16,
    PRIMITIVE_U32,
    PRIMITIVE_U64,
    PRIMITIVE_INT, // alias for s32
    PRIMITIVE_S8,
    PRIMITIVE_S16,
    PRIMITIVE_S32,
    PRIMITIVE_S64,

    PRIMITIVE_FLOAT, // alias for f32
    PRIMITIVE_F32,
    PRIMITIVE_F64,

    PRIMITIVE_STRING,
    PRIMITIVE_BOOL,
    PRIMITIVE_VOID,

    PRIMITIVE_UNTYPED_INT,
    PRIMITIVE_UNTYPED_FLOAT,

    PRIMITIVE_COUNT,
} PrimitiveKind;

typedef enum ArrayKind {
    ARRAY_FIXED,
    ARRAY_SLICE,
    ARRAY_DYNAMIC,
} ArrayKind;

typedef struct Type {
    TypeKind kind;
    char    *name;
    int      size;
    int      flags;
} Type;

typedef struct TypePrimitive {
    Type           head;
    PrimitiveKind  kind;
} TypePrimitive;

typedef struct TypeArray {
    Type      head;
    ArrayKind kind;
    Type     *elem_type;
    int       count;
} TypeArray;

typedef struct TypeEnum {
    Type           head;
    Type          *backing_type;
    int            min_value;
    int            max_value;
} TypeEnum;

typedef struct StructMember {
    char *name;
    Type *type;
    int   index;
    int   offset;
} StructMember;

typedef struct TypeStruct {
    Type          head;
    StructMember *members;
    int           members_count;
    int           alignment;
} TypeStruct;

TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = "uint"   }, .kind = PRIMITIVE_UINT     },
    {.head = {.kind = TYPE_INTEGER, .size = 1, .name = "u8"     }, .kind = PRIMITIVE_U8       },
    {.head = {.kind = TYPE_INTEGER, .size = 2, .name = "u16"    }, .kind = PRIMITIVE_U16      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = "u32"    }, .kind = PRIMITIVE_U32      },
    {.head = {.kind = TYPE_INTEGER, .size = 8, .name = "u64"    }, .kind = PRIMITIVE_U64      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = "int"    }, .kind = PRIMITIVE_INT      },
    {.head = {.kind = TYPE_INTEGER, .size = 1, .name = "s8"     }, .kind = PRIMITIVE_S8       },
    {.head = {.kind = TYPE_INTEGER, .size = 2, .name = "s16"    }, .kind = PRIMITIVE_S16      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = "s32"    }, .kind = PRIMITIVE_S32      },
    {.head = {.kind = TYPE_INTEGER, .size = 8, .name = "s64"    }, .kind = PRIMITIVE_S64      },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = "float"  }, .kind = PRIMITIVE_FLOAT    },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = "f32"    }, .kind = PRIMITIVE_F32      },
    {.head = {.kind = TYPE_FLOAT,   .size = 8, .name = "f64"    }, .kind = PRIMITIVE_F64      },
    {.head = {.kind = TYPE_STRING,  .size = 8, .name = "string" }, .kind = PRIMITIVE_STRING   },
    {.head = {.kind = TYPE_BOOL,    .size = 1, .name = "bool"   }, .kind = PRIMITIVE_BOOL     },
    {.head = {.kind = TYPE_VOID,    .size = 0, .name = "void"   }, .kind = PRIMITIVE_VOID     },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = "number" }, .kind = PRIMITIVE_UNTYPED_INT  },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = "untyped(float)"}, .kind = PRIMITIVE_UNTYPED_FLOAT},
};

TypePrimitive *Type_uint          = &primitive_types[PRIMITIVE_UINT];
TypePrimitive *Type_u8            = &primitive_types[PRIMITIVE_U8];
TypePrimitive *Type_u16           = &primitive_types[PRIMITIVE_U16];
TypePrimitive *Type_u32           = &primitive_types[PRIMITIVE_U32];
TypePrimitive *Type_u64           = &primitive_types[PRIMITIVE_U64];
TypePrimitive *Type_int           = &primitive_types[PRIMITIVE_INT];
TypePrimitive *Type_s8            = &primitive_types[PRIMITIVE_S8];
TypePrimitive *Type_s16           = &primitive_types[PRIMITIVE_S16];
TypePrimitive *Type_s32           = &primitive_types[PRIMITIVE_S32];
TypePrimitive *Type_s64           = &primitive_types[PRIMITIVE_S64];
TypePrimitive *Type_float         = &primitive_types[PRIMITIVE_FLOAT];
TypePrimitive *Type_f32           = &primitive_types[PRIMITIVE_F32];
TypePrimitive *Type_f64           = &primitive_types[PRIMITIVE_F64];
TypePrimitive *Type_string        = &primitive_types[PRIMITIVE_STRING];
TypePrimitive *Type_bool          = &primitive_types[PRIMITIVE_BOOL];
TypePrimitive *Type_void          = &primitive_types[PRIMITIVE_VOID];
TypePrimitive *Type_untyped_int   = &primitive_types[PRIMITIVE_UNTYPED_INT];
TypePrimitive *Type_untyped_float = &primitive_types[PRIMITIVE_UNTYPED_FLOAT];

// @Speed @Improvement: We want to do something better than heap allocating every runtime type!
TypeArray *runtime_get_type_array(char *name, ArrayKind kind, Type *elem_type, int count) {
    TypeArray *type_array  = malloc(sizeof(TypeArray));
    type_array->head.kind  = TYPE_ARRAY;
    type_array->head.name  = name;
    type_array->head.size  = elem_type->size * count;
    type_array->head.flags = 0;
    type_array->kind       = kind;
    type_array->elem_type  = elem_type;
    type_array->count      = count;
    return type_array;
}

TypeEnum *runtime_get_type_enum(char *name, Type *backing_type, int min_value, int max_value) {
    TypeEnum *type_enum     = malloc(sizeof(TypeEnum));
    type_enum->head.kind    = TYPE_ENUM;
    type_enum->head.name    = name;
    type_enum->head.size    = backing_type->size;
    type_enum->head.flags   = 0;
    type_enum->backing_type = backing_type;
    type_enum->min_value    = min_value;
    type_enum->max_value    = max_value;
    return type_enum;
}

TypeStruct *runtime_get_type_struct(char *name, StructMember *members, int members_count, int size, int alignment) {
    TypeStruct *type_struct    = malloc(sizeof(TypeStruct));
    type_struct->head.kind     = TYPE_STRUCT;
    type_struct->head.name     = name;
    type_struct->head.size     = size;
    type_struct->head.flags    = 0;
    type_struct->members       = members;
    type_struct->members_count = members_count;
    type_struct->alignment     = alignment;
    return type_struct;
}