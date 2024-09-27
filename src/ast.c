#include "lexer.c"
#include "symbol_table.h"

#define MAX_STATEMENTS_WITHIN_BLOCK 64

typedef struct AstDeclaration AstDeclaration;

typedef enum AstType {
    AST_ERR,
    AST_BLOCK,
    AST_STRUCT,
    AST_STRUCT_LITERAL,
    AST_STRUCT_INITIALIZER,
    AST_ARRAY_LITERAL,
    AST_ENUM,
    AST_ENUMERATOR,
    AST_ENUM_LITERAL,
    AST_FUNCTION_DEFN,
    AST_FUNCTION_CALL,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_PRINT,
    AST_ASSERT,
    AST_TYPEOF,
    AST_RETURN,
    AST_BREAK_OR_CONTINUE,
    AST_IF,
    AST_FOR,
    AST_EXPR,
    AST_TYPE,
    AST_RANGE_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_IDENTIFIER,
    AST_SUBSCRIPT,
    AST_ACCESSOR,
    AST_MEMBER_ACCESS,
    AST_ARRAY_ACCESS,
    AST_LITERAL,
} AstType;

typedef enum OperatorType {     // Here so that operators with the same symbols still can have different enumerations f.x unary minus and binary minus
    OP_LOGICAL_OR         = TOKEN_LOGICAL_OR,
    OP_LOGICAL_AND        = TOKEN_LOGICAL_AND,
    OP_DOUBLE_EQUAL       = TOKEN_DOUBLE_EQUAL,
    OP_NOT_EQUAL          = TOKEN_NOT_EQUAL,
    OP_NOT                = '!',
    OP_GREATER_THAN       = '>',
    OP_LESS_THAN          = '<',
    OP_GREATER_THAN_EQUAL = TOKEN_GREATER_EQUAL,
    OP_LESS_THAN_EQUAL    = TOKEN_LESS_EQUAL,

    OP_MINUS        = '-',
    OP_PLUS         = '+',
    OP_TIMES        = '*',
    OP_DIVIDE       = '/',
    OP_MODULO       = '%',
    OP_POWER        = '^',
    OP_UNARY_MINUS  = 1, // Picked arbitrarily. Should just not conflict with the others
    
    OP_DOT          = '.',
    OP_SUBSCRIPT    = '[',
    OP_ADDRESS_OF   = '&',
    OP_POINTER_DEREFERENCE = 2, // Picked arbitrarily. Should just not conflict with the others
} OperatorType;

typedef struct Ast {
    AstType type;

    Pos start;
    Pos end;
} Ast;

typedef enum TypeKind {
    TYPE_INVALID,
    TYPE_NAME,   // This type is here, when we don't know the actual type yet. F.x 'a : Car', Car might refer to a struct or an enum that we only resolve in the typing phase
    TYPE_VOID,
    TYPE_BOOL, 
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_STRUCT,
    TYPE_ENUM,
} TypeKind;

typedef enum TypeFlags {
    TYPE_IS_DEFINITION = 1 << 0,
} TypeFlags;

typedef struct Type {
    Ast  head;
    TypeKind  kind;
    TypeFlags flags;

    int size;
    union {
        char *name; // A name to a struct or enum // @Cleanup - No need to be a union
    } as;
} Type;

typedef struct AstExpr {
    Ast  head;
    Type *evaluated_type;
} AstExpr;

typedef struct AstCode {
    Ast      head;
    DynamicArray statements;
} AstCode;

typedef enum IdentifierFlags {
    IDENTIFIER_IS_NAME_OF_ENUM   = 1 << 0,
    IDENTIFIER_IS_NAME_OF_STRUCT = 1 << 1,
    IDENTIFIER_IS_CONSTANT       = 1 << 2,
} IdentifierFlags;

typedef struct AstIdentifier {
    Ast head;

    Type  *type;
    IdentifierFlags flags;
    char      *name;
    int        length;

    AstDeclaration *belongs_to_decl;

    int stack_offset;
} AstIdentifier;

typedef enum DeclarationFlags {
    DECLARATION_TYPED                 = 1 << 0,
    DECLARATION_TYPED_NO_EXPR         = 1 << 1,
    DECLARATION_INFER                 = 1 << 2,
    DECLARATION_CONSTANT              = 1 << 3,
    DECLARATION_IS_STRUCT_MEMBER      = 1 << 4,
    DECLARATION_IS_FUNCTION_PARAMETER = 1 << 5,
} DeclarationFlags;

typedef struct AstDeclaration {
    Ast head;

    DeclarationFlags flags;
    AstIdentifier   *identifier;
    Type        *declared_type;
    AstExpr         *expr;

    int member_index;   // Used to know the insertion order of a struct member
    int member_offset;  // Relative offset within the struct
} AstDeclaration;

typedef enum AssignOp {
    ASSIGN_EQUAL        = '=',
    ASSIGN_PLUS_EQUAL   = TOKEN_PLUS_EQUAL,
    ASSIGN_MINUS_EQUAL  = TOKEN_MINUS_EQUAL,
    ASSIGN_TIMES_EQUAL  = TOKEN_TIMES_EQUAL,
    ASSIGN_DIVIDE_EQUAL = TOKEN_DIVIDE_EQUAL,
} AssignOp;

typedef struct AstAssignment {
    Ast head;

    AstExpr  *lhs;
    AssignOp  op;
    AstExpr  *expr;
} AstAssignment;

typedef struct AstBlock {
    Ast head;

    Ast  *statements[MAX_STATEMENTS_WITHIN_BLOCK]; // @Cleanup - Make into a DynamicArray!
    int   num_of_statements;
} AstBlock;

typedef struct AstStruct {
    Ast head;

    AstIdentifier *identifier;
    SymbolTable    member_table; // of AstDeclaration
} AstStruct;

typedef struct AstEnum {
    Ast head;

    AstIdentifier *identifier;
    DynamicArray   enumerators; // of *AstEnumerator
} AstEnum;

typedef struct AstEnumerator {
    Ast head;

    AstEnum *parent;

    char    *name;
    AstExpr *expr;  // null if auto-incremented enum

    int      value; // Evaluated value set by the typer
    int      index;
    
    int label; // Used by print to determine how to branch to this enum value

    bool     is_typechecked;
} AstEnumerator;

typedef struct AstEnumLiteral {
    AstExpr head;

    AstIdentifier *identifier;
    AstEnumerator *enum_member;
} AstEnumLiteral;

typedef struct AstFunctionDefn {
    Ast head;

    AstIdentifier *identifier;
    DynamicArray parameters; // of AstDeclaration
    Type *return_type;
    AstBlock *body;

    int bytes_allocated; // Number of bytes allocated in the function
    
    int return_label;
} AstFunctionDefn;

typedef struct AstFunctionCall {
    AstExpr head;

    AstIdentifier *identifer;
    DynamicArray arguments; // of AstExpr
} AstFunctionCall;

typedef struct AstPrint {
    Ast head;

    AstExpr *expr;
} AstPrint;

typedef struct AstAssert {
    Ast head;

    AstExpr *expr;
} AstAssert;

typedef struct AstTypeof {
    Ast head;

    AstExpr *expr;
} AstTypeof;

typedef struct AstReturn {
    Ast head;

    AstExpr *expr;
    AstFunctionDefn *enclosing_function;
} AstReturn;

typedef struct AstIf {
    Ast head;

    AstExpr *condition;
    AstBlock *block;
    AstBlock *else_block;
    DynamicArray else_ifs;  // of *AstIf
} AstIf;

typedef enum ForKind {
    FOR_WITH_NAMED_ITERATOR = 1,
    FOR_WITH_NAMED_ITERATOR_AND_INDEX = 2,
    FOR_WITH_EXPR = 3,
    FOR_INFINITY_AND_BEYOND = 4,
} ForKind;

typedef struct AstFor {
    Ast head;

    ForKind kind;
    AstIdentifier *iterator;
    AstIdentifier *index;
    AstExpr *iterable;
    AstBlock *body;

    int post_expression_label; // Set in CodeGenerator so break and continue know where to branch to
    int done_label;
} AstFor;

typedef struct AstBreakOrContinue {
    Ast head;
    
    Token token; // Either has type TOKEN_BREAK or TOKEN_CONTINUE
    AstFor *enclosing_for;
} AstBreakOrContinue;

typedef struct AstRangeExpr {
    AstExpr head;

    AstExpr* start;
    AstExpr* end;
    bool     inclusive;
} AstRangeExpr;

typedef struct AstBinary {
    AstExpr head;

    TokenType operator;
    AstExpr  *left;
    AstExpr  *right;
} AstBinary;

typedef struct AstArrayAccess {
    AstExpr head;

    AstExpr *accessing;
    AstExpr *subscript;
    
    Token open_bracket;
    Token close_bracket;
} AstArrayAccess;

typedef enum MemberAccessKind {
    MEMBER_ACCESS_STRUCT,
    MEMBER_ACCESS_ENUM,
} AccessorKind;

typedef struct AstMemberAccess {
    AstExpr head;

    AstExpr *left;
    AstExpr *right;
    
    AccessorKind access_kind;
    union {
        AstDeclaration *struct_member;
        AstEnumerator  *enum_member;
    };
} AstMemberAccess;

typedef struct AstUnary {
    AstExpr head;

    OperatorType operator;
    AstExpr     *expr;
} AstUnary;

typedef struct AstStructLiteral {
    AstExpr head;

    Type     *explicit_type;
    DynamicArray initializers; // of AstStructInitializer
} AstStructLiteral;

typedef struct AstStructInitializer {
    Ast head;

    AstIdentifier *designator;  // null if no explicit member name is used
    AstExpr       *value;

    AstDeclaration *member;     // member that this initializer is initializing
} AstStructInitializer;

typedef struct AstArrayLiteral {
    AstExpr head;

    DynamicArray expressions; // of *AstExpression

    int base_offset; // used in CodeGenerator
} AstArrayLiteral;

typedef enum LiteralKind {
    LITERAL_BOOLEAN     = TOKEN_BOOLEAN,
    LITERAL_INTEGER     = TOKEN_INTEGER,
    LITERAL_FLOAT       = TOKEN_FLOAT,
    LITERAL_STRING      = TOKEN_STRING,
    LITERAL_NIL         = TOKEN_NIL,
    LITERAL_IDENTIFIER  = TOKEN_IDENTIFIER,
} LiteralKind;

typedef struct AstLiteral {
    AstExpr head;

    LiteralKind kind;
    As_value    as;
} AstLiteral;


typedef enum PrimitiveKind {
    PRIMITIVE_INVALID,

    PRIMITIVE_INT,
    PRIMITIVE_U8,
    PRIMITIVE_U16,
    PRIMITIVE_U32,
    PRIMITIVE_U64,
    PRIMITIVE_S8,
    PRIMITIVE_S16,
    PRIMITIVE_S32,
    PRIMITIVE_S64,

    PRIMITIVE_FLOAT,
    PRIMITIVE_F32,
    PRIMITIVE_F64,

    PRIMITIVE_STRING,
    PRIMITIVE_BOOL,
    PRIMITIVE_VOID,

    PRIMITIVE_COUNT,
} PrimitiveKind;

char *primitive_type_names[PRIMITIVE_COUNT] = {
    "invalid",
    "int",  // alias for s32
    "u8",
    "u16",
    "u32",
    "u64",
    "s8",
    "s16",
    "s32",
    "s64",
    "float",  // alias for f32
    "f32",
    "f64",
    "string",
    "bool",
    "void",
};

typedef struct TypePrimitive {
    Type      head;
    PrimitiveKind kind;
} TypePrimitive;

// @Note - maybe make each of these into a global variable instead with prefix t_<TYPE>
TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
    {.kind = PRIMITIVE_INVALID, .head = {.kind = TYPE_INVALID, .size = 0}},
    {.kind = PRIMITIVE_INT,     .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U8,      .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_U16,     .head = {.kind = TYPE_INTEGER, .size = 2}},
    {.kind = PRIMITIVE_U32,     .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U64,     .head = {.kind = TYPE_INTEGER, .size = 8}},
    {.kind = PRIMITIVE_S8,      .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_S16,     .head = {.kind = TYPE_INTEGER, .size = 2}},
    {.kind = PRIMITIVE_S32,     .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_S64,     .head = {.kind = TYPE_INTEGER, .size = 8}},
    {.kind = PRIMITIVE_FLOAT,   .head = {.kind = TYPE_FLOAT,   .size = 4}},
    {.kind = PRIMITIVE_F32,     .head = {.kind = TYPE_FLOAT,   .size = 4}},
    {.kind = PRIMITIVE_F64,     .head = {.kind = TYPE_FLOAT,   .size = 8}},
    {.kind = PRIMITIVE_STRING,  .head = {.kind = TYPE_STRING,  .size = 8}},
    {.kind = PRIMITIVE_BOOL,    .head = {.kind = TYPE_BOOL,    .size = 1}},
    {.kind = PRIMITIVE_VOID,    .head = {.kind = TYPE_VOID,    .size = 0}},
};


Type *primitive_type(PrimitiveKind kind) {
    return (Type *)(&primitive_types[kind]);
}

typedef struct TypePointer {
    Type head;
    Type *pointer_to;
} TypePointer;

TypePointer *t_nil_ptr = &(TypePointer){
    .head = {.kind = TYPE_POINTER, .size = 8},
    .pointer_to = (Type *)&(TypePrimitive){.kind = PRIMITIVE_VOID, .head = {.kind = TYPE_VOID, .size = 0}}
};

typedef enum ArrayFlags {
    ARRAY_IS_STATIC                         = 1 << 0,
    ARRAY_IS_STATIC_WITH_INFERRED_CAPACITY  = 1 << 1,
    ARRAY_IS_DYNAMIC                        = 1 << 2,
} ArrayFlags;

typedef struct TypeArray {
    Type head;
    AstArrayLiteral *node;
    
    ArrayFlags flags;
    Type *elem_type;

    AstExpr  *capacity_expr;
    long long capicity; // Will be set in in typer
} TypeArray;

typedef struct TypeEnum {
    Type head;
    AstEnum *node;
    
    AstIdentifier *identifier;
    Type *backing_type;
} TypeEnum;

typedef struct TypeEnumValue {
    Type head;
    AstEnum *node;
    
    char    *name;
    As_value value;
} TypeEnumValue;

typedef struct TypeStruct {
    Type head;
    AstStruct *node;

    AstIdentifier *identifier;

    unsigned int alignment;
} TypeStruct;

typedef struct TypeTable {
    HashTable user_types;
    Arena     types; // @Speed - Allocating types seperately from the ast nodes might be bad for cache locality as we are usually looking at the ast node and the type together. 
                     // Maybe it would be better to just allocate the Type right next to the Ast.
} TypeTable;

bool compare_user_types(const void *key, const void *item) {
    Type *ti = (*(Type **)(item));
    return strcmp((const char *)(key), ti->as.name) == 0;
}

TypeTable type_table_init() {
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

Type *type_var(TypeTable *tt, TypeKind kind, char *name) {
    Type *result = type_alloc(tt, sizeof(Type));
    result->kind = kind;
    result->as.name = name;
    return result;
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
        return primitive_type_names[prim->kind];
    }
    case TYPE_POINTER: {
        TypePointer *ptr = (TypePointer *)(type);
        StringBuilder builder = string_builder_init(32);
        sb_append(&builder, "*%s", type_to_str(ptr->pointer_to));

        return sb_to_string(&builder); // @Leak
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *)(type);
        StringBuilder builder = string_builder_init(32);
        sb_append(&builder, "[]%s", type_to_str(array->elem_type));

        return sb_to_string(&builder); // @Leak
    }
    case TYPE_NAME:      return type->as.name;
    case TYPE_STRUCT:    return type->as.name;
    case TYPE_ENUM:      return type->as.name;
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

bool is_a_before_b (Ast *a, Ast *b) {
    if ((b->start.line - a->start.line) > 0) return true;
    if ((b->start.line - a->start.line) < 0) return false;
    else {
        return (b->start.col - a->start.col) < 0;
    }
}