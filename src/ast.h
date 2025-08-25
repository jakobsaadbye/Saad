#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "lib/table.h"
#include "lib/dynamic_array.h"

#define MAX_STATEMENTS_WITHIN_BLOCK 64

typedef struct AstDeclaration AstDeclaration;
typedef struct AstBlock AstBlock;
typedef struct AstStruct AstStruct;
typedef struct AstEnum AstEnum;
typedef struct Type Type;
typedef struct TypeTable TypeTable;

typedef enum AstKind {
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
    AST_DIRECTIVE,
    AST_PRINT,
    AST_ASSERT,
    AST_TYPEOF,
    AST_RETURN,
    AST_BREAK_OR_CONTINUE,
    AST_IF,
    AST_FOR,
    AST_WHILE,
    AST_EXPR,
    AST_TYPE,
    AST_RANGE_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_CAST,
    AST_IDENTIFIER,
    AST_SUBSCRIPT,
    AST_ACCESSOR,
    AST_MEMBER_ACCESS,
    AST_ARRAY_ACCESS,
    AST_LITERAL,
} AstKind;

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
    OP_CAST         = TOKEN_CAST,
} OperatorType;

typedef struct Ast {
    AstKind kind;

    Pos start;
    Pos end;
} Ast;

typedef struct AstExpr {
    Ast   head;
    Type *type;
} AstExpr;

typedef struct AstCode {
    Ast      head;
    DynamicArray statements;
} AstCode;

typedef enum IdentifierFlags {
    IDENTIFIER_IS_NAME_OF_ENUM     = 1 << 0,
    IDENTIFIER_IS_NAME_OF_STRUCT   = 1 << 1,
    IDENTIFIER_IS_NAME_OF_FUNCTION = 1 << 2,
    IDENTIFIER_IS_CONSTANT         = 1 << 3,
} IdentifierFlags;

typedef struct AstIdentifier {
    Ast head;

    Type  *type;
    IdentifierFlags flags;
    char  *name;
    int    length;

    AstDeclaration *belongs_to_decl;

    int    stack_offset;
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

    AstIdentifier   *ident;
    Type            *type;
    AstExpr         *expr;
    DeclarationFlags flags;

    int member_index;   // Used to know the insertion order of a struct member
    int member_offset;  // Relative positive offset of the member within the struct
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


typedef enum DirectiveKind {
    DIRECTIVE_INVALID,
    DIRECTIVE_IMPORT,
    DIRECTIVE_EXTERN,
} DirectiveKind;

extern const char *directive_names[];

typedef enum Abi {
    ABI_INVALID,
    ABI_C,
} Abi;

typedef struct AstDirective {
    Ast           head;
    DirectiveKind kind;

    union {
        char      *import_name;
        Abi        extern_abi;
    };
} AstDirective;

typedef enum BlockKind {
    BLOCK_INVALID,
    BLOCK_IMPERATIVE,
    BLOCK_DECLARATIVE, // a struct or enum
} BlockKind;

typedef struct AstBlock {
    Ast head;

    BlockKind kind;
    AstBlock *parent;

    DynamicArray statements;

    DynamicArray identifiers; // of *AstIdentifier
    DynamicArray members;     // of *AstDeclaration. Used in structs

    AstStruct *belongs_to_struct;

    int scope_number; // Only for debug purposes
} AstBlock;

typedef struct AstStruct {
    Ast head;

    AstIdentifier *identifier;
    AstBlock      *scope;
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

typedef enum CallingConv {
    CALLING_CONV_SAAD,  // @Note - Currently this is just the same as MSVC but we might differ in the future?
    CALLING_CONV_MSVC,
    CALLING_CONV_SYSTEM_V,
    CALLING_CONV_C_DECL,
    CALLING_CONV_STD_CALL,
    CALLING_CONV_FAST_CALL,
} CallingConv;

typedef struct AstFunctionDefn {
    Ast             head;
    AstIdentifier  *identifier;
    DynamicArray    parameters; // of *AstDeclaration
    AstBlock       *body;
    Type           *return_type;
    CallingConv     call_conv;
    bool            is_extern;
 
    int num_bytes_locals;      // Number of bytes allocated for variables in the function
    int num_bytes_temporaries; // Number of bytes allocated for temporaries in the function
    int base_ptr;              // Where rbp is currently at in codegen
    int temp_ptr;              // Offset to allocate temporary stack locations (used for function calls and function arguments). The lifetime of the temporary storage is as long as a statement, to allow expressions to be fully evaluated

    int return_label;

} AstFunctionDefn;

typedef struct AstFunctionCall {
    AstExpr          head;
    AstIdentifier   *identifer;
    AstFunctionDefn *func_defn;    // The called function
    DynamicArray     arguments; // of *AstExpr
} AstFunctionCall;

typedef struct AstPrint {
    Ast           head;
    DynamicArray  arguments;    // of *AstExpr
    char         *c_string;     // Generated c string in typer
    int           c_args;       // Number of arguments that needs to be supplied to the c_string
} AstPrint;

typedef struct AstAssert {
    Ast      head;
    AstExpr *expr;
} AstAssert;

typedef struct AstTypeof {
    Ast      head;
    AstExpr *expr;
} AstTypeof;

typedef struct AstReturn {
    Ast              head;
    AstExpr         *expr;
    AstFunctionDefn *enclosing_function;
} AstReturn;

typedef struct AstIf {
    Ast           head;
    AstExpr      *condition;
    AstBlock     *then_block;
    AstBlock     *else_block;
    DynamicArray  else_ifs;  // of *AstIf
} AstIf;

typedef enum ForKind {
    FOR_WITH_NAMED_ITERATOR,
    FOR_WITH_NAMED_ITERATOR_AND_INDEX,
    FOR_WITH_EXPR,
    FOR_INFINITY_AND_BEYOND,
} ForKind;

typedef struct AstFor {
    Ast            head;
    ForKind        kind;
    AstIdentifier *iterator;
    AstIdentifier *index;
    AstExpr       *iterable;
    AstBlock      *body;

    int post_expression_label; // Set in CodeGenerator so break and continue know where to branch to
    int done_label;
} AstFor;

typedef struct AstWhile {
    Ast       head;
    AstExpr  *condition;
    AstBlock *body;

    int condition_label; // Set in CodeGenerator so break and continue know where to branch to
    int done_label;
} AstWhile;

typedef struct AstBreakOrContinue {
    Ast       head;
    Token     token;           // Either TOKEN_BREAK or TOKEN_CONTINUE
    TokenType enclosing_loop;  // Either TOKEN_FOR or TOKEN_WHILE
    union {
        AstFor   *for_loop;
        AstWhile *while_loop;
    } enclosing;

} AstBreakOrContinue;

typedef struct AstRangeExpr {
    AstExpr  head;
    AstExpr* start;
    AstExpr* end;
    bool     inclusive;
} AstRangeExpr;

typedef struct AstBinary {
    AstExpr    head;
    TokenType  operator;
    AstExpr   *left;
    AstExpr   *right;
} AstBinary;

typedef struct AstArrayAccess {
    AstExpr  head;
    AstExpr *accessing;
    AstExpr *subscript;
    Token    open_bracket;
    Token    close_bracket;
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

typedef struct AstCast {
    AstExpr  head;
    AstExpr *expr;
    Type    *cast_to;  // Explicit type cast e.g "cast(f64)x"
    bool     is_auto;  // Weather its an auto-cast "xx 2 / 4.0"
} AstCast;

typedef struct AstStructLiteral {
    AstExpr       head;
    Type         *explicit_type;
    DynamicArray  initializers; // of AstStructInitializer
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
    LITERAL_NULL        = TOKEN_NULL,
    LITERAL_IDENTIFIER  = TOKEN_IDENTIFIER,
} LiteralKind;

typedef struct AstLiteral {
    AstExpr head;

    LiteralKind kind;
    As_value    as;
} AstLiteral;

static const char *ast_to_str(Ast *ast) {
    switch (ast->kind) {
    case AST_ERR:                return "AST_ERR";
    case AST_BLOCK:              return "AST_BLOCK";
    case AST_DECLARATION:        return "AST_DECLARATION";
    case AST_ASSIGNMENT:         return "AST_ASSIGNMENT";
    case AST_DIRECTIVE:          return "AST_DIRECTIVE";
    case AST_PRINT:              return "AST_PRINT";
    case AST_ASSERT:             return "AST_ASSERT";
    case AST_TYPEOF:             return "AST_TYPEOF";
    case AST_STRUCT:             return "AST_STRUCT";
    case AST_STRUCT_LITERAL:     return "AST_STRUCT_LITERAL";
    case AST_STRUCT_INITIALIZER: return "AST_STRUCT_INITIALIZER";
    case AST_ARRAY_LITERAL:      return "AST_ARRAY_LITERAL";
    case AST_ENUM:               return "AST_ENUM";
    case AST_ENUMERATOR:         return "AST_ENUMERATOR";
    case AST_ENUM_LITERAL:       return "AST_ENUM_LITERAL";
    case AST_FUNCTION_DEFN:      return "AST_FUNCTION_DEFN";
    case AST_FUNCTION_CALL:      return "AST_FUNCTION_CALL";
    case AST_IF:                 return "AST_IF";
    case AST_RETURN:             return "AST_RETURN";
    case AST_FOR:                return "AST_FOR";
    case AST_WHILE:              return "AST_WHILE";
    case AST_BREAK_OR_CONTINUE:  return "AST_BREAK_OR_CONTINUE";
    case AST_EXPR:               return "AST_EXPR";
    case AST_RANGE_EXPR:         return "AST_RANGE_EXPR";
    case AST_BINARY:             return "AST_BINARY";
    case AST_UNARY:              return "AST_UNARY";
    case AST_CAST:               return "AST_CAST";
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
    TYPE_FUNCTION,
} TypeKind;

typedef enum TypeFlags {
    TYPE_IS_FULLY_SIZED      = 1 << 0,
} TypeFlags;

typedef struct Type {
    Ast       head;
    TypeKind  kind;
    TypeFlags flags;
    int       size;
    char     *name; // A name to a struct or enum
} Type;

typedef enum PrimitiveKind {
    PRIMITIVE_INVALID,

    PRIMITIVE_INT, // alias for s32
    PRIMITIVE_U8,
    PRIMITIVE_U16,
    PRIMITIVE_U32,
    PRIMITIVE_U64,
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

    PRIMITIVE_COUNT,
} PrimitiveKind;

typedef struct TypePrimitive {
    Type           head;
    PrimitiveKind  kind;
    char          *name;
} TypePrimitive;

// @Note - maybe make each of these into a global variable instead with prefix t_<TYPE>
static TypePrimitive primitive_types[PRIMITIVE_COUNT] = {
    {.kind = PRIMITIVE_INVALID, .name = "invalid",   .head = {.kind = TYPE_INVALID, .size = 0}},
    {.kind = PRIMITIVE_INT,     .name = "int",       .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U8,      .name = "u8",        .head = {.kind = TYPE_INTEGER, .size = 1}},
    {.kind = PRIMITIVE_U16,     .name = "u16",       .head = {.kind = TYPE_INTEGER, .size = 2}},
    {.kind = PRIMITIVE_U32,     .name = "u32",       .head = {.kind = TYPE_INTEGER, .size = 4}},
    {.kind = PRIMITIVE_U64,     .name = "u64",       .head = {.kind = TYPE_INTEGER, .size = 8}},
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
    {.kind = PRIMITIVE_UNTYPED_INT, .name = "untyped(int)", .head = {.kind = TYPE_INTEGER, .size = 4}},
};


static Type *primitive_type(PrimitiveKind kind) {
    return (Type *)(&primitive_types[kind]);
}

typedef struct TypePointer {
    Type head;
    Type *pointer_to;
} TypePointer;

extern TypePointer *t_null_ptr;

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
    Type           head;
    AstStruct     *node;
    AstIdentifier *identifier;
    DynamicArray   members; // of *AstDeclaration @Note - Copy into the node's the scope members
    unsigned int   alignment;
} TypeStruct;

typedef struct TypeFunction {
    Type head;
    AstFunctionDefn *node;
} TypeFunction;

typedef struct TypeTable {
    HashTable user_types;
    Arena     types; // @Speed - Allocating types seperately from the ast nodes might be bad for cache locality as we are usually looking at the ast node and the type together. 
                     // Maybe it would be better to just allocate the Type right next to the Ast.
} TypeTable;


bool compare_user_types(const void *key, const void *item);
TypeTable type_table_init(void);
Type     *type_lookup(TypeTable *tt, char *name);
Type     *type_add_user_defined(TypeTable *tt, Type *type);
void     *type_alloc(TypeTable *tt, size_t size);
char     *type_to_str(Type *type);
bool is_primitive_type(TypeKind kind);
bool is_signed_integer(Type *type);

#endif
