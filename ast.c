#include "lexer.c"
#include "lib/dynamic_array.c"

#define MAX_STATEMENTS_WITHIN_BLOCK 64

typedef struct AstNode AstNode;
typedef struct AstExpr AstExpr;
typedef struct AstDeclaration AstDeclaration;
typedef struct AstBinary AstBinary;
typedef struct AstUnary AstUnary;
typedef struct AstLiteral AstLiteral;
typedef struct AstIdentifier AstIdentifier;

typedef enum AstType {
    AST_ERR,
    AST_BLOCK,
    AST_FUNCTION_DEFN,
    AST_FUNCTION_CALL,
    AST_DECLARATION,
    AST_PRINT,
    AST_RETURN,
    AST_IF,
    AST_FOR,
    AST_EXPR,
    AST_RANGE_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_IDENTIFIER,
    AST_LITERAL,
} AstType;

typedef struct AstNode {
    AstType type;

    Pos start;
    Pos end;
} AstNode;


typedef enum TypeKind {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_IDENTIFER,
} TypeKind;

typedef struct AstExpr {
    AstNode head;
    TypeKind evaluated_type;
} AstExpr;

typedef struct AstCode {
    AstNode head;

    DynamicArray statements;
} AstCode;

typedef enum DeclarationType {
    DECLARATION_TYPED,          // ex. a : int = b
    DECLARATION_TYPED_NO_EXPR,  // ex. a : int;
    DECLARATION_INFER,          // ex. a := b
    DECLARATION_CONSTANT        // ex. a :: 3 @ToDo - Not a thing yet
} DeclarationType;

typedef struct AstDeclaration {
    AstNode head;

    DeclarationType declaration_type;
    AstIdentifier  *identifier;
    TypeKind        declared_type;
    AstExpr        *expr;

    bool is_function_param;
} AstDeclaration;

typedef struct AstBlock {
    AstNode head;

    AstNode  *statements[MAX_STATEMENTS_WITHIN_BLOCK]; // @Cleanup - Make into a DynamicArray!
    int       num_of_statements;
} AstBlock;

typedef struct AstFunctionDefn {
    AstNode head;

    AstIdentifier *identifier;
    DynamicArray parameters; // of AstDeclaration
    TypeKind return_type;
    AstBlock *body;
    
    int return_label;

} AstFunctionDefn;

typedef struct AstFunctionCall {
    AstExpr head;

    AstIdentifier *identifer;
    DynamicArray arguments; // of AstExpr
} AstFunctionCall;

typedef struct AstPrint {
    AstNode head;

    AstExpr *expr;
} AstPrint;

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
    bool inclusive;
} AstRangeExpr;

typedef struct AstBinary {
    AstExpr head;

    TokenType operator;
    AstExpr  *left;
    AstExpr  *right;
} AstBinary;

typedef struct AstUnary {
    AstExpr head;

    TokenType operator;
    AstExpr  *expr;
} AstUnary;

typedef struct AstLiteral {
    AstExpr head;

    TokenType type;
    As_value as_value;
} AstLiteral;

typedef struct AstIdentifier {
    AstNode head;

    TypeKind type;
    char *name;
    int   length;

    int stack_offset;
} AstIdentifier;

const char *type_kind_to_str(TypeKind kind) {
    switch (kind) {
        case TYPE_VOID:      return "void";
        case TYPE_BOOL:      return "bool";
        case TYPE_INTEGER:   return "int";
        case TYPE_FLOAT:     return "float";
        case TYPE_STRING:    return "string";
        case TYPE_IDENTIFER: return "type_ident";
    }

    printf("%s:%d: compiler-error: TypeKind with enum number '%d' could not be turned into a string", __FILE__, __LINE__, kind);
    exit(1);
}