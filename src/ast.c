#include "lexer.c"
#include "symbol_table.h"

#define MAX_STATEMENTS_WITHIN_BLOCK 64

typedef enum AstType {
    AST_ERR,
    AST_BLOCK,
    AST_STRUCT,
    AST_STRUCT_LITERAL,
    AST_STRUCT_INITIALIZER,
    AST_FUNCTION_DEFN,
    AST_FUNCTION_CALL,
    AST_DECLARATION,
    AST_ASSIGNMENT,
    AST_PRINT,
    AST_ASSERT,
    AST_RETURN,
    AST_IF,
    AST_FOR,
    AST_EXPR,
    AST_TYPE_INFO,
    AST_RANGE_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_IDENTIFIER,
    AST_ACCESSOR,
    AST_MEMBER_ACCESS,
    AST_LITERAL,
} AstType;

typedef enum OperatorType {
    OP_LOGICAL_OR = TOKEN_LOGICAL_OR,
    OP_LOGICAL_AND = TOKEN_LOGICAL_AND,
    OP_DOUBLE_EQUAL = TOKEN_DOUBLE_EQUAL,
    OP_NOT_EQUAL = TOKEN_NOT_EQUAL,
    OP_GREATER_THAN = '>',
    OP_LESS_THAN = '<',
    OP_GREATER_THAN_EQUAL = TOKEN_GREATER_EQUAL,
    OP_LESS_THAN_EQUAL = TOKEN_LESS_EQUAL,
    OP_MINUS = '-',
    OP_PLUS = '+',
    OP_TIMES = '*',
    OP_DIVIDE = '/',
    OP_MODULO = '%',
    OP_POWER = '^',
    OP_NOT = '!',
    OP_UNARY_MINUS = 5
} OperatorType;

typedef struct AstNode {
    AstType type;

    Pos start;
    Pos end;
} AstNode;

typedef enum TypeKind {
    TYPE_UNTYPED,
    TYPE_VAR,   // This type is here, when we don't know the actual type yet. F.x 'a : Car', Car might refer to a struct or an enum
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_STRUCT, // @Cleanup - Is this needed?
} TypeKind;

typedef struct TypeInfo {
    AstNode head;

    TypeKind kind;
    union {
        char *identifier; // A name to a struct, enum or function @Note - enums and functions are not yet a thing
    } as;
} TypeInfo;

typedef struct AstExpr {
    AstNode head;
    TypeInfo evaluated_type;
} AstExpr;

typedef struct AstCode {
    AstNode head;

    DynamicArray statements;
} AstCode;

typedef struct AstIdentifier {
    AstNode head;

    TypeInfo type;
    char    *name;
    int      length;

    int stack_offset;
} AstIdentifier;

typedef enum DeclarationType {
    DECLARATION_TYPED,          // ex. a : int = b
    DECLARATION_TYPED_NO_EXPR,  // ex. a : int
    DECLARATION_INFER,          // ex. a := b
    DECLARATION_CONSTANT        // ex. a :: 3 @ToDo - Not a thing yet
} DeclarationType;

typedef enum DeclarationFlags {
    DECL_IS_STRUCT_MEMBER      = 1 << 1,
    DECL_IS_FUNCTION_PARAMETER = 1 << 2,
} DeclarationFlags;

typedef struct AstDeclaration {
    AstNode head;

    DeclarationType  declaration_type;
    AstIdentifier   *identifier;
    TypeInfo         declared_type;
    AstExpr         *expr;
    DeclarationFlags flags;

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
    AstNode head;

    AstExpr *lhs;
    AssignOp op;
    AstExpr *expr;
} AstAssignment;

typedef struct AstBlock {
    AstNode head;

    AstNode  *statements[MAX_STATEMENTS_WITHIN_BLOCK]; // @Cleanup - Make into a DynamicArray!
    int       num_of_statements;
} AstBlock;

typedef struct AstStruct {
    AstNode head;

    AstIdentifier *identifier;
    SymbolTable    member_table; // of AstDeclaration

    unsigned size_bytes;
    unsigned padding;
} AstStruct;

typedef struct AstFunctionDefn {
    AstNode head;

    AstIdentifier *identifier;
    DynamicArray parameters; // of AstDeclaration
    TypeInfo return_type;
    AstBlock *body;
    
    int return_label;

} AstFunctionDefn;

typedef struct AstFunctionCall {
    AstExpr head;

    AstIdentifier *identifer; // @Cleanup - Why doesn't this have a 'evaluated_type' field???
    DynamicArray arguments; // of AstExpr
} AstFunctionCall;

typedef struct AstPrint {
    AstNode head;

    AstExpr *expr;
} AstPrint;

typedef struct AstAssert {
    AstNode head;

    AstExpr *expr;
} AstAssert;

typedef struct AstReturn {
    AstNode head;

    AstExpr *expr;
    AstFunctionDefn *enclosing_function;
} AstReturn;

typedef struct AstIf {
    AstNode head;

    AstExpr *condition;
    AstBlock *block;
    AstBlock *else_block;
    DynamicArray else_ifs;  // of AstIf
    bool has_else_block;
} AstIf;

typedef struct AstFor {
    AstNode head;

    AstIdentifier *iterator;
    AstExpr *iterable;
    AstBlock *body;
} AstFor;

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


typedef struct AstAccessor {
    AstNode head;

    char *name;
    AstDeclaration *member; // Member that is being accessed. NULL in case of the first accessor
} AstAccessor;

typedef struct AstMemberAccess {
    AstExpr head;

    AstIdentifier *ident;
    DynamicArray chain; // Of *AstAccessor
} AstMemberAccess;

typedef struct AstUnary {
    AstExpr head;

    OperatorType operator;
    AstExpr     *expr;
} AstUnary;

typedef struct AstStructLiteral {
    AstExpr head;

    TypeInfo     explicit_type;
    DynamicArray initializers; // of AstStructInitializer
} AstStructLiteral;

typedef struct AstStructInitializer {
    AstNode head;

    AstIdentifier *designator; // *Optional
    AstExpr       *value;

    int member_index;
} AstStructInitializer;

typedef struct AstLiteral {
    AstExpr head;

    TokenType type;
    As_value  as;
} AstLiteral;

char *type_to_str(TypeInfo type) {
    switch (type.kind) {
        case TYPE_UNTYPED:   return "untyped";
        case TYPE_VOID:      return "void";
        case TYPE_BOOL:      return "bool";
        case TYPE_INTEGER:   return "int";
        case TYPE_FLOAT:     return "float";
        case TYPE_STRING:    return "string";
        case TYPE_VAR:       return type.as.identifier;
        case TYPE_STRUCT:    return type.as.identifier;
    }
    XXX;
}

bool is_primitive_type(TypeKind kind) {
    if (kind == TYPE_VOID)    return true;
    if (kind == TYPE_BOOL)    return true;
    if (kind == TYPE_INTEGER) return true;
    if (kind == TYPE_FLOAT)   return true;
    if (kind == TYPE_STRING)  return true;

    return false;
}

TypeInfo type(TypeKind kind) {
    TypeInfo result = {0};
    result.kind = kind;
    return result;
}