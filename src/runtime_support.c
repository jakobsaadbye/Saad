//////////////////////////////////
// Runtime support file that is linked with the user program executable
//
// @Important:
//
// The struct layout of these types HAS TO match 1:1 the memory layout
// of the types found in "reflect.sd"
//////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct RawDynamicArray {
    unsigned char *data;
    int64_t        count;
    int64_t        capacity;
    int64_t        elem_size;
} RawDynamicArray;

typedef struct RawString {
    char   *data;
    int64_t len;
} RawString;

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

    PRIMITIVE_CHAR,
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
    RawString name;
    TypeKind  kind;
    int       size;
    int       flags;
} Type;

typedef struct TypePrimitive {
    Type           head;
    PrimitiveKind  kind;
} TypePrimitive;

typedef struct TypePointer {
    Type      head;
    Type     *pointer_to;
} TypePointer;

typedef struct TypeArray {
    Type      head;
    int       kind; // ArrayKind
    Type     *elem_type;
    int64_t   count;
} TypeArray;

typedef struct TypeEnum {
    Type           head;
    RawString      value_name;
    Type          *backing_type;
    int            min_value;
    int            max_value;
} TypeEnum;

typedef struct StructMember {
    RawString name;
    Type     *type;
    void     *value;
    int64_t   index;
    int64_t   offset;
} StructMember;

typedef struct Slice {
    void    *data;
    int64_t  count;
} Slice;

typedef struct TypeStruct {
    Type       head;        // offset 0, size 32
    // uint8_t   _pad0[8];     // pad head → 32 (16-byte alignment)
    Slice      members;     // offset 32, size 16
    int32_t    alignment;   // offset 48, size 4
    // int32_t   _pad1;        // pad to 8
    uint64_t  _pad2;        // tail padding → 64
} TypeStruct;

typedef struct TypeAny {
    Type  head;
    void *data;
    Type *type;
} TypeAny;

typedef struct TypeString {
    Type    head;
    char   *data;
    int64_t len;
} TypeString;

TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = {"uint",     4} }, .kind = PRIMITIVE_UINT     },
    {.head = {.kind = TYPE_INTEGER, .size = 1, .name = {"u8",       2} }, .kind = PRIMITIVE_U8       },
    {.head = {.kind = TYPE_INTEGER, .size = 2, .name = {"u16",      3} }, .kind = PRIMITIVE_U16      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = {"u32",      3} }, .kind = PRIMITIVE_U32      },
    {.head = {.kind = TYPE_INTEGER, .size = 8, .name = {"u64",      3} }, .kind = PRIMITIVE_U64      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = {"int",      3} }, .kind = PRIMITIVE_INT      },
    {.head = {.kind = TYPE_INTEGER, .size = 1, .name = {"s8",       2} }, .kind = PRIMITIVE_S8       },
    {.head = {.kind = TYPE_INTEGER, .size = 2, .name = {"s16",      3} }, .kind = PRIMITIVE_S16      },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = {"s32",      3} }, .kind = PRIMITIVE_S32      },
    {.head = {.kind = TYPE_INTEGER, .size = 8, .name = {"s64",      3} }, .kind = PRIMITIVE_S64      },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = {"float",    5} }, .kind = PRIMITIVE_FLOAT    },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = {"f32",      3} }, .kind = PRIMITIVE_F32      },
    {.head = {.kind = TYPE_FLOAT,   .size = 8, .name = {"f64",      3} }, .kind = PRIMITIVE_F64      },
    {.head = {.kind = TYPE_INTEGER, .size = 1, .name = {"char",     4} }, .kind = PRIMITIVE_CHAR     },
    {.head = {.kind = TYPE_BOOL,    .size = 1, .name = {"bool",     4} }, .kind = PRIMITIVE_BOOL     },
    {.head = {.kind = TYPE_VOID,    .size = 0, .name = {"void",     4} }, .kind = PRIMITIVE_VOID     },
    {.head = {.kind = TYPE_INTEGER, .size = 4, .name = {"number",   6} }, .kind = PRIMITIVE_UNTYPED_INT  },
    {.head = {.kind = TYPE_FLOAT,   .size = 4, .name = {"untyped(float)", 15} }, .kind = PRIMITIVE_UNTYPED_FLOAT},
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
TypePrimitive *Type_char          = &primitive_types[PRIMITIVE_CHAR];
TypePrimitive *Type_bool          = &primitive_types[PRIMITIVE_BOOL];
TypePrimitive *Type_void          = &primitive_types[PRIMITIVE_VOID];
TypePrimitive *Type_untyped_int   = &primitive_types[PRIMITIVE_UNTYPED_INT];
TypePrimitive *Type_untyped_float = &primitive_types[PRIMITIVE_UNTYPED_FLOAT];

TypeString *Type_string = &(TypeString){
    .head = {
        .kind = TYPE_STRING, 
        .size = 16,
        .name = {
            .data = "string",
            .len  = 6,
        }
    },
    .data = NULL,
    .len  = 0,
};

TypePointer *runtime_get_type_pointer(RawString name, Type *pointer_to) {
    TypePointer *type_ptr = malloc(sizeof(TypePointer));
    type_ptr->head.kind   = TYPE_POINTER;
    type_ptr->head.name   = name;
    type_ptr->head.size   = 8;
    type_ptr->head.flags  = 0;
    type_ptr->pointer_to  = pointer_to;
    return type_ptr;
}

// @Speed @Improvement: We want to do something better than heap allocating every runtime type!
TypeArray *runtime_get_type_array(RawString name, int kind, Type *elem_type, int64_t count) {
    TypeArray *type_array  = malloc(sizeof(TypeArray));
    type_array->head.kind  = TYPE_ARRAY;
    type_array->head.name  = name;
    type_array->head.size  = kind == ARRAY_FIXED ? elem_type->size * count : kind == ARRAY_SLICE ? 16 : 32;
    type_array->head.flags = 0;
    type_array->kind       = kind;
    type_array->elem_type  = elem_type;
    type_array->count      = count;
    return type_array;
}

TypeEnum *runtime_get_type_enum(RawString name, RawString *value_name, Type *backing_type, int min_value, int max_value) {
    TypeEnum *type_enum     = malloc(sizeof(TypeEnum));
    type_enum->head.kind    = TYPE_ENUM;
    type_enum->head.name    = name;
    type_enum->head.size    = backing_type->size;
    type_enum->head.flags   = 0;
    type_enum->backing_type = backing_type;
    type_enum->min_value    = min_value;
    type_enum->max_value    = max_value;

    type_enum->value_name.data = malloc(value_name->len + 1);
    strcpy(type_enum->value_name.data, value_name->data);
    type_enum->value_name.len = value_name->len;

    return type_enum;
}

TypeStruct *runtime_get_type_struct(RawString name, StructMember *members, int members_count, int size, int alignment) {
    TypeStruct *type_struct    = malloc(sizeof(TypeStruct));
    type_struct->head.kind     = TYPE_STRUCT;
    type_struct->head.name     = name;
    type_struct->head.size     = size;
    type_struct->head.flags    = 0;
    type_struct->members.data  = members;
    type_struct->members.count = (size_t) members_count;
    type_struct->alignment     = alignment;
    return type_struct;
}

TypeAny *runtime_get_type_any() {
    TypeAny *type_any       = malloc(sizeof(TypeAny));
    type_any->head.kind     = TYPE_ANY;
    type_any->head.name     = (RawString){"any", 3};
    type_any->head.size     = 16;
    type_any->head.flags    = 0;
    type_any->data          = NULL;
    type_any->type          = NULL;
    return type_any;
}

void runtime_builtin_append(RawDynamicArray *da, void *item, bool by_value) {
    int64_t tmp;

    if (by_value) {
        tmp = (int64_t) item;
        item = &tmp;
    }

    if (da->count + 1 > da->capacity) {
        // Reallocate memory
        da->data = realloc(da->data, da->capacity * 2 * da->elem_size);
        if (da->data == NULL) {
            printf("Buy more ram!\n");
            exit(1);
        }
        da->capacity *= 2;
    }

    void *dst = da->data + (da->count * da->elem_size);
    memcpy(dst, item, da->elem_size);
    da->count += 1;
}

int runtime_compare_strings(RawString *str1, RawString *str2) {
    if (str1->len == str2->len) {
        for (int i = 0; i < str1->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return 0;
    }

    if (str1->len < str2->len) {
        for (int i = 0; i < str1->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return -1;
    } else {
        for (int i = 0; i < str2->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return +1;

    }
}