#include "typer.c"

typedef struct CodeGenerator {
    StringBuilder head;     // Declaring bit target and other misc stuff
    StringBuilder data;     // Corresponding to section .data
    StringBuilder code;     // Corresponding to section .text
    StringBuilder code_header;  // Header of externs, globals etc within the .text section

    Parser *parser;

    AstBlock        *current_scope;
    AstFunctionDefn *func;  // Current enclosing function
    TypeTable type_table;

    size_t constants;   // For float and string literals
    size_t labels;      // For coditional jumps

    int enum_scratch_buffers; // Used for printing enum values where an int to string conversion needs to take place
    int num_pushed_arguments;
    
} CodeGenerator;

typedef struct MemberAccessResult {
    int base_offset;
    bool is_runtime_computed;
} MemberAccessResult;

void go_nuts(CodeGenerator *cg, AstCode *code);
void emit_builtin_functions(CodeGenerator *cg);
void emit_header(CodeGenerator *cg);
void emit_footer(CodeGenerator *cg);

void emit_code(CodeGenerator *cg, AstCode *code);
void emit_statement(CodeGenerator *cg, Ast *node);
void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn);
void emit_function_call(CodeGenerator *cg, AstFunctionCall *call);
void emit_return(CodeGenerator *cg, AstReturn *ast_return);
void emit_print(CodeGenerator *cg, AstPrint *print_stmt);
void emit_assert(CodeGenerator *cg, AstAssert *assertion);
void emit_typeof(CodeGenerator *cg, AstTypeof *ast_typeof);
void emit_if(CodeGenerator *cg, AstIf *ast_if);
void emit_for(CodeGenerator *cg, AstFor *ast_for);
void emit_break_or_continue(CodeGenerator *cg, AstBreakOrContinue *boc);
void emit_enum(CodeGenerator *cg, AstEnum *ast_enum);
void emit_struct(CodeGenerator *cg, AstStruct *ast_struct);
void emit_declaration(CodeGenerator *cg, AstDeclaration *decl);
void emit_assignment(CodeGenerator *cg, AstAssignment *assign);
void emit_array_access(CodeGenerator *cg, AstArrayAccess *array_ac, bool lvalue);
void emit_expression(CodeGenerator *cg, AstExpr *expr);

void emit_integer_to_float_conversion(CodeGenerator *cg, Type *l_type, Type *r_type);
void emit_initialization(CodeGenerator *cg, int dest_offset, AstExpr *expr, Type *lhs_type, Type *rhs_type);
void emit_simple_initialization(CodeGenerator *cg, int dest_offset, bool dest_is_runtime_computed, Type *lhs_type, Type *rhs_type);
void emit_move_and_push(CodeGenerator *cg, int src_offset, bool src_is_runtime_computed, Type *src_type, bool lvalue);

MemberAccessResult emit_member_access(CodeGenerator *cg, AstMemberAccess *ma);

void check_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *comparison_operator_to_set_instruction(TokenType op);
const char *boolean_operator_to_instruction(TokenType op);
char *WIDTH(Type *type);

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
    cg.func           = NULL;
    cg.constants      = 0;
    cg.labels         = 0;
    cg.enum_scratch_buffers = 0;
    cg.num_pushed_arguments = 0;

    return cg;
}

void go_nuts(CodeGenerator *cg, AstCode *code) {
    check_main_exists(cg);
    emit_header(cg);
    emit_code(cg, code);
}

void check_main_exists(CodeGenerator *cg) {
    AstIdentifier *main = lookup_from_scope(cg->parser, cg->current_scope, "main", NULL);
    if (main == NULL) {
        printf("Error: Missing function 'main' as entry-point to the program\n");
        exit(1);
    }
}

void code_generator_dump(CodeGenerator *cg, const char *output_path) {
    FILE *f = fopen(output_path, "w");
    if (f == NULL) {
        printf("%s:%d: error: Failed to open file '%s'\n", __FILE__, __LINE__, output_path);
        exit(1);
    }

    fwrite(cg->head.buffer, 1, cg->head.cursor, f);
    fwrite(cg->data.buffer, 1, cg->data.cursor, f);
    fwrite(cg->code_header.buffer, 1, cg->code_header.cursor, f);
    fwrite("\n", 1, 1, f);
    fwrite(cg->code.buffer, 1, cg->code.cursor, f);

    fclose(f);
}

void emit_header(CodeGenerator *cg) {
    sb_append(&cg->head, "bits 64\n");
    sb_append(&cg->head, "default rel\n");
    sb_append(&cg->head, "\n");

    sb_append(&cg->data, "segment .data\n");
    sb_append(&cg->data, "   fmt_int   DB \"%s\", 0\n", "%lld");
    sb_append(&cg->data, "   fmt_float DB \"%s\", 0\n", "%lf");
    sb_append(&cg->data, "   fmt_string DB \"%s\", 0\n", "%s");
    sb_append(&cg->data, "   fmt_address DB \"0x%s\", 0\n", "%p");
    sb_append(&cg->data, "   string_false DB \"false\", 0\n");
    sb_append(&cg->data, "   string_true  DB \"true\", 0\n");
    sb_append(&cg->data, "   string_assert_fail  DB \"Assertion failed at line %s\", 10, 0\n", "%d");
    sb_append(&cg->code, "\n");

    sb_append(&cg->code_header, "segment .text\n");
    sb_append(&cg->code_header, "   global main\n");

    sb_append(&cg->code_header, "   extern printf\n");
    sb_append(&cg->code_header, "   extern sprintf\n");
    sb_append(&cg->code_header, "   extern ExitProcess\n");
    sb_append(&cg->code_header, "   extern malloc\n");

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
    case AST_TYPEOF:            emit_typeof(cg, (AstTypeof *)(node)); break;
    case AST_IF:                emit_if(cg, (AstIf *)(node)); break;
    case AST_FOR:               emit_for(cg, (AstFor *)(node)); break;
    case AST_ENUM:              emit_enum(cg, (AstEnum *)(node)); break;
    case AST_STRUCT:        break;
    default:
        printf("compiler-error: emit_statement not implemented for %s\n", ast_to_str(node));
        XXX;
    }
}

void emit_break_or_continue(CodeGenerator *cg, AstBreakOrContinue *boc) {
    assert(boc->enclosing_for);

    if (boc->token.type == TOKEN_BREAK) {
        sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing_for->done_label);
    } else if (boc->token.type == TOKEN_CONTINUE) {
        sb_append(&cg->code, "   jmp\t\tL%d\n", boc->enclosing_for->post_expression_label);
    } else {
        XXX; // Shouldn't happen
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
    emit_expression(cg, assign->expr);

    bool offset_is_runtime_computed = false;
    int base_offset = 0;
    if (assign->lhs->head.kind == AST_LITERAL) {
        AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(assign->lhs))->as.value.identifier.name, (Ast *)assign->lhs);
        assert(ident);

        if (ident->type->kind == TYPE_POINTER) {
            if (assign->expr->type->kind != TYPE_POINTER) {
                // We are updating the value at the address that is pointed to
                sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);
                base_offset = 0;
                offset_is_runtime_computed = true;
            } else {
                // We are updating the address value of the pointer
                base_offset = ident->stack_offset;
                offset_is_runtime_computed = false;
            }
        } else {
            base_offset = ident->stack_offset;
            offset_is_runtime_computed = false;
        }
    }
    else if (assign->lhs->head.kind == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)(assign->lhs);
        MemberAccessResult result = emit_member_access(cg, ma);
        base_offset = result.base_offset;
        offset_is_runtime_computed = result.is_runtime_computed;
    }
    else if (assign->lhs->head.kind == AST_ARRAY_ACCESS) {
        AstArrayAccess *array_ac = (AstArrayAccess *)(assign->lhs);
        emit_array_access(cg, array_ac, true);
        offset_is_runtime_computed = true;
    }
    else {
        XXX;
    }

    char address_str[16];
    if (offset_is_runtime_computed) {
        sprintf(address_str, "[rbx]");
    } else {
        sprintf(address_str, "%d[rbp]", base_offset);
    }

    if (assign->op == ASSIGN_EQUAL) {
        emit_simple_initialization(cg, base_offset, offset_is_runtime_computed, assign->lhs->type, assign->expr->type);
        return;
    }

    if (assign->lhs->type->kind == TYPE_INTEGER) {
        POP(RAX);
        switch (assign->op) {
        case ASSIGN_PLUS_EQUAL:  sb_append(&cg->code, "   add\t\t%s, eax\n", address_str); break;
        case ASSIGN_MINUS_EQUAL: sb_append(&cg->code, "   sub\t\t%s, eax\n", address_str); break;
        case ASSIGN_TIMES_EQUAL: {
            sb_append(&cg->code, "   mov\t\tecx, DWORD %s\n", address_str);
            sb_append(&cg->code, "   imul\t\tecx, eax\n"); 
            sb_append(&cg->code, "   mov\t\tDWORD %s, ecx\n", address_str);
            break;
        } 
        
        case ASSIGN_DIVIDE_EQUAL: {
            sb_append(&cg->code, "   mov\t\tecx, eax\n");
            sb_append(&cg->code, "   mov\t\teax, DWORD %s\n", address_str);
            sb_append(&cg->code, "   cqo\n");
            sb_append(&cg->code, "   idiv\t\tecx\n");
            sb_append(&cg->code, "   mov\t\tDWORD %s, eax\n", address_str);
            break;
        }
        default: exit(1); // Unreachable
        }
    }
    else if (assign->lhs->type->kind == TYPE_FLOAT) {

        Type *lhs_type = assign->lhs->type;
        Type *rhs_type = assign->expr->type;

        POP(RBX);
        if (rhs_type->kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   %s\txmm1, rbx\n", cvtsi2ss_or_cvtsi2sd(lhs_type));
        } else {
            sb_append(&cg->code, "   %s\t\txmm1, %s\n", movd_or_movq(rhs_type), REG_B(rhs_type));
        }

        sb_append(&cg->code, "   %s\t\txmm0, %s\n", movd_or_movq(lhs_type), address_str);

        // @FloatRefactor - These operations should handle 32 and 64 bit floats
        switch (assign->op) {
        case ASSIGN_PLUS_EQUAL:   sb_append(&cg->code, "   addss\t\txmm0, xmm1\n"); break;
        case ASSIGN_MINUS_EQUAL:  sb_append(&cg->code, "   subss\t\txmm0, xmm1\n"); break;
        case ASSIGN_TIMES_EQUAL:  sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n"); break;
        case ASSIGN_DIVIDE_EQUAL: sb_append(&cg->code, "   divss\t\txmm0, xmm1\n"); break;
        default: exit(1); // Unreachable
        }

        sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(lhs_type), address_str);
    } else {
        XXX;
    }
}

void emit_for(CodeGenerator *cg, AstFor *ast_for) {
    int cond_label            = make_label_number(cg);
    int post_expression_label = make_label_number(cg);
    int done_label            = make_label_number(cg);

    ast_for->post_expression_label = post_expression_label;
    ast_for->done_label = done_label;
    
    if (!ast_for->iterable) {
        sb_append(&cg->code, "L%d:\n", cond_label);
        emit_block(cg, ast_for->body);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);
        sb_append(&cg->code, "L%d:\n", done_label);
    }
    else if (ast_for->iterable->head.kind == AST_RANGE_EXPR) {
        AstRangeExpr * range = (AstRangeExpr *)(ast_for->iterable);
        
        // allocate space for start and end range values 
        int size_iterator = ast_for->iterator->type->size;

        cg->func->base_ptr = align_value(cg->func->base_ptr, size_iterator);

        int offset_iterator = cg->func->base_ptr - size_iterator;
        int offset_start    = offset_iterator - 8;
        int offset_end      = offset_iterator - 16;
        int offset_index    = offset_iterator - 24; // Only used if an index is also specified

        cg->func->base_ptr -= size_iterator + 16;
        if (ast_for->index) {
            cg->func->base_ptr -= 8;
            ast_for->index->stack_offset = offset_index;
        }
        ast_for->iterator->stack_offset = offset_iterator;


        emit_expression(cg, range->start);
        emit_expression(cg, range->end);

        POP(RBX);
        POP(RAX);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", offset_start);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rbx\n", offset_end);

        // initialize iterator and optionally the index
        sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", offset_start);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", offset_iterator);
        if (ast_for->index)  sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", offset_index);

        sb_append(&cg->code, "L%d:\n", cond_label);
        sb_append(&cg->code, "   mov\t\trax, %d[rbp]\n", offset_end);
        sb_append(&cg->code, "   cmp\t\t%d[rbp], rax\n", offset_iterator);
        sb_append(&cg->code, "   %s\t\tL%d\n", range->inclusive ? "jg" : "jge", done_label);

        emit_block(cg, ast_for->body);
        
        sb_append(&cg->code, "L%d:\n", post_expression_label);
        sb_append(&cg->code, "   inc\t\tQWORD %d[rbp]\n", offset_iterator);
        if (ast_for->index)  sb_append(&cg->code, "   inc\t\tQWORD %d[rbp]\n", offset_index);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    } 
    else {
        AstIdentifier *iterator = ast_for->iterator;
        AstIdentifier *index    = ast_for->index;
        AstExpr *iterable       = ast_for->iterable;

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index
        //
        // :WrongForLoopSizing @Investigate - Might have to do with the fact that we are not aligning the stack before allocating space for the iterator!
        int aligned_iterator_size = align_value(iterator->type->size, 8);
        int offset_iterator       = cg->func->base_ptr - aligned_iterator_size;
        int offset_data           = offset_iterator - 8;
        int offset_count          = offset_iterator - 16;
        int offset_index          = offset_iterator - 24;
        cg->func->base_ptr             -= 24 + aligned_iterator_size;
        
        assert(iterator);
        iterator->stack_offset = offset_iterator;
        if (index) {
            index->stack_offset = offset_index;
        }

        emit_expression(cg, iterable);

        sb_append(&cg->code, "   ; For-loop\n");
        POP(RAX);
        POP(RBX);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rax     ; data\n", offset_data);
        sb_append(&cg->code, "   mov\t\t%d[rbp], rbx     ; count\n", offset_count);
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
                sb_append(&cg->code, "   mov\t\trax, [rbx + %d]\n", i * 8);
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

void emit_return(CodeGenerator *cg, AstReturn *ast_return) {
    emit_expression(cg, ast_return->expr);

    // Place the return value into the correct register
    // https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#return-values

    Type *expr_type = ast_return->expr->type;
    Type *return_type = ast_return->enclosing_function->return_type;

    if (is_untyped_literal(expr_type) && return_type->kind == TYPE_FLOAT) {

        // Here we just convert the integer to a float
        assert(expr_type->kind == TYPE_INTEGER);
        POP(RAX);
        sb_append(&cg->code, "   %s\txmm0, %s\n", cvtsi2ss_or_cvtsi2sd(return_type), REG_A(expr_type));
        sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(return_type), REG_A(expr_type));
        PUSH(RAX);
    }

    sb_append(&cg->code, "   jmp\t\tL%d\n", ast_return->enclosing_function->return_label);
}

void emit_pop_r_value(CodeGenerator *cg, Type *type) {
    switch (type->kind) {
    case TYPE_VOID: {
        sb_append(&cg->code, "   mov\t\trax, 0\n");
        return;
    }
    case TYPE_BOOL:
    case TYPE_INTEGER:
    case TYPE_ENUM:
    case TYPE_STRING: {
        POP(RAX);
        return;
    }
    case TYPE_FLOAT: {
        // @Incomplete - Missing int to float conversion
        POP(RAX);
        sb_append(&cg->code, "   %s\t\txmm0, %s\n", movd_or_movq(type), REG_A(type));
        return;
    }
    case TYPE_POINTER: {
        POP(RAX);
        return;
    }
    case TYPE_ARRAY: {
        POP(RAX); // data
        POP(RCX); // count
        return;
    }
    case TYPE_STRUCT: {
        TypeStruct *struct_defn = (TypeStruct *)type;
        
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
    assert(width >= 0 && width <= 8);

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

void MOV_ADDR_REG(CodeGenerator *cg, int dst_addr, Register src_reg, Type *src_type) {
    char *src_reg_string = register_string(src_reg, src_type->size);

    if (src_type->kind == TYPE_FLOAT) {
        if (src_type->size == 8) sb_append(&cg->code, "   movq\t\t%d[rbp], %s\n", dst_addr, src_reg_string);
        else                     sb_append(&cg->code, "   movd\t\t%d[rbp], %s\n", dst_addr, src_reg_string);
    } else {
        sb_append(&cg->code, "   mov\t\t%d[rbp], %s\n", dst_addr, src_reg_string);
    }
}

void MOV_ADDR_ADDR(CodeGenerator *cg, int dst_addr, Type *dst_type, int src_addr, Type *src_type) {
    if (src_type->kind == TYPE_FLOAT) {
        sb_append(&cg->code, "   %s\t\t%s, %d[rbp]\n", movd_or_movq(src_type), REG_A(src_type), src_addr);
        sb_append(&cg->code, "   %s\t\t%d[rbp], %s\n", movd_or_movq(dst_type), dst_addr, REG_A(src_type));
    } else {
        sb_append(&cg->code, "   mov\t\t%s, %d[rbp]\n", REG_A(src_type), src_addr);
        sb_append(&cg->code, "   mov\t\t%d[rbp], %s\n", dst_addr, REG_A(src_type));
    }
}

void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn) {
    cg->func = func_defn;

    if (func_defn->is_extern) {
        sb_append(&cg->code_header, "   extern %s\n", func_defn->identifier->name);
        return;
    }

    int bytes_args      = func_defn->num_bytes_args * 2; // We need twice the amount of space for arguments as we also need temporary space for them while popping arguments in a function call
    int bytes_locals    = func_defn->num_bytes_locals;
    int bytes_total     = align_value(32 + bytes_args + bytes_locals, 16);

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "; bytes locals   : %d\n", bytes_locals);
    sb_append(&cg->code, "; bytes arguments: %d\n", bytes_args);
    sb_append(&cg->code, "; bytes total    : %d\n", bytes_total);
    sb_append(&cg->code, "%s:\n", func_defn->identifier->name);

    //
    // Prolog
    //

    PUSH(RBP);
    sb_append(&cg->code, "   mov\t\trbp, rsp\n");
    sb_append(&cg->code, "   sub\t\trsp, %d\n", bytes_total);
    
    // Reserve space for stack arguments to called functions
    cg->func->base_ptr -= bytes_args / 2.0;
    
    // Move arguments to this function into their home addresses
    for (int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        Type           *param_type = param->type;
        
        // Bump the base pointer down by the size of the parameter type
        cg->func->base_ptr -= param_type->size;
        cg->func->base_ptr  = align_value(cg->func->base_ptr, param_type->size);
        
        // Assign the offset to the parameter
        int param_offset = cg->func->base_ptr;
        param->ident->stack_offset = param_offset;
        
        if (i < 4) {
            Register arg_reg = 0;
            if (param_type->kind == TYPE_FLOAT) {
                if (i == 0) arg_reg = REG_XMM0;
                if (i == 1) arg_reg = REG_XMM1;
                if (i == 2) arg_reg = REG_XMM2;
                if (i == 3) arg_reg = REG_XMM3;
            } else {
                if (i == 0) arg_reg = REG_RCX;
                if (i == 1) arg_reg = REG_RDX;
                if (i == 2) arg_reg = REG_R8;
                if (i == 3) arg_reg = REG_R9;
            }
            assert(arg_reg != 0);
            
            MOV_ADDR_REG(cg, param_offset, arg_reg, param_type);
        } else {
            // Argument was passed on the stack and can be found at rbp+48 and above
            int arg_base_offset = 48;   // shadow-space=32, func call=8, push rbp=8
            int arg_offset = arg_base_offset + (i - 4) * 8;
            sb_append(&cg->code, "   mov\t\trax, [rbp+%d]\n", arg_offset);
            sb_append(&cg->code, "   mov\t\t%d[rbp], %s\n", param_offset, REG_A(param_type));
        }
    }

    // Make a label for the return code so that return statements within the function can jump to this
    int return_label = make_label_number(cg);
    func_defn->return_label = return_label;

    emit_block(cg, func_defn->body);

    //
    // Epilog
    //
    sb_append(&cg->code, "L%d:\n", return_label);
    emit_pop_r_value(cg, func_defn->return_type);
    sb_append(&cg->code, "   add\t\trsp, %d\n", bytes_total);
    POP(RBP);
    sb_append(&cg->code, "   ret\n");
}

void emit_cast(CodeGenerator *cg, AstCast *cast) {
    emit_expression(cg, cast->expr);

    Type *from = cast->expr->type;
    Type *to   = cast->cast_to;


    //
    //  Follows the casting rules defined in can_cast_explicitly()
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


    printf("%s:%d: Compiler Error: There were unhandled cases in 'emit_cast'. Casting from '%s' -> '%s' \n", __FILE__, __LINE__, type_to_str(from), type_to_str(to));
    return;
}

void move_func_argument_to_register_or_stack(CodeGenerator *cg, int arg_index, int arg_count, Type *arg_type, Type *param_type) {
    if (arg_index < 4) {
        // Pass the argument in a register
        Register reg = 0;
        if (arg_type->kind == TYPE_FLOAT || (param_type->kind == TYPE_FLOAT && is_untyped_literal(arg_type) && ((TypePrimitive *)arg_type)->kind == PRIMITIVE_UNTYPED_INT) ) {
            if (arg_index == 0) reg = REG_XMM0;
            if (arg_index == 1) reg = REG_XMM1;
            if (arg_index == 2) reg = REG_XMM2;
            if (arg_index == 3) reg = REG_XMM3;
            assert(reg != 0);

            char *xmm_reg = register_string(reg, 0);

            sb_append(&cg->code, "   %s\t\t%s, %s\n", movd_or_movq(param_type), xmm_reg, REG_A(param_type));
        } 
        else {
            if (arg_type->kind == TYPE_ARRAY) XXX;
            if (arg_type->kind == TYPE_STRUCT) XXX;
            if (arg_type->kind == TYPE_FUNCTION) XXX;

            if (arg_index == 0) reg = REG_RCX;
            if (arg_index == 1) reg = REG_RDX;
            if (arg_index == 2) reg = REG_R8;
            if (arg_index == 3) reg = REG_R9;
            assert(reg != 0);
            
            char *gpr_reg = register_string(reg, arg_type->size);
            
            sb_append(&cg->code, "   mov\t\t%s, %s\n", gpr_reg, REG_A(arg_type));
        }
    } else {
        // Put the argument into temporary storage
        int temp_idx    = arg_count - arg_index;
        int temp_offset = -temp_idx * 8;
        sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", temp_offset);
    }
}

void emit_function_call(CodeGenerator *cg, AstFunctionCall *call) {

    int arg_count = call->arguments.count;

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
                
                if (param_type->kind == TYPE_FLOAT && is_untyped_literal(arg_type) && ((TypePrimitive *)arg_type)->kind == PRIMITIVE_UNTYPED_INT) {
                    // Convert the untyped int to be a float
                    sb_append(&cg->code, "   %s\txmm0, %s\n", cvtsi2ss_or_cvtsi2sd(param_type), REG_A(arg_type));
                    sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(param_type), REG_A(arg_type));
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
            case TYPE_ARRAY:
            case TYPE_STRUCT:
            default: XXX;
        }

        move_func_argument_to_register_or_stack(cg, i, arg_count, arg_type, param_type);

    }

    if (arg_count > 4) {
        // Put the arguments from temporary storage onto the stack now that the stack pointer is not being moved by the popping of arguments
        int stack_arg_count = arg_count - 4;
        for (int i = 0; i < stack_arg_count; i++) {
            int temp_offset   = (i + 1) * 8;
            int stack_offset  = 32 + (stack_arg_count - i - 1) * 8;

            sb_append(&cg->code, "   mov\t\trax, QWORD -%d[rbp]\n", temp_offset);
            sb_append(&cg->code, "   mov\t\tQWORD [rsp+%d], rax\n", stack_offset);
        }
    }


    AstFunctionDefn *func_defn = call->func_defn;

    if (func_defn->call_conv == CALLING_CONV_MSVC) {
        bool align_stack = cg->num_pushed_arguments % 2 == 1;

        // Boundary needs to be 16 byte aligned
        if (align_stack) sb_append(&cg->code, "   sub\t\trsp, 8\n");
        sb_append(&cg->code, "   call\t\t%s\n", call->identifer->name);
        if (align_stack) sb_append(&cg->code, "   add\t\trsp, 8\n");
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
    Type *expr_type = ast_typeof->expr->type;

    char *type_name  = type_to_str(expr_type);
    int   type_label = make_label_number(cg);

    sb_append(&cg->data, "   T%d\tDB \"%s\", 10, 0\n", type_label, type_name);
    sb_append(&cg->code, "   mov\t\trcx, T%d\n", type_label);
    sb_append(&cg->code, "   call\t\tprintf\n");
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

void move_argument_to_register_or_stack(CodeGenerator *cg, int arg_index, int arg_total) {
    if (arg_index > 3) {
        // Goes on the stack

        int offset = -32;   // Start at shadow space
        offset    -= (arg_total - arg_index - 1) * 8;

        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", offset);
    } else {
        // Goes in register

        char *reg = NULL;
        if (arg_index == 0) reg = "rcx";
        if (arg_index == 1) reg = "rdx";
        if (arg_index == 2) reg = "r8";
        if (arg_index == 3) reg = "r9";
        if (reg == NULL) {
            printf("%s:%d: Internal Compiler Error: Failed to assign argument to a valid msvc register. arg_index = %d, arg_total = %d", __FILE__, __LINE__, arg_index, arg_total);
            exit(1);
        }

        sb_append(&cg->code, "   mov\t\t%s, rax\n", reg);
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
        // @Note @Hack - register r9 needs to be kept safe during the printout of the structure
        if (struct_depth == 0) {
            POP(R9);
        }

        AstStruct *struct_ = ((TypeStruct *)arg_type)->node;
        DynamicArray members = struct_->scope->members;
        for (int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)members.items)[i];

            int   m_offset = struct_base_offset + member->member_offset;
            Type *m_type   = member->type;

            if (m_type->kind == TYPE_STRUCT) {
                // Save the base offset before recursing
                struct_base_offset = m_offset;
            } else {
                // Push each member on the stack
                sb_append(&cg->code, "   lea\t\trbx, %d[r9]\n", m_offset);
                emit_move_and_push(cg, 0, true, m_type, false);
            }

            // Do any transforms necessary to print the struct member
            emit_printable_value(cg, m_type, num_enum_arguments, struct_depth + 1, struct_base_offset);
        }
        break;
    }
    default:
        break;
    }
}

void pop_struct_members_from_print(CodeGenerator *cg, AstStruct *struct_, int *c_arg_index, int c_args) {
    DynamicArray members = struct_->scope->members;
    for (int i = struct_->scope->members.count - 1; i >= 0; i--) {
        AstDeclaration *member = ((AstDeclaration **)members.items)[i];

        if (member->type->kind == TYPE_STRUCT) {
            AstStruct *nested_struct = ((TypeStruct *)member->type)->node;
            pop_struct_members_from_print(cg, nested_struct, c_arg_index, c_args);
        } else {
            POP(RAX);
            move_argument_to_register_or_stack(cg, *c_arg_index, c_args);
            *c_arg_index -= 1;
        }

    }
}

void emit_print(CodeGenerator *cg, AstPrint *print) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; expression of print\n");

    // Push all the arguments on the stack
    cg->num_pushed_arguments = 0;
    int num_enum_arguments = 0;
    for (int i = 1; i < print->arguments.count; i++) {
        AstExpr *arg = ((AstExpr **)print->arguments.items)[i];
        Type    *arg_type = arg->type;

        emit_expression(cg, arg);
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
            case TYPE_POINTER: {
                POP(RAX);
                move_argument_to_register_or_stack(cg, c_arg_index, c_args);
                c_arg_index -= 1;
                break;
            }
            case TYPE_ARRAY: {
                POP(RAX);
                POP(RBX); // Don't use the length for anything
                move_argument_to_register_or_stack(cg, c_arg_index, c_args);
                c_arg_index -= 1;
                break;
            }
            case TYPE_STRUCT: {
                AstStruct *struct_ = ((TypeStruct *)arg_type)->node;
                pop_struct_members_from_print(cg, struct_, &c_arg_index, c_args);
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
        for (int i = 0; i < c_args - 4; i++) {
            int temp_offset   = 32 + (i * 8);
            int stack_offset  = 32 + (c_args - 4 - i - 1) * 8;

            sb_append(&cg->code, "   mov\t\trax, QWORD -%d[rbp]\n", temp_offset);
            sb_append(&cg->code, "   mov\t\tQWORD [rsp + %d], rax\n", stack_offset);
        }
    }

    // No need to align the stack as all the arguments are in registers
    sb_append(&cg->code, "   call\t\tprintf\n");
        
}

void zero_initialize(CodeGenerator *cg, int dest_offset, Type *type, bool is_array_initialization) {
    switch (type->kind) {
    case TYPE_INTEGER: sb_append(&cg->code, "   mov\t\t%s %d[rbp], 0\n", WIDTH(type), dest_offset); break;
    case TYPE_FLOAT:   sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", dest_offset); break;
    case TYPE_BOOL:    sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], 0\n",  dest_offset); break;
    case TYPE_STRING:  sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dest_offset); break;
    case TYPE_ENUM:    sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", dest_offset); break;
    case TYPE_POINTER: sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dest_offset); break;
    case TYPE_STRUCT: {
        AstStruct *struct_defn = ((TypeStruct *)(type))->node;
        assert(struct_defn != NULL);

        for (int i = 0; i < struct_defn->scope->members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)(struct_defn->scope->members.items))[i];
            zero_initialize(cg, dest_offset + member->member_offset, member->type, member->type->kind == TYPE_ARRAY);
        }
        break;
    }
    case TYPE_ARRAY : {
        // @Note - Maybe it should only be done when
        // the inner type is a struct, where the struct might contrain default values that we would
        // want to initialize

        TypeArray *array = (TypeArray *)(type);
        if (is_array_initialization) {
            for (int i = 0; i < array->capicity; i++) {
                zero_initialize(cg, dest_offset + (i * array->elem_type->size), array->elem_type, true);
            }
        } else {
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dest_offset); // data
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", dest_offset + 8); // count
        }

        break;
    }
    default:
        printf("%s:%d: compiler-error: There were unhandled cases in 'zero_initialize'\n", __FILE__, __LINE__);
        exit(1);
    }
}

void emit_simple_initialization(CodeGenerator *cg, int dest_offset, bool dest_is_runtime_computed, Type *lhs_type, Type *rhs_type) {

    char address_str[16];
    if (dest_is_runtime_computed) {
        sprintf(address_str, "[rbx]");
    } else {
        sprintf(address_str, "%d[rbp]", dest_offset);
    }

    switch (lhs_type->kind) {
    case TYPE_BOOL: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tBYTE %s, al\n", address_str);
        return;
    }
    case TYPE_INTEGER: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\t%s %s, %s\n", WIDTH(lhs_type), address_str, REG_A(lhs_type));
        return;
    }
    case TYPE_FLOAT: {
        POP(RAX);
        if (rhs_type->kind == TYPE_INTEGER) {
            // @Note - I feel like this conversion from int to float should be dealt with at the typing level instead of here. 
            // One idea is to just change it at the check_delcarartion level. If we have a float on the left and an integer on the right, just change the right hand side type to be integer
            sb_append(&cg->code, "   %s\txmm0, rax\n", cvtsi2ss_or_cvtsi2sd(lhs_type));
            sb_append(&cg->code, "   %s\t\t%s, xmm0\n", movd_or_movq(lhs_type), address_str);
        }
        else if (rhs_type->kind == TYPE_FLOAT) {
            if (lhs_type->size == 8 && rhs_type->size == 4) {
                sb_append(&cg->code, "   movd\t\txmm0, eax\n");
                sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
                sb_append(&cg->code, "   movsd\t\t%s, xmm0\n", address_str);
            }
            else if (lhs_type->size == 4 && rhs_type->size == 8) {
                sb_append(&cg->code, "   movq\t\txmm0, rax\n");
                sb_append(&cg->code, "   cvtsd2ss\txmm0, xmm0\n");
                sb_append(&cg->code, "   movss\t\t%s, xmm0\n", address_str);
            }
            else {
                sb_append(&cg->code, "   mov\t\t%s, %s\n", address_str, REG_A(lhs_type));
            }
        }
        else XXX;

        return;
    }
    case TYPE_STRING: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tQWORD %s, rax\n", address_str);
        return;
    }
    case TYPE_ENUM: {
        POP(RAX);
        sb_append(&cg->code, "   mov\t\tDWORD %s, eax\n", address_str);
        return;
    }
    case TYPE_POINTER: {
        if (rhs_type->kind == TYPE_POINTER) {
            POP(RAX);
            sb_append(&cg->code, "   mov\t\tQWORD %s, rax\n", address_str);
        } else {
            // Follow the pointer to get the base address
            assert(dest_is_runtime_computed);
            Type *points_to = ((TypePointer *)(lhs_type))->pointer_to;
            if (points_to->kind == TYPE_POINTER) {
                sb_append(&cg->code, "   mov\t\trbx, %s\n", address_str);
            }
            emit_simple_initialization(cg, 0, true, points_to, rhs_type);
        }
        return;
    }
    case TYPE_ARRAY: {
        POP(RAX); // data
        POP(RCX); // count
        if (dest_is_runtime_computed) {
            sb_append(&cg->code, "   mov\t\tQWORD [rbx], rax\n");
            sb_append(&cg->code, "   mov\t\tQWORD [rbx + 8], rax\n");
        } else {
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", dest_offset);
            sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rcx\n", dest_offset + 8);
        }
        return;
    }
    default:
    XXX;
    }
}

void emit_struct_initialization(CodeGenerator *cg, int dest_offset, AstStructLiteral *lit) {
    for (int i = 0; i < lit->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(lit->initializers.items))[i];

        int member_offset = dest_offset + init->member->member_offset;

        emit_initialization(cg, member_offset, init->value, init->member->type, init->value->type);
    }
}

void emit_array_initialization(CodeGenerator *cg, int dest_offset, AstArrayLiteral *array_lit, Type *lhs_type) {
    assert(lhs_type->kind == TYPE_ARRAY);
    
    int elem_size   = ((TypeArray *)(array_lit->head.type))->elem_type->size;
    for (int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr *expr = ((AstExpr **)(array_lit->expressions.items))[i];

        int elem_offset = dest_offset + (i * elem_size);

        emit_initialization(cg, elem_offset, expr, ((TypeArray *)(lhs_type))->elem_type, expr->type);
    }
}

void emit_initialization(CodeGenerator *cg, int dest_offset, AstExpr *expr, Type *lhs_type, Type *rhs_type) {
    if (expr->head.kind == AST_STRUCT_LITERAL) {
        emit_struct_initialization(cg, dest_offset, (AstStructLiteral *)(expr));
    }
    else if (expr->head.kind == AST_ARRAY_LITERAL) {
        emit_array_initialization(cg, dest_offset, (AstArrayLiteral *)(expr), lhs_type);
    }
    else {
        emit_expression(cg, expr);
        emit_simple_initialization(cg, dest_offset, false, lhs_type, rhs_type);
    }
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

    int base_array_offset = -1;
    bool is_array_initialization = false;

    int type_size = decl->type->size;
    if (decl->type->kind == TYPE_ARRAY) {
        // We are still storing the full type and size information about the array
        // on the declaration on the stack, but also reserve 16 bytes for the data + count that we have to 
        // be aware of whenever we are dealing with the array.
        type_size = 16;
        if (decl->expr && decl->expr->head.kind == AST_ARRAY_LITERAL) {
            is_array_initialization = true;
        } else {
            // Could just be an assignment to a variable of type array. In that case, we DON'T want to allocate
            // the size of the array
        }
    }
    cg->func->base_ptr -= type_size;
    cg->func->base_ptr  = align_value(cg->func->base_ptr, type_size);

    decl->ident->stack_offset = cg->func->base_ptr;
    int identifier_offset     = cg->func->base_ptr;

    if (is_array_initialization) {
        cg->func->base_ptr -= decl->type->size;
        cg->func->base_ptr  = align_value(cg->func->base_ptr, 8);

        base_array_offset = cg->func->base_ptr;
        ((AstArrayLiteral *)decl->expr)->base_offset = base_array_offset; // @Incomplete - Need a function to set the base array offset on all child arrays of the array literal!
    }

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; initialization of '%s'\n", decl->ident->name);
    
    zero_initialize(cg, identifier_offset, decl->type, false);
    if (is_array_initialization) {
        zero_initialize(cg, base_array_offset, decl->type, true);
    }

    if (decl->expr) {
        if (is_array_initialization) {
            // @Cleanup - This is a little mehh. Too implicit that we are actually doing two different things. Maybe split up into different functions for array initialization???
            emit_initialization(cg, base_array_offset, decl->expr, decl->type, decl->expr->type);        // to initialize array
            emit_expression(cg, decl->expr);
            emit_simple_initialization(cg, identifier_offset, false, decl->type, decl->expr->type); // to initialize data + count
        } else {
            emit_initialization(cg, identifier_offset, decl->expr, decl->type, decl->expr->type);
        }
    } 
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

    const char *set_instruction = comparison_operator_to_set_instruction(bin->operator);
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

const char *comparison_operator_to_set_instruction(TokenType op) {
    if (op == '<')                 return "setb";
    if (op == '>')                 return "seta";
    if (op == TOKEN_GREATER_EQUAL) return "setae";
    if (op == TOKEN_LESS_EQUAL)    return "setbe";
    if (op == TOKEN_DOUBLE_EQUAL)  return "sete";
    if (op == TOKEN_NOT_EQUAL)     return "setne";

    printf("%s:%d: compiler-error: There were unhandled cases in 'comparison_operator_to_instruction'\n", __FILE__, __LINE__);
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

// Result of offset is in rax
void emit_array_access_offset(CodeGenerator *cg, AstArrayAccess *array_ac) {
    AstArrayAccess *current = array_ac;
    while (true) {

        emit_expression(cg, current->subscript);
        POP(RAX);
        sb_append(&cg->code, "   imul\t\trax, %d\n", current->head.type->size);
        PUSH(RAX);

        if (current->accessing->head.kind == AST_ARRAY_ACCESS) {
            current = (AstArrayAccess *)(current->accessing);
        } else {
            break;
        }
    }

    current = array_ac;
    while (true) {
        POP(RAX);

        if (current->accessing->head.kind == AST_ARRAY_ACCESS) {
            POP(RBX);
            sb_append(&cg->code, "   add\t\trax, rbx\n");
            PUSH(RAX);

            current = (AstArrayAccess *)(current->accessing);
        } else {
            break;
        }
    }
}

void emit_unary_inside_member_access(CodeGenerator *cg, AstUnary *unary, AstMemberAccess *ma) {
    if (unary->operator == OP_POINTER_DEREFERENCE) {
        assert(unary->expr->type->kind == TYPE_POINTER);

        if (unary->expr->head.kind == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
            AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(unary->expr))->as.value.identifier.name, (Ast *)unary->expr);
            assert(ident->type->kind == TYPE_POINTER);

            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);
            sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
        }
        else if (unary->expr->head.kind == AST_MEMBER_ACCESS) {
            MemberAccessResult result = emit_member_access(cg, (AstMemberAccess *)(unary->expr));
            if (result.is_runtime_computed) {
                sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
            } else {
                sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", result.base_offset);
            }

            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);
        } 
        else if (unary->expr->head.kind == AST_UNARY) {
            emit_unary_inside_member_access(cg, (AstUnary *)(unary->expr), ma);
            sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
        }
        else {
            XXX;
        }
    } else if (unary->operator == OP_ADDRESS_OF) {
        XXX;
    } else {
        XXX;
    }
}

MemberAccessResult emit_member_access(CodeGenerator *cg, AstMemberAccess *ma) {
    if (ma->left->head.kind == AST_MEMBER_ACCESS) {
        AstMemberAccess *left = (AstMemberAccess *)(ma->left);

        MemberAccessResult result = emit_member_access(cg, left);

        Type *left_type = left->head.type;

        if (result.is_runtime_computed) {
            if (left_type->kind == TYPE_POINTER) {
                sb_append(&cg->code, "   mov\t\trbx, [rbx]   ; Pointer dereference 1\n");
            }
            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);
            return (MemberAccessResult){0, true};
        } else {
            if (left_type->kind == TYPE_POINTER) {
                sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", result.base_offset);
                sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);
                return (MemberAccessResult){0, true};
            }

            return (MemberAccessResult){
                .base_offset = result.base_offset + ma->struct_member->member_offset,
                .is_runtime_computed = false
            };
        }
    }
    if (ma->left->head.kind == AST_LITERAL && ((AstLiteral *)(ma->left))->kind == LITERAL_IDENTIFIER) {
        AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(ma->left))->as.value.identifier.name, (Ast *)(ma->left));

        if (ident->type->kind == TYPE_POINTER) {
            // Field access through pointer
            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);
            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);

            return (MemberAccessResult){0, true};
        } else {
            return (MemberAccessResult){
                .base_offset = ident->stack_offset + ma->struct_member->member_offset,
                .is_runtime_computed = false
            };
        }
    }
    if (ma->left->head.kind == AST_ARRAY_ACCESS) {
        AstArrayAccess *array_ac = (AstArrayAccess *)(ma->left);

        AstExpr *cursor = array_ac->accessing;
        while(cursor->head.kind == AST_ARRAY_ACCESS) {
            cursor = ((AstArrayAccess *)(cursor))->accessing;
        }

        if (cursor->head.kind == AST_LITERAL) {
            assert(((AstLiteral *)(cursor))->kind == LITERAL_IDENTIFIER);
            AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(cursor))->as.value.identifier.name, (Ast *)cursor);
            assert(ident && ident->type->kind == TYPE_ARRAY);

            emit_array_access_offset(cg, array_ac);


            sb_append(&cg->code, "\n   ; Left is basic identifier\n");
            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);         // load address to beginning of array
            sb_append(&cg->code, "   add\t\trbx, rax\n");                                  // offset into array
            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset); // offset into member
            sb_append(&cg->code, "\n");

            return (MemberAccessResult){0, true};
        }
        else if (cursor->head.kind == AST_MEMBER_ACCESS) {
            AstMemberAccess *left = (AstMemberAccess *)(cursor);
            MemberAccessResult result = emit_member_access(cg, left);
            assert(result.is_runtime_computed);

            emit_array_access_offset(cg, array_ac);

            sb_append(&cg->code, "\n   ; Left is member access\n");
            sb_append(&cg->code, "   add\t\trbx, rax\n");                                    // offset into array
            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);   // offset into member
            sb_append(&cg->code, "\n");

            return (MemberAccessResult){0, true};
        }
        else {
            XXX;
        }
    }
    else if (ma->left->head.kind == AST_UNARY) {
        emit_unary_inside_member_access(cg, (AstUnary *)(ma->left), ma);
        return (MemberAccessResult){0, true};
    }
    else {
        XXX;
    }

}

// If lvalue is set, the final offset will be in rbx, otherwise the rvalue needs to be popped/moved from stack
void emit_array_access(CodeGenerator *cg, AstArrayAccess *array_ac, bool lvalue) {
    emit_array_access_offset(cg, array_ac);

    Type *type = NULL;
    int base_offset = 0;

    AstExpr *expr = array_ac->accessing;
    while (expr->head.kind == AST_ARRAY_ACCESS) {
        expr = ((AstArrayAccess *)(expr))->accessing;
    }
    if (expr->head.kind == AST_LITERAL && ((AstLiteral *)(expr))->kind == LITERAL_IDENTIFIER) {
        AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(expr))->as.value.identifier.name, (Ast *)expr);

        type        = ident->type;
        base_offset = ident->stack_offset;
    } 
    else {
        XXX;
    }
  
    assert(type->kind == TYPE_ARRAY);
    Type *elem_type = ((TypeArray *)(type))->elem_type;
    while (elem_type->kind == TYPE_ARRAY) {
        elem_type = ((TypeArray *)(elem_type))->elem_type;
    }
    Type *innermost_type = elem_type;

    sb_append(&cg->code, "   mov\t\trbx, QWORD %d[rbp]\n", base_offset); // load pointer to data
    sb_append(&cg->code, "   add\t\trbx, rax\n"); // add offset

    if (lvalue) return;
    
    emit_move_and_push(cg, 0, true, innermost_type, lvalue);
}

// Src is [rbx], Dest is size of type
void emit_move_and_push(CodeGenerator *cg, int src_offset, bool src_is_runtime_computed, Type *src_type, bool lvalue) {
    char src[16];
    if (src_is_runtime_computed) {
        sprintf(src, "[rbx]");
    } else {
        sprintf(src, "%d[rbp]", src_offset);
    }

    switch (src_type->kind) {
    case TYPE_BOOL:
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
        if (lvalue) {
            PUSH(RAX);
        } else {
            sb_append(&cg->code, "   mov\t\trax, %s\n", src);
            PUSH(RAX);
        }
        return;
    }
    case TYPE_ARRAY: {
        sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", src_offset);
        sb_append(&cg->code, "   mov\t\trbx, QWORD %d[rbp]\n", src_offset + 8);
        PUSH(RBX);
        PUSH(RAX);
        return;
    }
    case TYPE_STRUCT: {
        sb_append(&cg->code, "   lea\t\trax, %s\n", src);
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
            sb_append(&cg->code, "   neg\t\trax\n");
            PUSH(RAX);
            return;
        }
        if (unary->operator == OP_ADDRESS_OF) {
            if (unary->expr->head.kind == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
                AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, ((AstLiteral *)(unary->expr))->as.value.identifier.name, (Ast *)unary->expr);
                assert(ident);

                sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", ident->stack_offset);
                PUSH(RAX);
                return;
            }
            else if (unary->expr->head.kind == AST_ARRAY_ACCESS) {
                emit_array_access(cg, (AstArrayAccess *)(unary->expr), true);
                PUSH(RBX);
                return;
            }
            else if (unary->expr->head.kind == AST_MEMBER_ACCESS) {
                MemberAccessResult result = emit_member_access(cg, (AstMemberAccess *)unary->expr);
                if (result.is_runtime_computed) {
                    PUSH(RBX);
                } else {
                    sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", result.base_offset);
                    PUSH(RBX);
                }
                return;
            } else {
                XXX;
            }
        }
        if (unary->operator == OP_POINTER_DEREFERENCE) {
            emit_expression(cg, unary->expr);

            Type *dereferenced_type = unary->head.type;
            POP(RBX);

            emit_move_and_push(cg, 0, true, dereferenced_type, false);

            return;
        }

        XXX;
    }
    case AST_CAST: {
        AstCast *cast = (AstCast *)(expr);
        emit_cast(cg, cast);
        return;
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(expr);
        emit_function_call(cg, call);
        PUSH(RAX);
        return;
    }
    case AST_ARRAY_ACCESS: {
        AstArrayAccess *array_ac  = (AstArrayAccess *)(expr);
        emit_array_access(cg, array_ac, false);
        return;
    }
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma  = (AstMemberAccess *)(expr);
        MemberAccessResult result = emit_member_access(cg, ma);

        if (ma->access_kind == MEMBER_ACCESS_STRUCT) {
            AstDeclaration *member = ma->struct_member;
            emit_move_and_push(cg, result.base_offset, result.is_runtime_computed, member->type, false);
        } else if (ma->access_kind == MEMBER_ACCESS_ENUM) {
            sb_append(&cg->code, "   push\t\t%d\n", ma->enum_member->value);
            INCR_PUSH_COUNT();
            return;
        }

        return;
    }
    case AST_ENUM_LITERAL : {
        AstEnumLiteral *elit = (AstEnumLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", elit->enum_member->value);
        INCR_PUSH_COUNT();
        return;
    }
    case AST_ARRAY_LITERAL : {
        AstArrayLiteral *array_lit = (AstArrayLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", array_lit->expressions.count);
        INCR_PUSH_COUNT();
        sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", array_lit->base_offset);
        PUSH(RAX);
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
            AstIdentifier *ident = lookup_from_scope(cg->parser, cg->current_scope, lit->as.value.identifier.name, (Ast *)lit);
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
            } else {
                emit_move_and_push(cg, ident->stack_offset, false, ident->type, false);
                return;
            }
        }}
        
        XXX;
    }
    case AST_STRUCT_LITERAL:
        // Handled in either the assignment or declaration
        break;
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