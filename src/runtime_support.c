//////////////////////////////////
// Runtime support file that is linked with the user program executable
//////////////////////////////////

#include <stdio.h>

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
    PRIMITIVE_INVALID,

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

typedef struct Type {
    TypeKind kind;
    int      size;
    int      flags;
} Type;

typedef struct TypePrimitive {
    Type           head;
    PrimitiveKind  kind;
    char          *name;
} TypePrimitive;

TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
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
    {.kind = PRIMITIVE_STRING,  .name = "string",    .head = {.kind = TYPE_STRING,  .size = 8}},
    {.kind = PRIMITIVE_BOOL,    .name = "bool",      .head = {.kind = TYPE_BOOL,    .size = 1}},
    {.kind = PRIMITIVE_VOID,    .name = "void",      .head = {.kind = TYPE_VOID,    .size = 0}},
    {.kind = PRIMITIVE_UNTYPED_INT,   .name = "number", .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_UNTYPED_FLOAT, .name = "untyped(float)", .head = {.kind = TYPE_FLOAT, .size = 4}},
};

// TypePrimitive *Type_invalid       = &primitive_types[PRIMITIVE_INVALID];
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
