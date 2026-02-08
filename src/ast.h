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
typedef struct AstFile AstFile;
typedef struct Type Type;
typedef struct TypeTable TypeTable;

typedef enum AstKind {
    AST_ERR,
    AST_FILE,
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
    AST_IMPORT,
    AST_EXTERN,
    AST_PRINT,
    AST_ASSERT,
    AST_RETURN,
    AST_BREAK_OR_CONTINUE,
    AST_IF,
    AST_FOR,
    AST_WHILE,
    AST_EXPR,
    AST_EXPR_STMT,
    AST_TYPE,
    AST_RANGE_EXPR,
    AST_SEMICOLON_EXPR,
    AST_BINARY,
    AST_UNARY,
    AST_CAST,
    AST_TYPEOF,
    AST_SIZEOF,
    AST_NEW,
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
    OP_POWER        = 1,
    OP_UNARY_MINUS  = 2, // Picked arbitrarily. Should just not conflict with the others

    OP_BITWISE_AND = '&',
    OP_BITWISE_OR  = '|',
    OP_BITWISE_XOR = '^',
    OP_BITWISE_NOT = '~',
    OP_BITWISE_SHIFT_LEFT  = TOKEN_BITWISE_SHIFT_LEFT,
    OP_BITWISE_SHIFT_RIGHT = TOKEN_BITWISE_SHIFT_RIGHT,
    
    OP_DOT          = '.',
    OP_SUBSCRIPT    = '[',
    OP_ADDRESS_OF   = 3,
    OP_POINTER_DEREFERENCE = 4, // Picked arbitrarily. Should just not conflict with the others
    OP_SPREAD       = TOKEN_TRIPLE_DOT, // Spread operator e.g `foo(...args)`
    OP_CAST         = TOKEN_CAST,
} OperatorType;

typedef enum AstFlags {
    AST_FLAG_COMPILER_GENERATED                    = 1 << 0,
    AST_FLAG_CG_EXPR_ASSIGNED_DIRECTLY_TO_VARIABLE = 1 << 1,
    AST_FLAG_IS_TYPE_CHECKED                       = 1 << 2, // Ast nodes that have this flag has gone through type checking
    AST_FLAG_IS_C_VARARG                           = 1 << 3, // Set if the argument is a variadic c argument
    AST_FLAG_EXPR_IS_SPREADED                      = 1 << 4, // Set if the expression has had the operator ... applied to it
} AstFlags;

typedef struct Ast {
    AstKind  kind;
    AstFlags flags;
    AstFile *file;
    Pos      start;
    Pos      end;
} Ast;

typedef struct AstExpr {
    Ast   head;
    Type *type;
} AstExpr;

typedef struct AstFile {
    Ast          head;
    char        *text;
    char        *absolute_path;
    DynamicArray statements; // of Ast*
    DynamicArray imports;    // of AstImport*
    AstBlock    *scope;      // All exported declarations in this file e.g function definitions, struct definitions etc is contained here
    bool         is_parsed;
} AstFile;

typedef struct AstImport {
    Ast      head;
    Token    path_token;
    char    *string;
    char    *resolved_path;
    AstFile *imported_file;
} AstImport;

typedef struct AstExtern {
    Ast       head;
    Token     extern_token;
    char     *extern_string;
    AstBlock *block;
} AstExtern;

typedef enum IdentifierFlags {
    IDENTIFIER_IS_NAME_OF_ENUM     = 1 << 0,
    IDENTIFIER_IS_NAME_OF_STRUCT   = 1 << 1,
    IDENTIFIER_IS_NAME_OF_FUNCTION = 1 << 2,
    IDENTIFIER_IS_STRUCT_MEMBER    = 1 << 3,
    IDENTIFIER_IS_STRUCT_METHOD    = 1 << 4,
    IDENTIFIER_IS_CONSTANT         = 1 << 5,
    IDENTIFIER_IS_RESOLVED         = 1 << 6,
} IdentifierFlags;

typedef struct AstIdentifier {
    Ast      head;
    char    *name;
    Type    *type;
    AstExpr *value;     // The value the identifier is set to in a declaration. @Note: Only available after typechecking
    IdentifierFlags flags;

    AstDeclaration *decl; // The declaration it belongs to

    int member_index;   // Used to know the insertion order of a struct member
    int member_offset;  // Relative positive offset of the member within the struct
    int stack_offset;
} AstIdentifier;

typedef enum DeclarationFlags {
    DECLARATION_TYPED                 = 1 << 0,
    DECLARATION_TYPED_NO_EXPR         = 1 << 1,
    DECLARATION_INFER                 = 1 << 2,
    DECLARATION_CONSTANT              = 1 << 3,
    DECLARATION_IS_STRUCT_MEMBER      = 1 << 4,
    DECLARATION_IS_FUNCTION_PARAMETER = 1 << 6,
    DECLARATION_IS_COMPILER_GENERATED = 1 << 7,
} DeclarationFlags;

typedef struct AstDeclaration {
    Ast              head;
    DynamicArray     idents; // of *AstIdentifier
    Type            *type;   // type is *TypeTuple if doing a multi declaration
    DynamicArray     values; // of *AstExpr
    DeclarationFlags flags;
} AstDeclaration;

typedef enum AssignOp {
    ASSIGN_EQUAL        = '=',
    ASSIGN_PLUS_EQUAL   = TOKEN_PLUS_EQUAL,
    ASSIGN_MINUS_EQUAL  = TOKEN_MINUS_EQUAL,
    ASSIGN_TIMES_EQUAL  = TOKEN_TIMES_EQUAL,
    ASSIGN_DIVIDE_EQUAL = TOKEN_DIVIDE_EQUAL,
} AssignOp;

typedef struct AstAssignment {
    Ast       head;
    AstExpr  *lhs;
    AssignOp  op;
    AstExpr  *expr;
} AstAssignment;

typedef enum DirectiveKind {
    DIRECTIVE_INVALID,
    DIRECTIVE_IMPORT,
    DIRECTIVE_EXTERN,
} DirectiveKind;

extern const char *directive_names[]; // Defined in ast.c

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
    Ast           head;
    BlockKind     kind;
    AstBlock     *parent;
    DynamicArray  statements;
    DynamicArray  identifiers; // of *AstIdentifier
    AstStruct    *belongs_to_struct;
    int           scope_number; // Only for debug purposes
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
    Ast      head;
    AstEnum *parent;
    char    *name;
    AstExpr *expr;  // null if auto-incremented enum
    int      value; // Evaluated value set by the typer
    int      index;
    int      label; // Used by print to determine how to branch to this enum value
    bool     is_typechecked; // @Deprectate!! We have the AST_FLAG_IS_TYPE_CHECKED to tell us this
} AstEnumerator;

typedef struct AstEnumLiteral {
    AstExpr head;

    AstIdentifier *identifier;
    AstEnumerator *enum_member;
} AstEnumLiteral;

typedef enum CallingConv {
    CALLING_CONV_SAAD,  // See the ABI in emit_function_defn or emit_function_call for the "spec" on this
    CALLING_CONV_MSVC,
    // CALLING_CONV_SYSTEM_V,
    // CALLING_CONV_C_DECL,
    // CALLING_CONV_STD_CALL,
    // CALLING_CONV_FAST_CALL,
} CallingConv;

typedef struct AstFunctionDefn {
    Ast             head;
    AstIdentifier  *identifier;
    Token           method_token;  // set if its a method
    Token           paren_start_token;
    AstIdentifier  *receiver;   // set if its a method and is just a short-hand for the 0'th parameter
    DynamicArray    parameters; // of *AstIdentifier
    int             variadic_parameter_index; // set to the index of a variadic parameter in the parameter list
    AstBlock       *body;
    DynamicArray    return_types;  // of *Type
    CallingConv     calling_convention;
    bool            is_method;
    bool            is_lambda;
    bool            is_extern;
    bool            is_variadic;
    
    int   num_bytes_locals;      // Number of bytes allocated for variables in the function
    int   num_bytes_temporaries; // Number of bytes allocated for temporaries in the function
    int   base_ptr;              // Where rbp is currently at in codegen
    int   temp_ptr;              // Offset to allocate temporary stack locations (used for function calls and function arguments). The lifetime of the temporary storage is as long as a statement, to allow expressions to be fully evaluated
    int   return_label;
    char *symbol_name;           // The assembly symbol name for the function call. Methods gets a prefix symbolname in the outputted asm to be namespaced to their receiver. E.g `foo :: method (bar: Bar)` would get an asm function name: `Bar.foo:`
    
    DynamicArray    lowered_return_params;  // of *AstIdentifier
    DynamicArray    lowered_params;         // of *AstIdentifier. A lowered representation of the parameter list with hidden return parameters / methods and all
} AstFunctionDefn;

typedef struct AstFunctionCall {
    AstExpr          head;
    AstIdentifier   *identifer;
    Token            paren_start_token;
    AstFunctionDefn *func_defn;         // The called function
    DynamicArray     arguments;         // of *AstExpr
    AstStruct       *belongs_to_struct; // The struct the function definition is defined on
    bool             is_method_call;    // If the function is defined within the struct or is a method of the struct, this field is true

    DynamicArray     lowered_arguments;      // of *AstExpr. A lowered representation of the argument list including hidden return arguments
} AstFunctionCall;

typedef struct AstPrint {
    Ast           head;
    DynamicArray  arguments;    // of *AstExpr
    char         *c_string;     // Generated c string in typer
    int           c_args;       // Number of arguments that needs to be supplied to the c_string
} AstPrint;

typedef struct AstExprStmt {
    AstExpr  head;
    AstExpr *expr;
} AstExprStmt;

typedef struct AstNew {
    AstExpr  head;
    AstExpr *expr;
} AstNew;

typedef struct AstAssert {
    Ast      head;
    AstExpr *expr;
} AstAssert;

typedef struct AstTypeof {
    AstExpr  head;
    AstExpr *expr;
} AstTypeof;

typedef struct AstSizeof {
    AstExpr  head;
    Type    *type;
} AstSizeof;

typedef struct AstReturn {
    Ast              head;
    DynamicArray     values; // of *AstExpr
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
    Token          asterix;
    bool           is_by_pointer;

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

typedef struct AstSemicolonExpr {
    AstExpr  head;
} AstSemicolonExpr;

typedef struct AstBinary {
    AstExpr    head;
    TokenType  operator;
    AstExpr   *left;
    AstExpr   *right;
} AstBinary;

typedef struct AstArrayAccess {
    AstExpr  head;
    AstExpr *accessing;
    AstExpr *index_expr;
    Token    open_bracket;
    Token    close_bracket;
} AstArrayAccess;

typedef enum MemberAccessKind {
    MEMBER_ACCESS_STRUCT,
    MEMBER_ACCESS_ENUM,
    MEMBER_ACCESS_METHOD_CALL,
} AccessorKind;

typedef struct AstMemberAccess {
    AstExpr head;

    AstExpr *left;
    AstExpr *right;
    
    AccessorKind access_kind;
    union {
        AstIdentifier   *struct_member;
        AstEnumerator   *enum_member;
        AstFunctionCall *method_call;
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
    Ast            head;
    AstIdentifier *designator;  // null if no explicit member name is used
    AstExpr       *value;
    AstIdentifier *member;      // member that this initializer is initializing
} AstStructInitializer;

typedef struct AstArrayLiteral {
    AstExpr head;
    DynamicArray expressions; // of *AstExpression
    int base_offset;          // used in CodeGenerator
} AstArrayLiteral;

typedef enum LiteralKind {
    LITERAL_BOOLEAN     = TOKEN_LITERAL_BOOLEAN,
    LITERAL_INTEGER     = TOKEN_LITERAL_INTEGER,
    LITERAL_FLOAT       = TOKEN_LITERAL_FLOAT,
    LITERAL_STRING      = TOKEN_LITERAL_STRING,
    LITERAL_NULL        = TOKEN_LITERAL_NULL,
    LITERAL_IDENTIFIER  = TOKEN_LITERAL_IDENTIFIER,
} LiteralKind;

typedef struct AstLiteral {
    AstExpr head;

    LiteralKind kind;
    As_value    as;
} AstLiteral;

const char *ast_to_str(Ast *ast); // Defined in ast.c


////////////////////////////////////////////////////////////////////////////////////////////////////////
//   Types
////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct TypeStruct TypeStruct;

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
    TYPE_TUPLE,
    TYPE_VARIADIC,
    TYPE_ANY,
} TypeKind;

typedef enum TypeFlags {
    TYPE_IS_FULLY_SIZED = 1 << 0,
} TypeFlags;

typedef struct Type {
    Ast       head;
    TypeKind  kind;
    TypeFlags flags;
    int       size; // Size in bytes
    char     *name; // A name to a struct or enum
} Type;

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

typedef struct TypePrimitive {
    Type           head;
    PrimitiveKind  kind;
    char          *name;
} TypePrimitive;


extern TypePrimitive primitive_types[PRIMITIVE_COUNT]; // Defined in ast.c
Type *primitive_type(PrimitiveKind kind);              // Defined in ast.c

typedef struct TypePointer {
    Type head;
    Type *pointer_to;
    bool has_been_dereferenced;
} TypePointer;

extern TypePointer *type_null_ptr;

typedef enum ArrayKind {
    ARRAY_FIXED,
    ARRAY_SLICE,
    ARRAY_DYNAMIC,
} ArrayKind;

typedef struct TypeArray {
    Type             head;
    AstArrayLiteral *node;
    ArrayKind        array_kind;
    Type            *elem_type;
    TypeStruct      *struct_defn;      // A generated struct to hold the .data and .count members
    AstExpr         *capacity_expr;    // If null, then the size was infered from the array literal
    long long        capacity;         // Will be set in typer
    long long        count;            // Compile-time number of elements in the array. Used for compile-time bounds checking
} TypeArray;

typedef struct TypeEnum {
    Type           head;
    AstEnum       *node;
    AstIdentifier *identifier;
    Type          *backing_type;
    int            min_value;
    int            max_value;
} TypeEnum;

typedef struct TypeEnumValue {
    Type     head;
    AstEnum *node;
    char    *name;
    As_value value;
} TypeEnumValue;

typedef struct TypeStruct {
    Type           head;
    AstStruct     *node;
    AstIdentifier *identifier;
    unsigned int   alignment;
} TypeStruct;

typedef struct TypeFunction {
    Type head;
    AstFunctionDefn *node;
} TypeFunction;

typedef struct TypeTuple {
    Type         head;
    DynamicArray types; // of *Type
} TypeTuple;

typedef struct TypeVariadic {
    Type       head;
    Type      *type;  // of *Type
    TypeArray *slice;
} TypeVariadic;

typedef struct TypeAny {
    Type        head;
    TypeStruct *struct_defn;
} TypeAny;

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
bool is_unsigned_integer(Type *type);

#endif
