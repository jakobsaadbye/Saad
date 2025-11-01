#include "typer.c"

typedef enum CodeGenFlags {
    CODEGEN_TRY_ASSIGN_EXPRESSION_TO_VARIABLE = 1 << 0,
    CODEGEN_EMIT_EXPRESSION_AS_LVALUE         = 1 << 1,
} CodeGenFlags;

typedef struct CodeGenerator {
    StringBuilder head;     // Declaring bit target and other misc stuff
    StringBuilder data;     // Corresponding to section .data
    StringBuilder code;     // Corresponding to section .text
    StringBuilder code_header;  // Header of externs, globals etc within the .text section

    Parser *parser;

    AstBlock        *current_scope;
    AstFunctionDefn *enclosing_function;  // Current enclosing function
    TypeTable        type_table;

    size_t constants;   // For float and string literals
    size_t labels;      // For coditional jumps

    CodeGenFlags flags;

    int variable_address;    // Used in emit_expression() to assign directly to this address instead of a stack push & pop

    int enum_scratch_buffers; // Used for printing enum values where an int to string conversion needs to take place
    int num_pushed_arguments;
    
} CodeGenerator;

typedef struct MemberAccessResult {
    int base_offset;
    bool is_runtime_computed;
} MemberAccessResult;

void emit_code(CodeGenerator *cg, AstCode *code);
void emit_builtin_functions(CodeGenerator *cg);
void emit_header(CodeGenerator *cg);
void emit_footer(CodeGenerator *cg);

void emit_code(CodeGenerator *cg, AstCode *code);
void emit_statement(CodeGenerator *cg, Ast *node);
void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn);
void emit_function_call(CodeGenerator *cg, AstFunctionCall *call);
void emit_function_return_value(CodeGenerator *cg, AstFunctionDefn *func_defn);
void emit_return(CodeGenerator *cg, AstReturn *ast_return);
void emit_print(CodeGenerator *cg, AstPrint *print_stmt);
void emit_assert(CodeGenerator *cg, AstAssert *assertion);
void emit_typeof(CodeGenerator *cg, AstTypeof *ast_typeof);
void emit_if(CodeGenerator *cg, AstIf *ast_if);
void emit_for(CodeGenerator *cg, AstFor *ast_for);
void emit_while(CodeGenerator *cg, AstWhile *ast_while);
void emit_break_or_continue(CodeGenerator *cg, AstBreakOrContinue *boc);
void emit_enum(CodeGenerator *cg, AstEnum *ast_enum);
void emit_struct(CodeGenerator *cg, AstStruct *ast_struct);
void emit_array_literal(CodeGenerator *cg, AstArrayLiteral *array_lit, int base_offset);
void emit_declaration(CodeGenerator *cg, AstDeclaration *decl);
void emit_assignment(CodeGenerator *cg, AstAssignment *assign);
void emit_array_access(CodeGenerator *cg, AstArrayAccess *array_ac, bool lvalue);
void emit_expression(CodeGenerator *cg, AstExpr *expr);

void emit_integer_to_float_conversion(CodeGenerator *cg, Type *l_type, Type *r_type);
void emit_initialization(CodeGenerator *cg, int dest_offset, AstExpr *expr, Type *lhs_type, Type *rhs_type);
void emit_simple_initialization(CodeGenerator *cg, int dest_offset, bool dest_is_runtime_computed, bool dest_is_relative_to_rsp, Type *lhs_type, Type *rhs_type);
void emit_move_and_push(CodeGenerator *cg, int src_offset, bool src_is_runtime_computed, Type *src_type, bool lvalue);

MemberAccessResult emit_member_access(CodeGenerator *cg, AstMemberAccess *ma);

bool has_large_return_type(AstFunctionDefn *func_defn);
void check_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *get_comparison_set_instruction(CodeGenerator *cg, AstBinary *bin);
const char *boolean_operator_to_instruction(TokenType op);
char *WIDTH(Type *type);
bool is_flag_set(CodeGenerator *cg, CodeGenFlags flag);
void set_flag(CodeGenerator *cg, CodeGenFlags flag);
void reset_flag(CodeGenerator *cg, CodeGenFlags flag);

const char *REG_A(Type *type);
const char *REG_B(Type *type);
const char *REG_C(Type *type);
const char *REG_D(Type *type);

#define RAX "rax"
#define RBX "rbx"
#define RCX "rcx"
#define RDX "rdx"
#define R8  "r8"
#define R9  "r9"
#define RBP "rbp"
#define RSP "rsp"

#define PUSH(reg)                                \
    sb_append(&cg->code, "   push\t\t"reg"\n");  \
    cg->num_pushed_arguments += 1                \
    
#define POP(reg)                                 \
    sb_append(&cg->code, "   pop\t\t"reg"\n");   \
    cg->num_pushed_arguments -= 1                \
    
#define INCR_PUSH_COUNT()                        \
    cg->num_pushed_arguments += 1                \


CodeGenerator code_generator_init(Parser *parser) {
    CodeGenerator cg = {0};
    cg.head = sb_init(1024);
    cg.data = sb_init(1024);
    cg.code = sb_init(1024);
    cg.code_header = sb_init(1024);

    cg.parser         = parser;
    cg.type_table     = parser->type_table;
    cg.current_scope  = parser->current_scope;
    cg.enclosing_function           = NULL;
    cg.constants      = 0;
    cg.labels         = 0;
    cg.flags          = 0;
    cg.variable_address    = 0;
    cg.enum_scratch_buffers = 0;
    cg.num_pushed_arguments = 0;

    return cg;
}

void begin_emit_code(CodeGenerator *cg, AstCode *code) {
    check_main_exists(cg);
    emit_header(cg);
    emit_code(cg, code);
}

void check_main_exists(CodeGenerator *cg) {
    AstIdentifier *main = lookup_from_scope(cg->parser, cg->current_scope, "main");
    if (main == NULL) {
        printf("Error: Missing function 'main' as entry-point to the program\n");
        exit(1);
    }
}

void emit_header(CodeGenerator *cg) {
    sb_append(&cg->head, "bits 64\n");
    sb_append(&cg->head, "default rel\n");
    sb_append(&cg->head, "\n");

    sb_append(&cg->data, "segment .data\n");
    sb_append(&cg->data, "   fmt_int   DB \"%s\", 0\n", "%lld");
    sb_append(&cg->data, "   fmt_float DB \"%s\", 0\n", "%lf");
    sb_append(&cg->data, "   fmt_string DB \"%s\", 0\n", "%s");
    sb_append(&cg->data, "   fmt_address DB \"0x%s\", 0\n", "%lx");
    sb_append(&cg->data, "   string_false DB \"false\", 0\n");
    sb_append(&cg->data, "   string_true  DB \"true\", 0\n");
    sb_append(&cg->data, "   string_assert_fail  DB \"Assertion failed at line %s\", 10, 0\n", "%d");
    sb_append(&cg->code, "\n");

    sb_append(&cg->code_header, "segment .text\n");
    sb_append(&cg->code_header, "   global main\n");

    sb_append(&cg->code_header, "   extern ExitProcess\n");
    sb_append(&cg->code_header, "   extern printf\n");
    sb_append(&cg->code_header, "   extern sprintf\n");
    sb_append(&cg->code_header, "   extern malloc\n");
    sb_append(&cg->code_header, "   extern calloc\n");
    sb_append(&cg->code_header, "   extern free\n");
    sb_append(&cg->code_header, "   extern memset\n");
    sb_append(&cg->code_header, "   extern memcpy\n");

    emit_builtin_functions(cg);
}

void emit_builtin_functions(CodeGenerator *cg) {
    // Assert
    sb_append(&cg->code, "assert:\n");
    sb_append(&cg->code, "   cmp\t\tcl, 0\n");
    sb_append(&cg->code, "   jz \t\tassert_fail\n");
    sb_append(&cg->code, "   ret\n");

    sb_append(&cg->code, "assert_fail:\n");
    sb_append(&cg->code, "   mov\t\trcx, string_assert_fail\n");
    sb_append(&cg->code, "   call\t\tprintf\n");
    sb_append(&cg->code, "   mov\t\trcx, 1\n");
    sb_append(&cg->code, "   call\t\tExitProcess\n\n");
}

void emit_code(CodeGenerator *cg, AstCode *code) {
    for (int i = 0; i < code->statements.count; i++) {
        Ast *stmt = ((Ast **)(code->statements.items))[i];
        emit_statement(cg, stmt);
    }
}

void emit_block(CodeGenerator *cg, AstBlock *block) {
    cg->current_scope = block;
    for (int i = 0; i < block->statements.count; i++) {
        Ast *stmt = ((Ast **)block->statements.items)[i];
        emit_statement(cg, stmt);
    }
    cg->current_scope = block->parent;
}

void emit_statement(CodeGenerator *cg, Ast *node) {

    reset_temporary_storage(cg->enclosing_function);

    switch (node->kind) {
    case AST_DECLARATION:       emit_declaration(cg, (AstDeclaration *)(node)); break;
    case AST_ASSIGNMENT:        emit_assignment(cg, (AstAssignment *)(node)); break;
    case AST_FUNCTION_DEFN:     emit_function_defn(cg, (AstFunctionDefn *)(node)); break;
    case AST_FUNCTION_CALL:     emit_function_call(cg, (AstFunctionCall *)(node)); break;
    case AST_RETURN:            emit_return(cg, (AstReturn *)(node)); break;
    case AST_BREAK_OR_CONTINUE: emit_break_or_continue(cg, (AstBreakOrContinue *)(node)); break;
    case AST_BLOCK:             emit_block(cg, (AstBlock *)(node)); break;
    case AST_PRINT:             emit_print(cg, (AstPrint *)(node)); break;
    case AST_ASSERT:            emit_assert(cg, (AstAssert *)(node)); break;
    case AST_IF:                emit_if(cg, (AstIf *)(node)); break;
    case AST_FOR:               emit_for(cg, (AstFor *)(node)); break;
    case AST_WHILE:             emit_while(cg, (AstWhile *)(node)); break;
    case AST_ENUM:              emit_enum(cg, (AstEnum *)(node)); break;
    case AST_STRUCT: break;
    default:
        printf("compiler-error: emit_statement not implemented for %s\n", ast_to_str(node));
        XXX;
    }
}

void emit_break_or_continue(CodeGenerator *cg, AstBreakOrContinue *boc) {

    if (boc->token.type == TOKEN_BREAK) {
        if (boc->enclosing_loop == TOKEN_FOR) {
            sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing.for_loop->done_label);
        } else {
            sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing.while_loop->done_label);
        }
    } else {
        if (boc->enclosing_loop == TOKEN_FOR) {
            sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing.for_loop->post_expression_label);
        } else {
            sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing.while_loop->condition_label);
        }
    }
}

void emit_enum(CodeGenerator *cg, AstEnum *ast_enum) {
    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->data, "   __%s.%s DB \".%s\", 0\n", ast_enum->identifier->name, etor->name, etor->name);
    }


    // char *get_enum_string_<ENUM_NAME>(char *buf, int value)
    // - buf is in rcx
    // - value is in rdx
    // - returned string is in rax
    int case_label = make_label_number(cg);
    sb_append(&cg->code, "get_enum_string_%s:\n", ast_enum->identifier->name);
    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->code, "   mov\t\tr8, %d\n", etor->value);
        sb_append(&cg->code, "   cmp\t\trdx, r8\n");
        sb_append(&cg->code, "   jz\t\t\tenum_case_%d\n", case_label);

        etor->label = case_label;
        case_label  = make_label_number(cg);
    }

    // Case integer value
    PUSH(RCX);
    sb_append(&cg->code, "   mov\t\tr8, rdx\n");
    sb_append(&cg->code, "   mov\t\trdx, fmt_int\n");

    PUSH(R8);
    PUSH(RDX);
    PUSH(RCX);

    sb_append(&cg->code, "   call\t\tsprintf\n"); // @Note - For some reason sprintf reads its arguments from the stack instead of in rcx, rdx ... ??? Say what

    POP(RAX);

    POP(RBX);
    POP(RBX);
    POP(RBX);

    sb_append(&cg->code, "   ret\n");

    // Case name of enum member
    for (int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->code, "enum_case_%d:\n", etor->label);
        sb_append(&cg->code, "   mov\t\trax, __%s.%s\n", ast_enum->identifier->name, etor->name);
        sb_append(&cg->code, "   ret\n");
    }
}

char *cvtsi2ss_or_cvtsi2sd(Type *float_type) {
    assert(float_type->kind == TYPE_FLOAT);

    if (float_type->size == 4) return "cvtsi2ss";
    if (float_type->size == 8) return "cvtsi2sd";

    printf("Internal Compiler Error: Unhandled cases in cvtsi2ss_or_cvtsi2sd(). Got float type of size %d", float_type->size);
    exit(1);
}

char *movd_or_movq(Type *float_type) {
    assert(float_type->kind == TYPE_FLOAT);

    if (float_type->size == 4) return "movd";
    if (float_type->size == 8) return "movq";

    printf("Internal Compiler Error: Unhandled cases in movd_or_movq(). Got float type of size %d", float_type->size);
    exit(1);
}

void emit_assignment(CodeGenerator *cg, AstAssignment *assign) {

    sb_append(&cg->code, "   ; Ln %d: Assignment\n", assign->head.start.line);

    // @Speed: I think we should try to utilize direct assignment if we can. Currently
    //         the problem is in knowing weather or not the direct assignment took place or not in.
    //         f.x in struct reassignment: 'my_struct = Vector2{5, 6}'. We first emit the struct literal temporarily
    //         and then perform a memcopy.

    if (assign->op == ASSIGN_EQUAL) {
        emit_expression(cg, assign->expr);
    } else {

        // For all the compound assignments we do the binary operation between the expression on the left hand side as an r-value and the expression on the right
        AstBinary bin       = {0};
        bin.head.head.kind  = AST_BINARY;
        bin.head.head.start = assign->head.start;
        bin.head.head.end   = assign->head.end;
        bin.head.type       = assign->expr->type;
    
        bin.left  = assign->lhs;
        bin.right = assign->expr;
    
        switch (assign->op) {
            case ASSIGN_PLUS_EQUAL:   bin.operator = '+'; break;
            case ASSIGN_MINUS_EQUAL:  bin.operator = '-'; break;
            case ASSIGN_TIMES_EQUAL:  bin.operator = '*'; break;
            case ASSIGN_DIVIDE_EQUAL: bin.operator = '/'; break;
            case ASSIGN_EQUAL:
            default: XXX;
        }
    
        emit_expression(cg, (AstExpr *)&bin);
    }

    // Get the offset to assign to. The offset will be in rbx after emitting the lhs
    set_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);
    emit_expression(cg, assign->lhs);
    POP(RBX);

    // If we assign a pointer to an rvalue, then we derefence the pointer before the assignment. For example:
    //    a := 5;
    //    ptr_a := &a;
    //    ptr_a = 10;      // Derefernce 'ptr_a' and assign 'a' to 10 (like it does in c)
    if (assign->lhs->type->kind == TYPE_POINTER && assign->expr->type->kind != TYPE_POINTER) {
        sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
    }

    emit_simple_initialization(cg, 0, true, false, assign->lhs->type, assign->expr->type);
}

void emit_for(CodeGenerator *cg, AstFor *ast_for) {
    int cond_label            = make_label_number(cg);
    int post_expression_label = make_label_number(cg);
    int done_label            = make_label_number(cg);

    ast_for->post_expression_label = post_expression_label;
    ast_for->done_label            = done_label;
    
    if (!ast_for->iterable) {
        sb_append(&cg->code, "   ; Infinite for-loop\n");
        sb_append(&cg->code, "L%d:\n", cond_label);
        emit_block(cg, ast_for->body);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);
        sb_append(&cg->code, "L%d:\n", done_label);
    }
    else if (ast_for->iterable->head.kind == AST_RANGE_EXPR) {
        AstRangeExpr * range = (AstRangeExpr *)(ast_for->iterable);
        
        // allocate space for start and end range values 

        cg->enclosing_function->base_ptr = align_value(cg->enclosing_function->base_ptr, 8);

        int offset_iterator = cg->enclosing_function->base_ptr - 8;
        int offset_start    = offset_iterator - 8;
        int offset_end      = offset_iterator - 16;
        int offset_index    = offset_iterator - 24; // Only used if an index is also specified

        cg->enclosing_function->base_ptr -= 24;
        if (ast_for->index) {
            cg->enclosing_function->base_ptr -= 8;
            ast_for->index->stack_offset = offset_index;
        }
        ast_for->iterator->stack_offset = offset_iterator;

        sb_append(&cg->code, "   ; Ln %d: Ranged for-loop\n", ast_for->head.start.line);
        emit_expression(cg, range->start);
        emit_expression(cg, range->end);

        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", offset_start);
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rbx\n", offset_end);

        // initialize iterator and optionally the index
        sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", offset_start);
        sb_append(&cg->code, "   mov\t\t%d[rbp], eax\n", offset_iterator);
        if (ast_for->index)  sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", offset_index);

        sb_append(&cg->code, "L%d:\n", cond_label);
        sb_append(&cg->code, "   mov\t\teax, %d[rbp]\n", offset_end);
        sb_append(&cg->code, "   cmp\t\t%d[rbp], eax\n", offset_iterator);
        sb_append(&cg->code, "   %s\t\tL%d\n", range->inclusive ? "jg" : "jge", done_label);

        emit_block(cg, ast_for->body);
        
        sb_append(&cg->code, "L%d:\n", post_expression_label);
        sb_append(&cg->code, "   inc\t\tDWORD %d[rbp]\n", offset_iterator);
        if (ast_for->index)  sb_append(&cg->code, "   inc\t\tDWORD %d[rbp]\n", offset_index);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    } 
    else {
        assert(ast_for->iterable);
        assert(ast_for->iterable->type->kind == TYPE_ARRAY);

        AstIdentifier *iterator = ast_for->iterator;
        AstIdentifier *index    = ast_for->index;
        AstExpr       *iterable = ast_for->iterable;
        TypeArray     *iterable_type = (TypeArray *)iterable->type;

        // Allocate space for iterator, pointer to start of array, stop condition (count) and index
        //
        // :WrongForLoopSizing @Investigate - Might have to do with the fact that we are not aligning the stack before allocating space for the iterator!
        int aligned_iterator_size = iterator->type->size;
        int offset_iterator       = cg->enclosing_function->base_ptr - aligned_iterator_size;
        int offset_data           = offset_iterator - 8;
        int offset_count          = offset_iterator - 16;
        int offset_index          = offset_iterator - 24;
        cg->enclosing_function->base_ptr -= (24 + aligned_iterator_size);
        
        iterator->stack_offset = offset_iterator;
        if (index) {
            index->stack_offset = offset_index;
        }

        sb_append(&cg->code, "   ; Ln %d: For-loop\n", ast_for->head.start.line);
        emit_expression(cg, iterable);

        POP(RAX);

        // Copy .data and .count from the iterable (array) into rbx and rcx respectively
        switch (iterable_type->array_kind) {
        case ARRAY_FIXED: {
            sb_append(&cg->code, "   mov\t\trbx, rax\n");
            sb_append(&cg->code, "   mov\t\trcx, %d\n", iterable_type->capacity);
            break;
        }
        case ARRAY_SLICE: {
            sb_append(&cg->code, "   mov\t\trbx, 0[rax]\n");
            sb_append(&cg->code, "   mov\t\trcx, 8[rax]\n");
            break;
        }
        case ARRAY_DYNAMIC: {
            sb_append(&cg->code, "   mov\t\trbx, 0[rax]\n");
            sb_append(&cg->code, "   mov\t\trcx, 8[rax]\n");
            break;
        }
        }

        sb_append(&cg->code, "   mov\t\t%d[rbp], rbx     ; data\n", offset_data);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rcx     ; count\n", offset_count);
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0 ; index\n", offset_index);
        
        sb_append(&cg->code, "L%d:\n", cond_label);
        sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", offset_count);
        sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", offset_index);
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   jge\t\tL%d\n", done_label);

        if (iterator->type->size <= 8) {
            // Copy the value into register
            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", offset_data);
            sb_append(&cg->code, "   imul\t\trax, %d\n", iterator->type->size);
            sb_append(&cg->code, "   add\t\trbx, rax\n");
            sb_append(&cg->code, "   mov\t\t%s, %s [rbx]\n", REG_A(iterator->type), WIDTH(iterator->type));
            sb_append(&cg->code, "   mov\t\t%d[rbp], %s \n", offset_iterator, REG_A(iterator->type));
        } else {
            // Copy the value in chunks of 8 bytes
            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", offset_data);
            sb_append(&cg->code, "   imul\t\trax, %d\n", iterator->type->size);
            sb_append(&cg->code, "   add\t\trbx, rax\n"); // rbx holds offset into beginning of struct
            int num_copies = aligned_iterator_size / 8;
            for (int i = 0; i < num_copies; i++) {
                sb_append(&cg->code, "   mov\t\trax, %d[rbx]\n", i * 8);
                sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", offset_iterator + (i * 8));
            }
        }

        emit_block(cg, ast_for->body);
        
        sb_append(&cg->code, "L%d:\n", post_expression_label);
        sb_append(&cg->code, "   inc\t\tQWORD %d[rbp]\n", offset_index);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    }
}

void emit_while(CodeGenerator *cg, AstWhile *ast_while) {
    int cond_label = make_label_number(cg);
    int done_label = make_label_number(cg);

    // Set the labels so that break and continue statements knows where to jump
    ast_while->condition_label = cond_label;
    ast_while->done_label = done_label;

    sb_append(&cg->code, "L%d:\n", cond_label);
    emit_expression(cg, ast_while->condition);
    POP(RAX);
    sb_append(&cg->code, "   cmp\t\tal, 0\n");
    sb_append(&cg->code, "   jz\t\t\tL%d\n", done_label);
    
    sb_append(&cg->code, "   ; While body\n");
    emit_block(cg, ast_while->body);
    sb_append(&cg->code, "   jmp\t\t\tL%d\n", cond_label);

    sb_append(&cg->code, "L%d:\n", done_label);

    return;
}

void emit_return(CodeGenerator *cg, AstReturn *ast_return) {

    emit_expression(cg, ast_return->expr);

    Type *expr_type = ast_return->expr->type;
    Type *return_type = ast_return->enclosing_function->return_type;

    if (is_untyped_type(expr_type) && return_type->kind == TYPE_FLOAT) {

        // Here we just convert the integer to a float
        assert(expr_type->kind == TYPE_INTEGER);
        POP(RAX);
        sb_append(&cg->code, "   %s\txmm0, %s\n", cvtsi2ss_or_cvtsi2sd(return_type), REG_A(expr_type));
        sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(return_type), REG_A(expr_type));
        PUSH(RAX);
    }

    sb_append(&cg->code, "   jmp\t\tL%d\n", ast_return->enclosing_function->return_label);
}

/* Source address is expected to be in rax. Destination address is expected to be in rbx. The rcx register is used as a temporary during the copy */ 
void emit_struct_copy(CodeGenerator *cg, TypeStruct *struct_defn, int dst_base_offset, int src_base_offset) {

    // Try and move the struct in registers
    if (struct_defn->head.size <= 8) {
        sb_append(&cg->code, "   mov\t\t%s, %d[rax]\n", REG_C((Type *) struct_defn), src_base_offset);
        sb_append(&cg->code, "   mov\t\t%d[rbx], %s\n", dst_base_offset, REG_C((Type *) struct_defn));
        return;
    }

    // Perform a memcpy of the struct
    sb_append(&cg->code, "   mov\t\tr8, %d\n", struct_defn->head.size);
    sb_append(&cg->code, "   lea\t\trdx, %d[rax]\n", src_base_offset);
    sb_append(&cg->code, "   lea\t\trcx, %d[rbx]\n", dst_base_offset);
    sb_append(&cg->code, "   call\t\tmemcpy\n");    // void* memcpy( void* dest, const void* src, std::size_t count );
}

// Moves the pushed value of the return expression into the 
// correct register or stack location expected by the caller
void emit_function_return_value(CodeGenerator *cg, AstFunctionDefn *func_defn) {

    Type *return_type = func_defn->return_type;

    switch (return_type->kind) {
    case TYPE_VOID: {
        sb_append(&cg->code, "   mov\t\trax, 0\n");
        return;
    }
    case TYPE_BOOL:
    case TYPE_INTEGER:
    case TYPE_ENUM:
    case TYPE_STRING:
    case TYPE_POINTER: {
        POP(RAX);
        return;
    }
    case TYPE_FLOAT: {
        POP(RAX);
        if (return_type->size == 8) {
            sb_append(&cg->code, "   movq\t\txmm0, rax\n");
        } else {
            sb_append(&cg->code, "   movd\t\txmm0, eax\n");
        }
        return;
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *) return_type;

        POP(RAX); // address of array

        // Copy .data, .count and possibly .capacity to the allocated return slot

        // Source is already in rax
        sb_append(&cg->code, "   mov\t\trbx, -8[rbp]\n");   // @Improvement -8[rbp] is only because we currently only can have a single return value
        emit_struct_copy(cg, array->struct_defn, 0, 0);
        sb_append(&cg->code, "   mov\t\trax, -8[rbp]\n");
        
        return;
    }
    case TYPE_STRUCT: {
        POP(RAX);
        
        if (return_type->size <= 8) {
            // Struct fits in a register, so the struct is just returned in rax
            return;
        }

        // Copy the struct to the allocated return value
        TypeStruct *struct_defn = (TypeStruct *)return_type;

        // Source is already in rax
        sb_append(&cg->code, "   mov\t\trbx, -8[rbp]\n");

        emit_struct_copy(cg, struct_defn, 0, 0);
        
        // Return a pointer to the returned struct
        sb_append(&cg->code, "   mov\t\trax, -8[rbp]\n");
        
        return;
    }
    case TYPE_FUNCTION:
    default: XXX;
    }
}

typedef enum Register {
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_RBP,
    REG_RSP,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,

    REG_XMM0,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7,
} Register;

char *gpr_register_names[16][4] = {
    { "rax", "eax",  "ax",   "al"   },
    { "rbx", "ebx",  "bx",   "bl"   },
    { "rcx", "ecx",  "cx",   "cl"   },
    { "rdx", "edx",  "dx",   "dl"   },
    { "rsi", "esi",  "si",   "sil"  },
    { "rdi", "edi",  "di",   "dil"  },
    { "rbp", "ebp",  "bp",   "bpl"  },
    { "rsp", "esp",  "sp",   "spl"  },
    { "r8 ", "r8d",  "r8w",  "r8b"  },
    { "r9 ", "r9d",  "r9w",  "r9b"  },
    { "r10", "r10d", "r10w", "r10b" },
    { "r11", "r11d", "r11w", "r11b" },
    { "r12", "r12d", "r12w", "r12b" },
    { "r13", "r13d", "r13w", "r13b" },
    { "r14", "r14d", "r14w", "r14b" },
    { "r15", "r15d", "r15w", "r15b" },
};

char *sse_register_names[8] = {
    "xmm0",
    "xmm1",
    "xmm2",
    "xmm3",
    "xmm4",
    "xmm5",
    "xmm6",
    "xmm7",
};

char *register_string(Register reg, int width) {
    if (width > 8) width = 8;

    if (reg <= REG_R15) {
        int i = -1;
        if (width == 8) i = 0;
        if (width == 4) i = 1;
        if (width == 2) i = 2;
        if (width == 1) i = 3;
        assert(i != -1);

        return gpr_register_names[reg][i];
    } 
    else if (reg <= REG_XMM7) {
        int i = reg - REG_XMM0;
        return sse_register_names[i];
    }
    else {
        XXX;
    }
}

// Moves the source register to the target offset relative to rbp or rsp
void MOV_ADDR_REG(CodeGenerator *cg, int dst_offset, char *relative_to, Register src_reg, Type *src_type) {
    char *src_reg_string = register_string(src_reg, src_type->size > 8 ? 8 : src_type->size);

    if (src_reg >= REG_XMM0) {
        assert(src_type->kind == TYPE_FLOAT);
        if (src_type->size == 8) {
            sb_append(&cg->code, "   movq\t\t%d[%s], %s\n", dst_offset, relative_to, src_reg_string);
        } else {
            sb_append(&cg->code, "   movd\t\t%d[%s], %s\n", dst_offset, relative_to, src_reg_string);
        }
    } else {
        sb_append(&cg->code, "   mov\t\t%d[%s], %s\n", dst_offset, relative_to, src_reg_string);
    }
}

// Moves the value at the source address into the destination address relative to rbp
void MOV_ADDR_ADDR(CodeGenerator *cg, int dst_addr, Type *dst_type, int src_addr, Type *src_type) {
    if (src_type->kind == TYPE_FLOAT) {
        sb_append(&cg->code, "   %s\t\t%s, %d[rbp]\n", movd_or_movq(src_type), REG_A(src_type), src_addr);
        sb_append(&cg->code, "   %s\t\t%d[rbp], %s\n", movd_or_movq(dst_type), dst_addr, REG_A(src_type));
    } else {
        sb_append(&cg->code, "   mov\t\t%s, %d[rbp]\n", REG_A(src_type), src_addr);
        sb_append(&cg->code, "   mov\t\t%d[rbp], %s\n", dst_addr, REG_A(src_type));
    }
}

char *get_argument_register_from_index(int index) {
    if (index == 0) return "rcx";
    if (index == 1) return "rdx";
    if (index == 2) return "r8";
    if (index == 3) return "r9";

    XXX;
}

void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn) {
    cg->enclosing_function = func_defn;

    if (func_defn->is_extern) {
        sb_append(&cg->code_header, "   extern %s\n", func_defn->identifier->name);
        return;
    }

    // Align local and temporary storage to 8 byte
    func_defn->num_bytes_locals      = align_value(func_defn->num_bytes_locals, 8);
    func_defn->num_bytes_temporaries = align_value(func_defn->num_bytes_temporaries, 8);

    int bytes_locals      = func_defn->num_bytes_locals;
    int bytes_temporaries = func_defn->num_bytes_temporaries; // We need twice the amount of space for arguments as we also need temporary space for them while popping arguments in a function call
    int bytes_total       = align_value(32 + bytes_locals + bytes_temporaries, 16);

    // Header
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "; bytes locals   : %d\n", bytes_locals);
    sb_append(&cg->code, "; bytes temp     : %d\n", bytes_temporaries);
    sb_append(&cg->code, "; bytes total    : %d\n", bytes_total);

    if (func_defn->is_method) {
        TypeStruct *receiver_struct = (TypeStruct *)func_defn->receiver_type;
        sb_append(&cg->code, "%s.%s:\n", receiver_struct->identifier->name, func_defn->identifier->name);
    } else {
        sb_append(&cg->code, "%s:\n", func_defn->identifier->name);
    }

    //
    // Prolog
    //

    PUSH(RBP);
    sb_append(&cg->code, "   mov\t\trbp, rsp\n");
    sb_append(&cg->code, "   sub\t\trsp, %d\n", bytes_total);

    // Move hidden struct return pointers (if any) to the first parts of the stack
    int num_large_return_values = func_defn->return_type->size > 8 ? 1 : 0;
    if (num_large_return_values > 0) {
        cg->enclosing_function->base_ptr -= 8;
        sb_append(&cg->code, "   mov\t\t-8[rbp], rcx\t; Return 0\n");
    }

    // Save big struct parameters into the shadow-space as we will be doing a memcpy that will override our input registers
    bool is_arguments_in_shadow_space = false;

    // Search for a big argument
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)func_defn->parameters.items)[i];
        
        if (i == 5) break; // None of the arguments were big

        if (param->type->size > 8) {
            is_arguments_in_shadow_space = true;
            break;
        }
    }

    if (is_arguments_in_shadow_space) {
        for (int i = 0; i < func_defn->parameters.count; i++) {
            int arg_index = i + num_large_return_values;

            if (arg_index > 3) break;

            int   save_address  = 16 + (arg_index * 8); // 16 bytes for push rbp and return address
            char *save_register = get_argument_register_from_index(arg_index);

            sb_append(&cg->code, "   mov\t\t%d[rbp], %s \t; Save arg %d \n", save_address, save_register, i);
        }
    }

    // Move arguments to this function into their home addresses
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        Type           *param_type = param->type;
        
        // Bump the base pointer down
        cg->enclosing_function->base_ptr -= param_type->size;

        // Align base pointer
        cg->enclosing_function->base_ptr = align_value(cg->enclosing_function->base_ptr, param_type->size > 8 ? 8 : param_type->size);
        
        // Assign the offset to the parameter
        param->ident->stack_offset = cg->enclosing_function->base_ptr;
        int param_offset = param->ident->stack_offset;
        
        int arg_index = i;

        // Shift the args by the number of hidden return parameters
        arg_index += num_large_return_values;

        sb_append(&cg->code, "   ; Copy %s -> %d\n", param->ident->name, param_offset);

        Register arg_reg = 0;
        if (arg_index < 4) {
            if (param_type->kind == TYPE_FLOAT) {
                if (arg_index == 0) arg_reg = REG_XMM0;
                if (arg_index == 1) arg_reg = REG_XMM1;
                if (arg_index == 2) arg_reg = REG_XMM2;
                if (arg_index == 3) arg_reg = REG_XMM3;
            } else {
                if (arg_index == 0) arg_reg = REG_RCX;
                if (arg_index == 1) arg_reg = REG_RDX;
                if (arg_index == 2) arg_reg = REG_R8;
                if (arg_index == 3) arg_reg = REG_R9;
            }
            assert(arg_reg != 0);

        } 

        int arg_address = 16 + (arg_index * 8);

        if (param->type->size <= 8) {
            if (is_arguments_in_shadow_space || arg_index >= 4) {
                sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", arg_address);
                MOV_ADDR_REG(cg, param_offset, RBP, REG_RAX, param_type);
            } else {
                MOV_ADDR_REG(cg, param_offset, RBP, arg_reg, param_type);
            }
        } else {
            // Argument was passed by reference and stored in the shadow space.
            if (is_arguments_in_shadow_space || arg_index >= 4) {
                sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", arg_address);
            } else {
                sb_append(&cg->code, "   mov\t\trax, %s\n", register_string(arg_reg, 8));
            }

            if (param->type->kind == TYPE_STRUCT) {
                sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", param_offset);
                emit_struct_copy(cg, (TypeStruct *)param->type, 0, 0);
            } else if (param->type->kind == TYPE_ARRAY) {
                TypeArray *array = (TypeArray *) param->type;
                sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", param_offset);
                emit_struct_copy(cg, array->struct_defn, 0, 0);
            } else {
                XXX;
            }
        }
    }

    // Make a label for the return statements to jump to
    int return_label = make_label_number(cg);
    func_defn->return_label = return_label;

    emit_block(cg, func_defn->body);

    //
    // Epilog
    //
    sb_append(&cg->code, "L%d:\n", return_label);

    emit_function_return_value(cg, func_defn);

    sb_append(&cg->code, "   add\t\trsp, %d\n", bytes_total);
    POP(RBP);
    sb_append(&cg->code, "   ret\n");
}

void emit_cast(CodeGenerator *cg, AstCast *cast) {
    emit_expression(cg, cast->expr);

    Type *from = cast->expr->type;
    Type *to   = cast->cast_to;


    //
    // @Note: This should follow the exact casting rules defined in typer.c "can_cast_explicitly()""
    //

    // Int -> X
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_INTEGER) return;    // already zero extended in the push
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_FLOAT) {
        POP(RAX);
        char *op = to->size == 4 ? "cvtsi2ss" : "cvtsi2sd";
        sb_append(&cg->code, "   %s\txmm0, rax\n", op);
        sb_append(&cg->code, "   movq\trax, xmm0\n");
        PUSH(RAX);
        return;
    }
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_ENUM) return;    // already handled when pushing enums
    if (from->kind == TYPE_INTEGER && to->kind == TYPE_BOOL) return;    // 0 = false, otherwise = true

    // Float -> X
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_FLOAT) {
        if (from->size == 8 && to->size == 4) {
            POP(RAX);
            sb_append(&cg->code, "   movq\t\txmm0, rax\n");
            sb_append(&cg->code, "   cvtsd2ss\txmm0, xmm0\n");
            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
            PUSH(RAX);
            return;
        }
        else if (from->size == 4 && to->size == 8) {
            POP(RAX);
            sb_append(&cg->code, "   movq\t\txmm0, rax\n");
            sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
            PUSH(RAX);
            return;
        } 
        else {
            return;
        }
    }
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_INTEGER) {
        POP(RAX);
        sb_append(&cg->code, "   movq\t\txmm0, rax\n");
        if (from->size == 8) {
            sb_append(&cg->code, "   cvttsd2si\t\trax, xmm0\n");
        } else if (from->size == 4) {
            sb_append(&cg->code, "   cvttss2si\t\teax, xmm0\n");
        } else {
            XXX; // unreachable!
        }
        PUSH(RAX);
        return;
    }
    if (from->kind == TYPE_FLOAT && to->kind == TYPE_BOOL) {
        POP(RAX);
        sb_append(&cg->code, "   movq\t\txmm0, rax\n");
        sb_append(&cg->code, "   xorps\t\txmm1, xmm1\n");   // clear to 0.0
        sb_append(&cg->code, "   ucomiss\txmm0, xmm1\n");   // unordered compare of float == 0.0
        sb_append(&cg->code, "   setne\t\tal\n");           // result is stored in zero-flag
        sb_append(&cg->code, "   movsx\t\trax, al\n");      // sign-extend result
        PUSH(RAX);
        return;
    }


    if (from->kind == TYPE_ENUM    && to->kind == TYPE_INTEGER) return; // handled
    if (from->kind == TYPE_ENUM    && to->kind == TYPE_ENUM)    return; // handled

    if (from->kind == TYPE_BOOL    && to->kind == TYPE_INTEGER) return; // handled
    if (from->kind == TYPE_BOOL    && to->kind == TYPE_FLOAT) {
        POP(RAX);
        char *cvt = to->size == 8 ? "cvtsi2sd" : "cvtsi2ss";
        sb_append(&cg->code, "   %s\txmm0, rax\n", cvt);
        sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(to), REG_A(to));
        PUSH(RAX);
        return;
    }

    if (from->kind == TYPE_POINTER && to->kind == TYPE_STRING) {
        TypePointer *from_ptr = (TypePointer *)from;
        if (from_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)from_ptr->pointer_to)->kind == PRIMITIVE_U8) {
            return; // handled
        }
    }

    if (from->kind == TYPE_STRING && to->kind == TYPE_POINTER) {
        TypePointer *to_ptr = (TypePointer *)to;
        if (to_ptr->pointer_to->kind == TYPE_INTEGER && ((TypePrimitive *)to_ptr->pointer_to)->kind == PRIMITIVE_U8) {
            return; // handled
        }
    }


    printf("%s:%d: Compiler Error: There were unhandled cases in 'emit_cast'. Casting from '%s' -> '%s' \n", __FILE__, __LINE__, type_to_str(from), type_to_str(to));
    return;
}

void move_func_argument_to_register_or_temp(CodeGenerator *cg, int arg_index, Type *arg_type, Type *param_type) {

    // Arguments gets put in either registers or known temporary stack locations.
    // We shift over the argument slot by the amount of large return types. So the layout is something like this:
    //
    // (Large_Return_0, Large_Return_1, Arg_0, Arg_1, ...)
    //      rcx              rdx          r8     r9
    //
    
    if (arg_index < 4) {
        // Pass the argument in a register
        Register reg = 0;

        if (arg_type->kind == TYPE_FLOAT || (param_type->kind == TYPE_FLOAT && is_untyped_type(arg_type) && ((TypePrimitive *)arg_type)->kind == PRIMITIVE_UNTYPED_INT) ) {
            if (arg_index == 0) reg = REG_XMM0;
            if (arg_index == 1) reg = REG_XMM1;
            if (arg_index == 2) reg = REG_XMM2;
            if (arg_index == 3) reg = REG_XMM3;
            assert(reg != 0);

            char *xmm_reg = register_string(reg, 0);

            sb_append(&cg->code, "   %s\t\t%s, %s\n", movd_or_movq(param_type), xmm_reg, REG_A(param_type));
        } 
        else {

            if (arg_index == 0) reg = REG_RCX;
            if (arg_index == 1) reg = REG_RDX;
            if (arg_index == 2) reg = REG_R8;
            if (arg_index == 3) reg = REG_R9;

            
            if (arg_type->size <= 8) {
                // <= 8 bytes: Pass the entire value in the register
                char *sized_reg = register_string(reg, arg_type->size);
                
                sb_append(&cg->code, "   mov\t\t%s, %s\n", sized_reg, REG_A(arg_type));
            } else {
                // > 8 bytes: Pass a pointer to the value
                char *reg8 = register_string(reg, 8);
                
                sb_append(&cg->code, "   mov\t\t%s, rax\n", reg8);
            }
        }
    } else {
        // Put the argument into temporary storage
        int temp_loc = push_temporary_value(cg->enclosing_function, 8);

        sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", temp_loc);
    }
}

inline bool has_large_return_type(AstFunctionDefn *func_defn) {
    return func_defn->return_type->size > 8;
}

void emit_function_call(CodeGenerator *cg, AstFunctionCall *call) {
    
    int arg_count = call->arguments.count;
    int num_large_return_values = 0;

    if (call->func_defn->return_type->size > 8) {
        num_large_return_values = 1;
    }

    // Push all the arguments
    for (int i = 0; i < arg_count; i++) {
        AstExpr *arg = ((AstExpr **)call->arguments.items)[i];
        emit_expression(cg, arg);
    }

    // Pop the arguments
    for (int i = arg_count - 1; i >= 0; i--) {
        AstExpr *arg      = ((AstExpr **)call->arguments.items)[i];
        Type    *arg_type = arg->type;

        AstDeclaration *param = ((AstDeclaration **)call->func_defn->parameters.items)[i];
        Type           *param_type = param->type;
        
        switch (arg_type->kind) {
            case TYPE_INTEGER: {
                POP(RAX);
                
                if (param_type->kind == TYPE_FLOAT && is_untyped_type(arg_type) && ((TypePrimitive *)arg_type)->kind == PRIMITIVE_UNTYPED_INT) {
                    // Convert the untyped int to be a float
                    sb_append(&cg->code, "   %s\txmm0, rax\n", cvtsi2ss_or_cvtsi2sd(param_type));
                    sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(param_type), REG_A(param_type));
                }

                break;
            }
            case TYPE_BOOL:
            case TYPE_STRING:
            case TYPE_ENUM:
            case TYPE_POINTER: {
                POP(RAX);
                break;
            }
            case TYPE_FLOAT: {
                POP(RAX);

                if (arg_type->size == 4 && param_type->size == 8) {
                    sb_append(&cg->code, "   movd\t\txmm0, eax\n");
                    sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
                    sb_append(&cg->code, "   movq\t\trax, xmm0\n");
                } else {
                    // We don't go from f64 -> f32 without it needing to have a cast, so we should not hit the following assert!
                    assert(arg_type->size == param_type->size);
                }

                break;
            }
            case TYPE_ARRAY: {
                POP(RAX); // Pop the pointer to the start of the array
                break;
            }
            case TYPE_STRUCT: {
                POP(RAX); // Here we just pop the pointer to the struct. We will copy the struct by value
                break;
            }
            default: XXX;
        }

        move_func_argument_to_register_or_temp(cg, i + num_large_return_values, arg_type, param_type);
    }

    // Put any arguments that needs to go on the stack, relative to the stack pointer
    int stack_arg_offset = 32;

    for (int i = 0; i < call->arguments.count; i++) {
        AstExpr *arg      = ((AstExpr **)call->arguments.items)[i];
        Type    *arg_type = arg->type;

        bool arg_is_in_register = i < 4 - num_large_return_values;
        if (arg_is_in_register) continue;
        

        int temp_loc = pop_temporary_value(cg->enclosing_function);

        sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", temp_loc);

        if (arg_type->kind == TYPE_STRUCT && arg_type->size <= 8) {
            // Struct needs to be passed by value. Copy the struct referenced in rax
            sb_append(&cg->code, "   mov\t\t%d[rsp], %s\n", stack_arg_offset, REG_A(arg_type));
        } else if (arg_type->kind == TYPE_ARRAY) {
            // @Incomplete
            XXX;
        } else {
            MOV_ADDR_REG(cg, stack_arg_offset, RSP, REG_RAX, arg_type);
        }

        stack_arg_offset += 8;

    }

    // Put large return values into the hidden slots allocated by the caller
    if (num_large_return_values > 0) {
        // @Hardcoded @Future - Right now we only deal with 1 large return value which we have reserved space for
        // in the function. Here we give the function a hidden pointer to that return value that the function knows it should write to
        int temp_loc = push_temporary_value(cg->enclosing_function, call->func_defn->return_type->size);

        sb_append(&cg->code, "   lea\t\trcx, %d[rbp]\t\t; Return value 0\n", temp_loc);
    }

    AstFunctionDefn *func_defn = call->func_defn;

    if (func_defn->call_conv == CALLING_CONV_MSVC) {
        // bool align_stack = cg->num_pushed_arguments % 2 == 1;

        // Boundary needs to be 16 byte aligned
        // if (align_stack) sb_append(&cg->code, "   sub\t\trsp, 8\n");
        sb_append(&cg->code, "   call\t\t%s\n", call->identifer->name);
        // if (align_stack) sb_append(&cg->code, "   add\t\trsp, 8\n");
    }
    else {
        // Assume standard calling convention (SAAD)
        sb_append(&cg->code, "   call\t\t%s\n", call->identifer->name);
    }
    

    Type *return_type = call->head.type;
    if (return_type->kind == TYPE_FLOAT) {
        if (return_type->size == 4) {
            sb_append(&cg->code, "   movd\t\teax, xmm0\n");
        } 
        else if (return_type->size == 8) {
            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
        }
    } else {
        // The result is already in rax
    }
}

void emit_if(CodeGenerator *cg, AstIf *ast_if) {
    emit_expression(cg, ast_if->condition);

    int done_label = make_label_number(cg);
    int first_else_if_label = -1;
    int else_label = -1;
    if (ast_if->else_ifs.count > 0)  first_else_if_label = make_label_number(cg);
    if (ast_if->else_block)          else_label          = make_label_number(cg);

    POP(RAX);
    sb_append(&cg->code, "   cmp\t\tal, 0\n");

    if (first_else_if_label != -1) {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", first_else_if_label);
    } else if (else_label != -1) {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", else_label);
    } else {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", done_label);
    }

    sb_append(&cg->code, "   ; block of if\n");
    emit_block(cg, ast_if->then_block);
    sb_append(&cg->code, "   jmp L%d\n", done_label);

    int next_if_else_label = first_else_if_label;
    for (int i = 0; i < ast_if->else_ifs.count; i++) {
        AstIf *else_if = &((AstIf *)(ast_if->else_ifs.items))[i];

        sb_append(&cg->code, ";#%zu else if\n", i + 1);
        sb_append(&cg->code, "L%d:\n", next_if_else_label);

        emit_expression(cg, else_if->condition);

        POP(RAX);
        sb_append(&cg->code, "   cmp\t\tal, 0\n");

        bool more_else_ifs_to_come = i + 1 < ast_if->else_ifs.count;
        bool last_else_if          = i + 1 == ast_if->else_ifs.count;
        if (more_else_ifs_to_come) {
            next_if_else_label = make_label_number(cg);
            sb_append(&cg->code, "   jz\t\t\tL%d\n", next_if_else_label);
        } else if (last_else_if) {
            if (ast_if->else_block) {
                sb_append(&cg->code, "   jz\t\t\tL%d\n", else_label);
            } else {
                sb_append(&cg->code, "   jz\t\t\tL%d\n", done_label);
            }
        }

        emit_block(cg, else_if->then_block);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    if (ast_if->else_block) {
        sb_append(&cg->code, "; else\n");
        sb_append(&cg->code, "L%d:\n", else_label);
        emit_block(cg, ast_if->else_block);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    sb_append(&cg->code, "; done\n");
    sb_append(&cg->code, "L%d:\n", done_label);
}

void emit_assert(CodeGenerator *cg, AstAssert *assertion) {
    emit_expression(cg, assertion->expr);

    POP(RCX);
    sb_append(&cg->code, "   mov\t\trdx, %d\n", assertion->head.start.line);
    sb_append(&cg->code, "   call\t\tassert\n");
}

void emit_typeof(CodeGenerator *cg, AstTypeof *ast_typeof) {
    char *type_name = type_to_str(ast_typeof->expr->type);

    int const_number = cg->constants++;

    sb_append(&cg->data, "   CS%d\tDB \"%s\", 10, 0\n", const_number, type_name);
    sb_append(&cg->code, "   mov\t\trax, CS%d\n", const_number);
}

void emit_new(CodeGenerator *cg, AstNew *ast_new) {
    AstExpr *expr = ast_new->expr;

    sb_append(&cg->code, "   mov\t\trcx, %d\n", expr->type->size);
    sb_append(&cg->code, "   call\t\tmalloc\n");
    PUSH(RAX);      // Save the pointer

    emit_expression(cg, expr);

    if (expr->type->kind == TYPE_STRUCT) {
        TypeStruct *struct_defn = (TypeStruct *) expr->type;
        POP(RAX);   // Pointer to temporary struct literal
        POP(RBX);   // Pointer to allocated data
        emit_struct_copy(cg, struct_defn, 0, 0);
    } else if (expr->type->kind == TYPE_ARRAY) {
        XXX;
    } else {
        XXX;
    }
}

const char *REG_A(Type *type) {
    if (type->size == 1) return "al";
    if (type->size == 2) return "ax";
    if (type->size <= 4) return "eax";
    if (type->size <= 8) return "rax";
    XXX;
}

const char *REG_B(Type *type) {
    if (type->size == 1) return "bl";
    if (type->size == 2) return "bx";
    if (type->size <= 4) return "ebx";
    if (type->size <= 8) return "rbx";
    XXX;
}

const char *REG_C(Type *type) {
    if (type->size == 1) return "cl";
    if (type->size == 2) return "cx";
    if (type->size <= 4) return "ecx";
    if (type->size <= 8) return "rcx";
    XXX;
}

const char *REG_D(Type *type) {
    if (type->size == 1) return "dl";
    if (type->size == 2) return "dx";
    if (type->size <= 4) return "edx";
    if (type->size <= 8) return "rdx";
    XXX;
}

// @Cleanup @Refactor: Begins to look like Copy pasta of move_func_argument_to_register_or_temp
void move_print_argument_to_register_or_temporary(CodeGenerator *cg, Type *arg_type, int arg_index) {
    if (arg_index < 4) {
        // Goes in register

        char *reg = NULL;
        if (arg_index == 0) reg = RCX;
        if (arg_index == 1) reg = RDX;
        if (arg_index == 2) reg = R8;
        if (arg_index == 3) reg = R9;

        if (arg_type->kind == TYPE_INTEGER && arg_type->size < 4) {
            sb_append(&cg->code, "   movzx\t\trax, %s\n", REG_A(arg_type));
        }

        sb_append(&cg->code, "   mov\t\t%s, rax\n", reg);
        
    } else {
        // Goes in a temporary

        int temp_loc = push_temporary_value(cg->enclosing_function, 8);

        if (arg_type->kind == TYPE_INTEGER && arg_type->size < 4) {
            sb_append(&cg->code, "   movzx\t\trax, %s\n", REG_A(arg_type));
        }

        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", temp_loc);        
    }
}

void emit_printable_value(CodeGenerator *cg, Type *arg_type, int *num_enum_arguments, int struct_depth, int struct_base_offset) {
    switch (arg_type->kind) {
    case TYPE_FLOAT: {
        if (arg_type->size == 4) {
            POP(RAX);
            sb_append(&cg->code, "   movd\t\txmm0, eax\n");
            sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
            PUSH(RAX);
        } else if (arg_type->size == 8) {
            // Already aligned
        }
        break;
    }
    case TYPE_BOOL: {
        int true_label        = make_label_number(cg);
        int fallthrough_label = make_label_number(cg);
        POP(RAX);
        sb_append(&cg->code, "   cmp\t\tal, 0\n");
        sb_append(&cg->code, "   jnz\t\tL%d\n", true_label);
        sb_append(&cg->code, "   mov\t\trax, string_false\n");
        sb_append(&cg->code, "   jmp\t\tL%d\n", fallthrough_label);
        sb_append(&cg->code, "L%d:\n", true_label);
        sb_append(&cg->code, "   mov\t\trax, string_true\n");
        sb_append(&cg->code, "L%d:\n", fallthrough_label);
        PUSH(RAX);
        break;
    }
    case TYPE_ENUM: {
        int buffer_index = *num_enum_arguments;
        POP(RDX); // enum integer value
        sb_append(&cg->code, "   mov\t\trcx, enum_buffer_%d\n", buffer_index);
        sb_append(&cg->code, "   call\t\tget_enum_string_%s\n", ((TypeEnum *)(arg_type))->identifier->name);
        PUSH(RAX);
        *num_enum_arguments += 1;
        break;
    }
    case TYPE_STRUCT: {
        TypeStruct *struct_defn = (TypeStruct *)arg_type;
        AstStruct  *struct_node = struct_defn->node;

        // @Note @Hack - register r9 needs to be kept safe during the printout of the structure
        if (struct_depth == 0) {
            if (struct_defn->head.size > 8) {
                POP(R9);
            } else {
                POP(RAX);
                // :PrintSmallStructHack @Cleanup @Hack: We shouldn't be doing this. Find the root cause instaed
                int temp_offset = push_temporary_value(cg->enclosing_function, struct_defn->head.size);
                sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", temp_offset);
                sb_append(&cg->code, "   lea\t\tr9, %d[rbp]\n", temp_offset);
            }
        }
        
        DynamicArray members = struct_node->scope->members;
        for (int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)members.items)[i];

            int offset = struct_base_offset + member->member_offset;

            if (member->type->kind == TYPE_STRUCT) {
                // Recurse on the sub struct
                emit_printable_value(cg, member->type, num_enum_arguments, struct_depth + 1, offset);
            } else {
                // Push the member value on the stack
                sb_append(&cg->code, "   lea\t\trbx, %d[r9]\n", offset);
                emit_move_and_push(cg, 0, true, member->type, false);
                emit_printable_value(cg, member->type, num_enum_arguments, struct_depth + 1, struct_base_offset);
            }
        }
        break;
    }
    case TYPE_ARRAY: {
        break;
    }
    default:
        break;
    }
}

void pop_struct_members_from_print(CodeGenerator *cg, AstStruct *struct_, int *c_arg_index) {
    DynamicArray members = struct_->scope->members;
    for (int i = struct_->scope->members.count - 1; i >= 0; i--) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];

        if (member->type->kind == TYPE_STRUCT) {
            AstStruct *nested_struct = ((TypeStruct *)member->type)->node;
            pop_struct_members_from_print(cg, nested_struct, c_arg_index);
        } else {
            POP(RAX);
            move_print_argument_to_register_or_temporary(cg, member->type, *c_arg_index);
            *c_arg_index -= 1;
        }

    }
}

void emit_print(CodeGenerator *cg, AstPrint *print) {
    sb_append(&cg->code, "   ; Ln %d Print\n", print->head.start.line);

    // Push all the arguments on the stack
    cg->num_pushed_arguments = 0;
    int num_enum_arguments = 0;
    for (int i = 1; i < print->arguments.count; i++) {
        AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
        Type    *arg_type = arg->type;

        // set_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);
        emit_expression(cg, arg);

        // Note: Here we transform some of the expressions to match a C printf call. E.g
        // we emit each struct member when printing structs. 
        // @Future: At some point we want to rely on our own printf so we can get rid of some the print
        // complexities.
        emit_printable_value(cg, arg_type, &num_enum_arguments, 0, 0);
    }

    // Potentially allocate more space for enum integer to string conversion
    if (num_enum_arguments > cg->enum_scratch_buffers) {
        for (int i = cg->enum_scratch_buffers; i < num_enum_arguments; i++) {
            sb_append(&cg->data, "   enum_buffer_%d times 20 DB 0\n", i); // 20 is the length of the largest integer number 2^64
        }
        cg->enum_scratch_buffers = num_enum_arguments;
    }

    // This tells where we are in the generated c argument list which might
    // be different from the argument index as struct arguments gets a c argument
    // per member
    int c_args      = print->c_args;
    int c_arg_index = c_args - 1;

    // Pop and assign registers
    sb_append(&cg->code, "   ; Pop print arguments\n");

    int arg_count = print->arguments.count;
    for (int i = arg_count - 1; i >= 1; i--) {
        AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
        Type    *arg_type = arg->type;

        // @Incomplete: We need to also count pushed struct members
        cg->num_pushed_arguments -= 1;

        switch (arg_type->kind) {
            case TYPE_INTEGER:
            case TYPE_FLOAT:
            case TYPE_BOOL:
            case TYPE_STRING:
            case TYPE_ENUM:
            case TYPE_POINTER: 
            case TYPE_ARRAY: {
                POP(RAX);
                move_print_argument_to_register_or_temporary(cg, arg_type, c_arg_index);
                c_arg_index -= 1;
                break;
            }
            case TYPE_STRUCT: {
                AstStruct *struct_node = ((TypeStruct *)arg_type)->node;
                pop_struct_members_from_print(cg, struct_node, &c_arg_index);
                break;
            }
            default: XXX;
        }
    }

    int const_number = cg->constants++;
    sb_append(&cg->data, "   CS%d DB `%s`, 10, 0 \n", const_number, print->c_string);
    sb_append(&cg->code, "   mov\t\trcx, CS%d\n", const_number);

    if (c_args > 4) {
        // We need to put all the fixed placed stack allocated arguments relative to the stack pointer
        int stack_offset = 32; // + (c_args - 5) * 8;
        for (int i = 0; i < c_args - 4; i++) {
            int temp_offset   = pop_temporary_value(cg->enclosing_function);
            
            sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", temp_offset);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rsp], rax\n", stack_offset);

            stack_offset  += 8;
        }
    }

    // No need to align the stack as all the arguments are in registers
    sb_append(&cg->code, "   call\t\tprintf\n");
        
}

void zero_initialize(CodeGenerator *cg, Type *type, int dst_offset) {
    switch (type->kind) {
    case TYPE_INTEGER:
    case TYPE_FLOAT:
    case TYPE_BOOL:
    case TYPE_STRING:
    case TYPE_ENUM:
    case TYPE_POINTER: {
        sb_append(&cg->code, "   mov\t\t%s %d[rbp], 0\n", WIDTH(type), dst_offset);
        return;
    } 

    case TYPE_STRUCT: {
        TypeStruct *struct_defn = (TypeStruct *) type;

        sb_append(&cg->code, "   lea\t\trcx, %d[rbp]\n", dst_offset);
        sb_append(&cg->code, "   mov\t\trdx, 0\n");
        sb_append(&cg->code, "   mov\t\tr8, %d\n", struct_defn->head.size);
        sb_append(&cg->code, "   call\t\tmemset\n");    // void *memset(void *ptr, int x, size_t n);

        return;
    }

    case TYPE_ARRAY : {
        TypeArray *array = (TypeArray *)(type);

        // Zero out the memory if the size of the array is specified. Otherwise
        // we can skip zero initializing as the size is infered from the array literal
        // which will populate all the slots anyway

        switch (array->array_kind) {
        case ARRAY_FIXED: {
            sb_append(&cg->code, "   lea\t\trcx, %d[rbp]\n", dst_offset);
            sb_append(&cg->code, "   mov\t\trdx, 0\n");
            sb_append(&cg->code, "   mov\t\tr8, %d\n", array->head.size);
            sb_append(&cg->code, "   call\t\tmemset\n");    // void *memset(void *ptr, int x, size_t n);
            break;
        }
        case ARRAY_SLICE: {
            // The array is fully initialized from the array literal
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dst_offset + 0);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dst_offset + 8);
            break;
        }
        case ARRAY_DYNAMIC: {
            sb_append(&cg->code, "   mov\t\trdx, %d\n", array->elem_type->size);
            sb_append(&cg->code, "   mov\t\trcx, %d\n", array->capacity);
            sb_append(&cg->code, "   call\t\tcalloc\n");        // void* calloc( size_t num, size_t size );

            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", dst_offset + 0);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], %d\n", dst_offset + 8, array->count);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], %d\n", dst_offset + 16, array->capacity);
            break;
        }}
            
        return;
    }
    default:
        printf("%s:%d: compiler-error: There were unhandled cases in 'zero_initialize'\n", __FILE__, __LINE__);
        exit(1);
    }
}

void emit_simple_initialization(CodeGenerator *cg, int dst_offset, bool dst_is_runtime_computed, bool dst_is_relative_to_rsp, Type *lhs_type, Type *rhs_type) {

    char dst[16];
    if (dst_is_runtime_computed) {
        sprintf(dst, "[rbx]");
    } else {
        if (dst_is_relative_to_rsp) {
            sprintf(dst, "%d[rsp]", dst_offset);
        } else {
            sprintf(dst, "%d[rbp]", dst_offset);
        }
    }

    switch (lhs_type->kind) {
    case TYPE_BOOL: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tBYTE %s, al\n", dst);
        return;
    }
    case TYPE_INTEGER: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\t%s %s, %s\n", WIDTH(lhs_type), dst, REG_A(lhs_type));
        return;
    }
    case TYPE_FLOAT: {
        POP(RAX);
        if (rhs_type) {
            if (rhs_type->kind == TYPE_INTEGER) {
                // @Note - I feel like this conversion from int to float should be dealt with at the typing level instead of here. 
                // One idea is to just change it at the check_declarartion level. If we have a float on the left and an integer on the right, just change the right hand side type to be integer
                sb_append(&cg->code, "   %s\txmm0, rax\n", cvtsi2ss_or_cvtsi2sd(lhs_type));
                sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(lhs_type), dst);
            }
            else if (rhs_type->kind == TYPE_FLOAT) {
                if (lhs_type->size == 8 && rhs_type->size == 4) {
                    sb_append(&cg->code, "   movd\t\txmm0, eax\n");
                    sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
                    sb_append(&cg->code, "   movsd\t\t%s, xmm0\n", dst);
                }
                else if (lhs_type->size == 4 && rhs_type->size == 8) {
                    sb_append(&cg->code, "   movq\t\txmm0, rax\n");
                    sb_append(&cg->code, "   cvtsd2ss\txmm0, xmm0\n");
                    sb_append(&cg->code, "   movss\t\t%s, xmm0\n", dst);
                }
                else {
                    sb_append(&cg->code, "   mov\t\t%s, %s\n", dst, REG_A(lhs_type));
                }
            }
            else XXX;
        }

        return;
    }
    case TYPE_STRING: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tQWORD %s, rax\n", dst);
        return;
    }
    case TYPE_ENUM: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tDWORD %s, eax\n", dst);
        return;
    }
    case TYPE_POINTER: {
        if (rhs_type->kind == TYPE_POINTER) {
            POP(RAX);
            sb_append(&cg->code, "   mov\t\tQWORD %s, rax\n", dst);
        } else {
            // Follow the pointer to get the base address
            assert(dst_is_runtime_computed);
            Type *points_to = ((TypePointer *)(lhs_type))->pointer_to;
            if (points_to->kind == TYPE_POINTER) {
                sb_append(&cg->code, "   mov\t\trbx, %s\n", dst);
            }
            emit_simple_initialization(cg, 0, true, dst_is_relative_to_rsp, points_to, rhs_type);
        }
        return;
    }
    case TYPE_ARRAY: {
        POP(RAX); // pointer to array type

        TypeArray *array = (TypeArray *) lhs_type;

        switch (array->array_kind) {
        case ARRAY_FIXED: {
            sb_append(&cg->code, "   mov\t\trbx, [rax]\n", dst);
            sb_append(&cg->code, "   mov\t\t%s, rbx\n", dst);
            break;
        }
        case ARRAY_SLICE: {
            sb_append(&cg->code, "   lea\t\trbx, %s\n", dst);
            emit_struct_copy(cg, array->struct_defn, 0, 0);
            break;
        }
        case ARRAY_DYNAMIC: {
            sb_append(&cg->code, "   lea\t\trbx, %s\n", dst);
            emit_struct_copy(cg, array->struct_defn, 0, 0);
            break;
        }
        }

        return;
    }
    case TYPE_STRUCT: {
        POP(RAX);
    
        // @Speed: We should move structs smaller than 8 bytes in registers instead of doing a memcpy
        TypeStruct *struct_defn = (TypeStruct *) lhs_type;

        sb_append(&cg->code, "   ; Copy struct\n");

        if (struct_defn->head.size <= 8) {
            // Copy the the struct from rax
            sb_append(&cg->code, "   mov\t\t%s, %s\n", dst, REG_A((Type *)struct_defn));
        }
        else {
            if (!dst_is_runtime_computed) {
                sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", dst_offset);
            }
            emit_struct_copy(cg, struct_defn, 0, 0);
        }

        return;
    }
    default:
    XXX;
    }
}

// Allocates the array literal relative to base_offset[rbp]. Returns nothing
void emit_array_literal(CodeGenerator *cg, AstArrayLiteral *array_lit, int base_offset) {
    array_lit->base_offset = base_offset;

    TypeArray *array = (TypeArray *)(array_lit->head.type);
    
    int elem_0_offset = base_offset;
        
    for (int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr *elem = ((AstExpr **) array_lit->expressions.items)[i];
        
        int elem_i_offset = elem_0_offset + (i * elem->type->size);
        
        set_flag(cg, CODEGEN_TRY_ASSIGN_EXPRESSION_TO_VARIABLE);
        cg->variable_address = elem_i_offset;
        emit_expression(cg, elem);
        
        if (elem->head.kind == AST_STRUCT_LITERAL) {
            // The expression was directly assigned to the element slot
            continue;
        }
        
        if (array->array_kind == ARRAY_DYNAMIC) {
            elem_i_offset = i * elem->type->size;
            sb_append(&cg->code, "   mov\t\trax, %d[rbp] ; elem %d\n", base_offset, i);
            sb_append(&cg->code, "   lea\t\trbx, %d[rax]\n", elem_i_offset);
        }

        emit_simple_initialization(cg, elem_i_offset, array->array_kind == ARRAY_DYNAMIC, false, elem->type, elem->type);
    }
}

// Allocates the struct literal relative to base_offset[rbp]
void emit_struct_literal(CodeGenerator *cg, AstStructLiteral *struct_lit, int base_offset) {

    TypeStruct *struct_defn = (TypeStruct *) struct_lit->head.type;

    // @Improvement: We should proably be smart about zero initializing just
    //               the members that don't have a designated initializer for
    zero_initialize(cg, (Type *) struct_defn, base_offset);

    for (int i = 0; i < struct_lit->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(struct_lit->initializers.items))[i];

        int member_offset = base_offset + init->member->member_offset;

        if (init->value->head.kind == AST_STRUCT_LITERAL) {
            emit_struct_literal(cg, (AstStructLiteral *)init->value, member_offset);
        } 
        else if (init->value->head.kind == AST_ARRAY_LITERAL) {
            emit_array_literal(cg, (AstArrayLiteral *) init->value, member_offset);
        } 
        else {
            emit_expression(cg, init->value);
            emit_simple_initialization(cg, member_offset, false, false, init->member->type, init->value->type);
        }
    }
}

// Allocates space within local storage by bumping down rbp by 'size' and returns the allocated address
int allocate_variable(CodeGenerator *cg, int size) {
    assert(cg->enclosing_function);

    cg->enclosing_function->base_ptr -= size;
    cg->enclosing_function->base_ptr = align_value(cg->enclosing_function->base_ptr, size > 8 ? 8 : size);

    return cg->enclosing_function->base_ptr;
}

void emit_declaration(CodeGenerator *cg, AstDeclaration *decl) {
    if (decl->flags & DECLARATION_CONSTANT) {
        assert(decl->expr->head.kind == AST_LITERAL);
        AstLiteral *lit = (AstLiteral *)(decl->expr);

        switch (lit->kind) {
        case LITERAL_BOOLEAN: break; // Immediate value is used
        case LITERAL_INTEGER: break; // Immediate value is used 
        case LITERAL_FLOAT:   sb_append(&cg->data, "   C_%s DD %lf\n", decl->ident->name, lit->as.value.floating); break; // :IdentifierNameAsConstant @Cleanup @FloatRefactor - Not accounting for float64
        case LITERAL_STRING:  sb_append(&cg->data, "   C_%s DB \"%s\"\n", decl->ident->name, lit->as.value.string.data); break; // :IdentifierNameAsConstant @Cleanup
        case LITERAL_NULL:    XXX; // @TODO
        case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
        }

        return;
    }

    decl->ident->stack_offset = allocate_variable(cg, decl->type->size);

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; Ln %d: $%s : %s = %d\n", decl->ident->head.start.line, decl->ident->name, type_to_str(decl->type), decl->ident->stack_offset);
    
    if (!decl->expr) {
        zero_initialize(cg, decl->type, decl->ident->stack_offset);
        return;
    }

    // Special zero initialization of fixed arrays
    if (decl->type->kind == TYPE_ARRAY && ((TypeArray *)decl->type)->array_kind == ARRAY_FIXED) {
        zero_initialize(cg, decl->type, decl->ident->stack_offset);
    }

    // The following "fast-path" code is here to omit emit_expression()
    // from first doing a copy of the value of the expression on the stack followed 
    // by a copy into the variable. emit_expression() will take the fast-path if 
    // the expression is only 1-level deep. e.g    a := Vector3{1, 2, 3};
    // In this example, the struct is directly moved into 'a' instead of doing
    // an extra copy. The same is true for array literals.

    set_flag(cg, CODEGEN_TRY_ASSIGN_EXPRESSION_TO_VARIABLE);
    cg->variable_address = decl->ident->stack_offset;
    emit_expression(cg, decl->expr);

    if (decl->expr->head.kind == AST_STRUCT_LITERAL) {
        // The value was directly assigned to the variable
        return;
    }
    
    emit_simple_initialization(cg, decl->ident->stack_offset, false, false, decl->type, decl->expr->type);
}

void emit_arithmetic_operator(CodeGenerator *cg, AstBinary *bin) {
    Type *l_type = bin->left->type;
    Type *r_type = bin->right->type;

    TypeKind l_kind = l_type->kind;
    TypeKind r_kind = r_type->kind;

    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    if (l_kind == TYPE_ENUM) l_kind = TYPE_INTEGER;
    if (r_kind == TYPE_ENUM) r_kind = TYPE_INTEGER;

    // Integer case
    if (l_kind == TYPE_INTEGER && r_kind == TYPE_INTEGER) {
        POP(RBX);
        POP(RAX);
        if      (bin->operator == '+') sb_append(&cg->code, "   add\t\trax, rbx\n");
        else if (bin->operator == '-') sb_append(&cg->code, "   sub\t\trax, rbx\n");
        else if (bin->operator == '*') sb_append(&cg->code, "   imul\t\trax, rbx\n");
        else if (bin->operator == '/') {
            sb_append(&cg->code, "   cqo\n");           // sign extend rax through rax:rdx needed for division for some reason???
            sb_append(&cg->code, "   idiv\t\trbx\n");
        } else exit(1); // Should not happen

        PUSH(RAX);
        return;
    }

    // Float case
    if ((l_kind == TYPE_FLOAT   && r_kind == TYPE_FLOAT) ||   
        (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) ||
        (l_kind == TYPE_FLOAT   && r_kind == TYPE_INTEGER)) {
        
        // @FloatRefactor - These operations should handle 32 and 64 bit floats
        emit_integer_to_float_conversion(cg, l_type, r_type);

        if (l_kind == TYPE_FLOAT && r_kind == TYPE_FLOAT) {
            if (l_type->size == 4 && r_type->size == 8) {
                sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
            }
            else if (l_type->size == 8 && r_type->size == 4) {
                sb_append(&cg->code, "   cvtss2sd\txmm1, xmm1\n");
            } 
        }

        bool use_64_bit_float_operation = (l_type->size == 8 || r_type->size == 8);

        if (use_64_bit_float_operation) {
            if      (bin->operator == '+') sb_append(&cg->code, "   addsd\t\txmm0, xmm1\n");
            else if (bin->operator == '-') sb_append(&cg->code, "   subsd\t\txmm0, xmm1\n");
            else if (bin->operator == '*') sb_append(&cg->code, "   mulsd\t\txmm0, xmm1\n");
            else if (bin->operator == '/') sb_append(&cg->code, "   divsd\t\txmm0, xmm1\n");
            else XXX;
        } else {
            if      (bin->operator == '+') sb_append(&cg->code, "   addss\t\txmm0, xmm1\n");
            else if (bin->operator == '-') sb_append(&cg->code, "   subss\t\txmm0, xmm1\n");
            else if (bin->operator == '*') sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n");
            else if (bin->operator == '/') sb_append(&cg->code, "   divss\t\txmm0, xmm1\n");
            else XXX;
        }

        if (use_64_bit_float_operation) {
            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
        } else {
            sb_append(&cg->code, "   movd\t\teax, xmm0\n");
        }
        PUSH(RAX);

        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_arithmetic_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_to_str(l_type), token_type_to_str(bin->operator), type_to_str(r_type));
    exit(1);
}


void emit_comparison_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_comparison_operator(bin->operator));

    Type *l_type = bin->left->type;
    Type *r_type = bin->right->type;

    TypeKind l_kind = l_type->kind;
    TypeKind r_kind = r_type->kind;

    if (l_kind == TYPE_ENUM) l_kind = TYPE_INTEGER;
    if (r_kind == TYPE_ENUM) r_kind = TYPE_INTEGER;

    const char *set_instruction = get_comparison_set_instruction(cg, bin);
    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    if (l_kind == TYPE_INTEGER && r_kind == TYPE_INTEGER) {
        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        PUSH(RAX);

        return;
    }
    if ((l_kind == TYPE_FLOAT   && r_kind == TYPE_FLOAT) ||   
        (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) ||
        (l_kind == TYPE_FLOAT   && r_kind == TYPE_INTEGER)) 
    { 
        emit_integer_to_float_conversion(cg, l_type, r_type);

        if (l_kind == TYPE_FLOAT && r_kind == TYPE_FLOAT) {
            if (l_type->size == 4 && r_type->size == 4) {
                sb_append(&cg->code, "   comiss\txmm0, xmm1\n");
            } else if (l_type->size == 8 && r_type->size == 8) {
                sb_append(&cg->code, "   comisd\txmm0, xmm1\n");
            } else if (l_type->size == 8 && r_type->size == 4) {
                sb_append(&cg->code, "   cvtss2sd\txmm1, xmm1\n");
                sb_append(&cg->code, "   comisd\txmm0, xmm1\n");
            } else if (l_type->size == 4 && r_type->size == 8) {
                sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
                sb_append(&cg->code, "   comisd\txmm0, xmm1\n");
            } else {
                XXX; // unreachable
            }
            sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
            PUSH(RAX);
            return;
        } else {
            Type *float_type = l_kind == TYPE_FLOAT ? l_type : r_type;
            if (float_type->size == 8) {
                sb_append(&cg->code, "   comisd\txmm0, xmm1\n");
            } else {
                sb_append(&cg->code, "   comiss\txmm0, xmm1\n");
            }
            sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
            PUSH(RAX);
            return;
        }
    }
    if (l_kind == TYPE_POINTER && r_kind == TYPE_POINTER) {
        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        PUSH(RAX);
        return;
    }

    if (l_kind == TYPE_BOOL && r_kind == TYPE_BOOL) {
        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   cmp\t\tal, bl\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        PUSH(RAX);
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_comparison_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_to_str(l_type), token_type_to_str(bin->operator), type_to_str(r_type));
    exit(1);
}

void emit_integer_to_float_conversion(CodeGenerator *cg, Type *l_type, Type *r_type) {
    if (l_type->kind == TYPE_FLOAT && r_type->kind == TYPE_INTEGER) {
        POP(RBX);
        sb_append(&cg->code, "   %s\txmm1, %s\n", cvtsi2ss_or_cvtsi2sd(l_type), REG_B(l_type));
        POP(RAX);
        sb_append(&cg->code, "   %s\t\txmm0, %s\n", movd_or_movq(l_type), REG_A(l_type));
    }
    else if (l_type->kind == TYPE_INTEGER && r_type->kind == TYPE_FLOAT) {
        POP(RBX);
        sb_append(&cg->code, "   %s\t\txmm1, %s\n", movd_or_movq(r_type), REG_B(r_type));
        POP(RAX);
        sb_append(&cg->code, "   %s\txmm0, %s\n", cvtsi2ss_or_cvtsi2sd(r_type), REG_A(r_type));
    } else {
        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   %s\t\txmm1, %s\n", movd_or_movq(r_type), REG_B(r_type));
        sb_append(&cg->code, "   %s\t\txmm0, %s\n", movd_or_movq(l_type), REG_A(l_type));
    }
}

void emit_boolean_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_boolean_operator(bin->operator));

    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    const char *bool_instruction = boolean_operator_to_instruction(bin->operator);
    sb_append(&cg->code, "\n");
    POP(RBX);
    POP(RAX);
    sb_append(&cg->code, "   %s\t\tal, bl\n", bool_instruction);
    PUSH(RAX);
}

const char *get_comparison_set_instruction(CodeGenerator *cg, AstBinary *bin) {
    TokenType op   = bin->operator;

    Type *lhs_type = bin->left->type;
    Type *rhs_type = bin->right->type;

    bool do_signed_comparison = false;

    if      (lhs_type->kind == TYPE_BOOL    && rhs_type->kind == TYPE_BOOL)    do_signed_comparison = false;
    else if (lhs_type->kind == TYPE_FLOAT   && rhs_type->kind == TYPE_INTEGER) do_signed_comparison = false;
    else if (lhs_type->kind == TYPE_INTEGER && rhs_type->kind == TYPE_FLOAT)   do_signed_comparison = false;
    else if (lhs_type->kind == TYPE_FLOAT   && rhs_type->kind == TYPE_FLOAT)   do_signed_comparison = false;
    else if (is_unsigned_integer(lhs_type)  && is_unsigned_integer(rhs_type))  do_signed_comparison = false;
    else if (lhs_type->kind == TYPE_POINTER && rhs_type->kind == TYPE_POINTER) do_signed_comparison = false;
    else if (lhs_type->kind == TYPE_ENUM || rhs_type->kind == TYPE_ENUM)       do_signed_comparison = true;
    else if (is_signed_integer(lhs_type) || is_signed_integer(rhs_type))       do_signed_comparison = true;
    else if (lhs_type->kind == TYPE_INTEGER && rhs_type->kind == TYPE_INTEGER) do_signed_comparison = true;
    else {
        report_error_ast(cg->parser, LABEL_ERROR, (Ast *) bin, "Internal Compiler Error: There were unhandled cases in 'comparison_operator_to_instruction()'. Left type = %s, Right type = %s", type_to_str(lhs_type), type_to_str(rhs_type));
        exit(1);
    }

    if (do_signed_comparison) {
        if (op == '<')                 return "setl";
        if (op == '>')                 return "setg";
        if (op == TOKEN_GREATER_EQUAL) return "setge";
        if (op == TOKEN_LESS_EQUAL)    return "setle";
        if (op == TOKEN_DOUBLE_EQUAL)  return "sete";
        if (op == TOKEN_NOT_EQUAL)     return "setne";
    }
    else {
        if (op == '<')                 return "setb";
        if (op == '>')                 return "seta";
        if (op == TOKEN_GREATER_EQUAL) return "setae";
        if (op == TOKEN_LESS_EQUAL)    return "setbe";
        if (op == TOKEN_DOUBLE_EQUAL)  return "sete";
        if (op == TOKEN_NOT_EQUAL)     return "setne";
    }

    report_error_ast(cg->parser, LABEL_ERROR, (Ast *) bin, "Internal Compiler Error: There were unhandled cases in 'comparison_operator_to_instruction()'. Left type = %s, Right type = %s", type_to_str(lhs_type), type_to_str(rhs_type));
    exit(1);
}

const char *boolean_operator_to_instruction(TokenType op) {
    if (op == TOKEN_LOGICAL_AND) return "and";
    if (op == TOKEN_LOGICAL_OR)  return "or ";
    if (op == '!')               return "not";

    printf("%s:%d: compiler-error: There were unhandled cases in 'boolean_operator_to_instruction'\n", __FILE__, __LINE__);
    exit(1);
}


int make_label_number(CodeGenerator *cg) {
    return cg->labels++;
}

// Pushes the member offset on the stack
MemberAccessResult emit_member_access(CodeGenerator *cg, AstMemberAccess *ma) {
    // @Speedup: It would make sense to try to calculate the entire member access offset at compile-time
    // to skip alot of push and pop operations. This was the initial idea about the MemberAccessResult
    MemberAccessResult result = {0, true};

    if (ma->access_kind == MEMBER_ACCESS_ENUM) {
        // We don't push the address of an enum value
        return result;
    }

    // Emit the left side as an lvalue
    set_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);
    emit_expression(cg, ma->left);

    // Special case for fixed arrays: Just return the address of the array
    if (ma->left->type->kind == TYPE_ARRAY && ((TypeArray *)ma->left->type)->array_kind == ARRAY_FIXED) {
        return result;
    }

    if (ma->left->type->kind == TYPE_POINTER) {
        TypePointer *ptr_type = (TypePointer *) ma->left->type;
        if (ptr_type->has_been_dereferenced) {
            // Skip
        } else {
            // Dereference the pointer
            POP(RBX);
            sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
            PUSH(RBX);
        }
    }

    // Don't bother adding to the offset if its the first member (offset = 0)
    if (ma->struct_member->member_offset > 0) {
        POP(RBX);
        sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);
        PUSH(RBX);
    }

    return result;
}


// If lvalue is true, the returned result will be the address of the indexed element, returned in rbx, otherwise it will be the value of the indexed value returned in rax
void emit_array_access(CodeGenerator *cg, AstArrayAccess *array_ac, bool lvalue) {
    assert(array_ac->accessing->type->kind == TYPE_ARRAY);

    TypeArray *array_type = (TypeArray *) array_ac->accessing->type;

    // Emit the left side as an lvalue
    set_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);
    emit_expression(cg, array_ac->accessing);

    // Emit index expression as rvalue
    emit_expression(cg, array_ac->index_expr);

    POP(RAX); // rax = index value
    POP(RBX); // rbx = address of array

    // Set rbx to the address of the 0'th element (.data pointer)
    if (array_type->array_kind == ARRAY_FIXED) {
        // Skip as the address of the array is already the address of the 0'th element
    } else {
        sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
    }

    // Add the integer result of the index expression to the lvalue
    int elem_size = ((TypeArray *)array_ac->accessing->type)->elem_type->size;
    sb_append(&cg->code, "   imul\t\trax, %d; Add index expr\n", elem_size);
    sb_append(&cg->code, "   add\t\trbx, rax\n");

    if (lvalue) {
        PUSH(RBX);
    } else {
        emit_move_and_push(cg, 0, true, array_ac->head.type, false);
    }
    // @Remember: To check that it also works on quick array access: e.g [1, 2, 3][0]
}

// Moves a value in src and pushes it on the stack. If src_is_runtime_computed = true, then the source offset is expected to be in rbx
void emit_move_and_push(CodeGenerator *cg, int src_offset, bool src_is_runtime_computed, Type *src_type, bool lvalue) {
    char src[16];
    if (src_is_runtime_computed) {
        sprintf(src, "[rbx]");
    } else {
        sprintf(src, "%d[rbp]", src_offset);
    }

    if (lvalue) {
        sb_append(&cg->code, "   lea\t\trax, %s\n", src);
        PUSH(RAX);
        return;
    }

    switch (src_type->kind) {
    case TYPE_BOOL: {
        sb_append(&cg->code, "   movzx\t\teax, BYTE %s\n", src);
        PUSH(RAX);
        return;
    }
    case TYPE_STRING:
    case TYPE_ENUM: {
        sb_append(&cg->code, "   mov\t\t%s, %s %s\n", REG_A(src_type), WIDTH(src_type), src);
        PUSH(RAX);
        return;
    }
    case TYPE_INTEGER: {
        sb_append(&cg->code, "   mov\t\t%s, %s %s\n", REG_A(src_type), WIDTH(src_type), src);
        if (is_signed_integer(src_type) && src_type->size != 8) {
            sb_append(&cg->code, "   movsx\t\trax, %s\n", REG_A(src_type));
        }
        PUSH(RAX);
        return;
    }
    case TYPE_FLOAT: {
        sb_append(&cg->code, "   mov\t\t%s, %s\n", REG_A(src_type), src);
        PUSH(RAX);
        return;
    }
    case TYPE_POINTER: {
        sb_append(&cg->code, "   mov\t\trax, %s\n", src);
        PUSH(RAX);
        return;
    }
    case TYPE_ARRAY: {
        sb_append(&cg->code, "   lea\t\trax, %s\n", src);
        PUSH(RAX);
        return;
    }
    case TYPE_STRUCT: {
        if (src_type->size <= 8) {
            sb_append(&cg->code, "   mov\t\t%s, %s\n", REG_A(src_type), src);
        } else {
            sb_append(&cg->code, "   lea\t\trax, %s\n", src);
        }

        PUSH(RAX);
        return;
    }
    default:
        printf("internal-compiler-error: Unhandled case %s in emit_move_and_push()", type_to_str(src_type));
        XXX;
    }
}

bool is_arithmetic_operator(TokenType op) {
    if (op == '+') return true;
    if (op == '-') return true;
    if (op == '*') return true;
    if (op == '/') return true;
    if (op == '%') return true;
    if (op == '^') return true;
    return false;
}

void emit_expression(CodeGenerator *cg, AstExpr *expr) {

    // Check if we can use a fast-path address to move to
    bool assigning_to_variable = is_flag_set(cg, CODEGEN_TRY_ASSIGN_EXPRESSION_TO_VARIABLE);
    bool emit_as_lvalue        = is_flag_set(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);

    // Reset any flags with the lifetime of 1 expression
    reset_flag(cg, CODEGEN_TRY_ASSIGN_EXPRESSION_TO_VARIABLE);
    reset_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);

    switch (expr->head.kind) {
    case AST_BINARY: {
        AstBinary *bin = (AstBinary *)(expr);
        TokenType op = bin->operator;
        if      (is_arithmetic_operator(op)) emit_arithmetic_operator(cg, bin);
        else if (is_comparison_operator(op)) emit_comparison_operator(cg, bin);
        else if (is_boolean_operator(op))    emit_boolean_operator(cg, bin); 
        else XXX;

        return;
    }
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *)(expr);
        if (unary->operator == OP_NOT) {
            emit_expression(cg, unary->expr);
            POP(RAX);
            sb_append(&cg->code, "   test\t\trax, rax\n");
            sb_append(&cg->code, "   sete\t\tal\n");
            sb_append(&cg->code, "   movzx\t\trax, al\n");
            PUSH(RAX);
            return;
        }
        if (unary->operator == OP_UNARY_MINUS) {
            emit_expression(cg, unary->expr);
            POP(RAX);
            Type *type = unary->expr->type;
            if (type->kind == TYPE_FLOAT) {
                if (type->size == 4) {
                    sb_append(&cg->code, "   movd\t\txmm0, eax\n");
                    sb_append(&cg->code, "   xorpd\t\txmm1, xmm1\n");
                    sb_append(&cg->code, "   subss\t\txmm1, xmm0\n");
                    sb_append(&cg->code, "   movd\t\teax, xmm1\n");
                } else {
                    sb_append(&cg->code, "   movq\t\txmm0, rax\n");
                    sb_append(&cg->code, "   xorpd\t\txmm1, xmm1\n");
                    sb_append(&cg->code, "   subsd\t\txmm1, xmm0\n");
                    sb_append(&cg->code, "   movq\t\trax, xmm1\n");
                }
            } else if (type->kind == TYPE_INTEGER) {
                sb_append(&cg->code, "   neg\t\trax\n");
            } else {
                XXX;
            }
            PUSH(RAX);
            return;
        }
        if (unary->operator == OP_ADDRESS_OF) {
            // Emit the lvalue of the expression
            set_flag(cg, CODEGEN_EMIT_EXPRESSION_AS_LVALUE);
            emit_expression(cg, unary->expr);
            return;
        }
        if (unary->operator == OP_POINTER_DEREFERENCE) {
            emit_expression(cg, unary->expr);

            Type *dereferenced_type = unary->head.type;
            POP(RBX);
            emit_move_and_push(cg, 0, true, dereferenced_type, false);

            if (dereferenced_type->kind == TYPE_POINTER) {
                // Mark the pointer as dereferenced so e.g a member access knows not to dereference the pointer again
                TypePointer *ptr_type = (TypePointer *) dereferenced_type;
                ptr_type->has_been_dereferenced = true;
            }

            return;
        }

        XXX;
    }
    case AST_SEMICOLON_EXPR: {
        // No-op
        return;
    }
    case AST_CAST: {
        emit_cast(cg, (AstCast *)(expr));
        return;
    }
    case AST_FUNCTION_CALL: {
        emit_function_call(cg, (AstFunctionCall *)(expr));
        PUSH(RAX);
        return;
    }
    case AST_ARRAY_ACCESS: {
        AstArrayAccess *array_ac  = (AstArrayAccess *)(expr);
        emit_array_access(cg, array_ac, emit_as_lvalue);
        return;
    }
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma  = (AstMemberAccess *)(expr);
        
        MemberAccessResult result = emit_member_access(cg, ma);
        if (emit_as_lvalue) {
            // Lvalue is already pushed
            return;
        } 

        if (ma->access_kind == MEMBER_ACCESS_STRUCT) {

            // Special case for member access on an fixed array type
            if (ma->left->type->kind == TYPE_ARRAY && ((TypeArray *)ma->left->type)->array_kind == ARRAY_FIXED) {
                TypeArray *fixed_array = (TypeArray *)ma->left->type;
                if (ma->struct_member->member_index == 0) { // .data
                    // Address of array is already pushed
                } else if (ma->struct_member->member_index == 1) { // .count
                    POP(RBX);
                    sb_append(&cg->code, "   push\t\t%d\n", fixed_array->count);        
                } else {
                    XXX;
                }
                return;
            }

            AstDeclaration *member = ma->struct_member;
            POP(RBX);
            emit_move_and_push(cg, result.base_offset, result.is_runtime_computed, member->type, false);
            return;
        } 
        else if (ma->access_kind == MEMBER_ACCESS_ENUM) {
            sb_append(&cg->code, "   push\t\t%d\n", ma->enum_member->value);
            INCR_PUSH_COUNT();
        } else {
            XXX;
        }

        return;
    }
    case AST_TYPEOF: {
        emit_typeof(cg, (AstTypeof *)expr);
        PUSH(RAX);
        return;
    }
    case AST_SIZEOF: {
        AstSizeof *ast_sizeof = (AstSizeof *) expr;
        sb_append(&cg->code, "   mov\t\trax, %d\n", ast_sizeof->expr->type->size);
        PUSH(RAX);
        return;
    }
    case AST_NEW: {
        emit_new(cg, (AstNew *)expr);
        PUSH(RAX);
        return;
    }
    case AST_ENUM_LITERAL: {
        AstEnumLiteral *elit = (AstEnumLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", elit->enum_member->value);
        INCR_PUSH_COUNT();
        return;
    }
    case AST_ARRAY_LITERAL: {
        AstArrayLiteral *array_lit = (AstArrayLiteral *)(expr);
        TypeArray *array_type = (TypeArray *) array_lit->head.type;
        
        // Get the address that we emit the array literal relative to
        int offset = -1;
        switch (array_type->array_kind) {
        case ARRAY_FIXED: {
            if (assigning_to_variable) {
                offset = cg->variable_address;
            } else {
                // This is probably a one-off array literal, e.g "foo(5, [1, 2, 3])"
                //                                                       ^^^^^^^^^------- This is stored only temporarily
                offset = push_temporary_value(cg->enclosing_function, expr->type->size);
            }
            break;
        }
        case ARRAY_SLICE: {
            // Allocate the underlying array in local storage and return a slice to it (data + count)
            offset = allocate_variable(cg, array_type->count * array_type->elem_type->size);
            break;
        }
        case ARRAY_DYNAMIC: {
            // Allocate the underlying array with malloc
            sb_append(&cg->code, "   mov\t\trdx, %d\n", array_type->elem_type->size);
            sb_append(&cg->code, "   mov\t\trcx, %d\n", array_type->capacity);
            sb_append(&cg->code, "   call\t\tcalloc\n");        // void* calloc( size_t num, size_t size );

            // Allocate a temporary pointer to the malloced array
            int array_pointer_address = push_temporary_value(cg->enclosing_function, 8);
            sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", array_pointer_address);

            offset = array_pointer_address;
            break;
        }}

        emit_array_literal(cg, array_lit, offset);
        
        switch (array_type->array_kind) {
        case ARRAY_FIXED: {
            // Return pointer to beginning of array (element 0)
            sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", offset);
            PUSH(RAX);
            break;
        }
        case ARRAY_SLICE: {
            int slice_offset = push_temporary_value(cg->enclosing_function, 16);
            sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", offset);
            sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", slice_offset + 0);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], %d\n",  slice_offset + 8, array_type->count);
            sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", slice_offset);
            PUSH(RAX);
            break;
        }
        case ARRAY_DYNAMIC: {
            int dyn_array_offset = push_temporary_value(cg->enclosing_function, 24);
            sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", offset);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n",   dyn_array_offset + 0);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], %lld\n" , dyn_array_offset + 8, array_type->count);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], %lld\n" , dyn_array_offset + 16, array_type->capacity);
            sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", dyn_array_offset);
            PUSH(RAX);
            break;
        }}

        break;
    }
    case AST_STRUCT_LITERAL: {
        AstStructLiteral *struct_literal = (AstStructLiteral *) expr;
        TypeStruct       *struct_defn    = (TypeStruct *) struct_literal->head.type;

        int allocation_offset = assigning_to_variable ? cg->variable_address : push_temporary_value(cg->enclosing_function, expr->type->size);

        emit_struct_literal(cg, struct_literal, allocation_offset);

        if (!assigning_to_variable) {
            if (struct_defn->head.size <= 8) {
                // Return the struct literal in rax
                sb_append(&cg->code, "   mov\t\t%s, %d[rbp]\n", REG_A((Type *) struct_defn), allocation_offset);
            } else {
                // Return pointer to struct literal in rax
                sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", allocation_offset);
            }
            PUSH(RAX);
        }

        break;
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)(expr);
        switch (lit->kind) {
        case LITERAL_INTEGER: {
            sb_append(&cg->code, "   mov\t\trax, %llu\n", lit->as.value.integer); // @Incomplete - Need to check if signed or unsigned!!!
            PUSH(RAX);
            return;
        }
        case LITERAL_FLOAT: {
            if (lit->head.type->size == 4) {
                sb_append(&cg->data, "   CF%d DD %.7lf\n", cg->constants, lit->as.value.floating);
                sb_append(&cg->code, "   movss\t\txmm0, [CF%d]\n", cg->constants);
                sb_append(&cg->code, "   movd\t\teax, xmm0\n");
                PUSH(RAX);
                cg->constants++;
            } else if (lit->head.type->size == 8) {
                sb_append(&cg->data, "   CF%d DQ %.15lf\n", cg->constants, lit->as.value.floating);
                sb_append(&cg->code, "   movsd\t\txmm0, [CF%d]\n", cg->constants);
                sb_append(&cg->code, "   movq\t\trax, xmm0\n");
                PUSH(RAX);
                cg->constants++;
            } else XXX;
            return;
        }
        case LITERAL_STRING: {
            sb_append(&cg->data, "   CS%d DB \"%s\", 0 \n", cg->constants, lit->as.value.string.data);
            sb_append(&cg->code, "   mov\t\trax, CS%d\n", cg->constants);
            PUSH(RAX);
            cg->constants++;
            return;
        }
        case LITERAL_BOOLEAN: {
            sb_append(&cg->code, "   push\t\t%d\n", lit->as.value.boolean ? 1 : 0);
            INCR_PUSH_COUNT();
            return;
        }
        case LITERAL_NULL: {
            PUSH("0");
            return;
        }
        case LITERAL_IDENTIFIER: {
            AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, lit->as.value.identifier.name);
            assert(ident);

            if (ident->belongs_to_decl && ident->belongs_to_decl->flags & DECLARATION_CONSTANT) {
                assert(ident->belongs_to_decl->expr->head.kind == AST_LITERAL);
                AstLiteral *lit = (AstLiteral *)(ident->belongs_to_decl->expr);
                switch (lit->kind) {
                    case LITERAL_BOOLEAN: {
                        sb_append(&cg->code, "   push\t\t%d\n", lit->as.value.boolean ? -1 : 0);
                        INCR_PUSH_COUNT();
                        return;
                    }
                    case LITERAL_INTEGER: {
                        sb_append(&cg->code, "   push\t\t%lld\n", lit->as.value.integer);
                        INCR_PUSH_COUNT();
                        return;
                    }
                    case LITERAL_FLOAT: {
                        if (lit->head.type->size == 4) {
                            sb_append(&cg->code, "   movss\t\txmm0, [C_%s]\n", ident->name); // :IdentifierNameAsConstant @Cleanup - Should the identifier name really be used as the constant name??? Not good if we have the same identifier name in two seperate blocks inside same function!
                            sb_append(&cg->code, "   movd\t\teax, xmm0\n");
                            PUSH(RAX);
                        } else if (lit->head.type->size == 8) {
                            sb_append(&cg->code, "   movsd\t\txmm0, [C_%s]\n", ident->name); // :IdentifierNameAsConstant
                            sb_append(&cg->code, "   movq\t\trax, xmm0\n");
                            PUSH(RAX);
                        } else XXX;
                        return;
                    }
                    case LITERAL_STRING: {
                        sb_append(&cg->code, "   mov\t\trax, C_%s\n", ident->name); // :IdentifierNameAsConstant @Cleanup
                        PUSH(RAX);
                        return;
                    }
                    case LITERAL_NULL: XXX; // @TODO
                    case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
                }
            } 
            else {
                emit_move_and_push(cg, ident->stack_offset, false, ident->type, emit_as_lvalue);
                return;
            }
        }}
        
        XXX;
    }
    default:
        printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression' with %s\n", __FILE__, __LINE__, ast_to_str((Ast *)expr));
        exit(1);
    }
}

// Convert type to one of the four intel data types / width's (BYTE, WORD, DWORD, QWORD)
char *WIDTH(Type *type) {
    if (type->size == 1) return "BYTE";
    if (type->size == 2) return "WORD";
    if (type->size <= 4) return "DWORD";
    if (type->size <= 8) return "QWORD";

    printf("Internal Compiler Error: Taking WIDTH() of type %s with size %d\n", type_to_str(type), type->size);
    XXX;
}

bool is_flag_set(CodeGenerator *cg, CodeGenFlags flag) {
    return cg->flags & flag;
}

void set_flag(CodeGenerator *cg, CodeGenFlags flag) {
    cg->flags |= flag;
}

void reset_flag(CodeGenerator *cg, CodeGenFlags flag) {
    cg->flags &= ~flag;
}