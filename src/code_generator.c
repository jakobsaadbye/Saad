#include "typer.c"

typedef struct CodeGenerator {
    StringBuilder head;     // Declaring bit target and other misc stuff
    StringBuilder data;     // Corresponding to section .data
    StringBuilder code;     // Corresponding to section .text

    Parser *parser;

    SymbolTable ident_table;
    SymbolTable function_table;
    TypeTable   type_table;

    int base_ptr;

    size_t constants;   // For float and string literals
    size_t labels;      // For coditional jumps
    
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

void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_kind, TypeKind r_kind);
void emit_initialization(CodeGenerator *cg, int dest_offset, AstExpr *expr, Type *lhs_type, Type *rhs_type);
void emit_simple_initialization(CodeGenerator *cg, int dest_offset, bool dest_is_runtime_computed, Type *lhs_type, Type *rhs_type);
void emit_move_and_push(CodeGenerator *cg, int src_offset, bool src_is_runtime_computed, Type *src_type, bool lvalue);

void check_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *comparison_operator_to_set_instruction(TokenType op);
const char *boolean_operator_to_instruction(TokenType op);
MemberAccessResult emit_member_access(CodeGenerator *cg, AstMemberAccess *ma);
char *WIDTH(Type *type);

const char *REG_A(Type *type);
const char *REG_B(Type *type);
const char *REG_C(Type *type);
const char *REG_D(Type *type);


CodeGenerator code_generator_init(Parser *parser) {
    CodeGenerator cg = {0};
    cg.head = string_builder_init(1024);
    cg.data = string_builder_init(1024);
    cg.code = string_builder_init(1024);

    cg.parser         = parser;
    cg.ident_table    = parser->ident_table;
    cg.function_table = parser->function_table;
    cg.type_table     = parser->type_table;
    cg.base_ptr       = 0;
    cg.constants      = 0;
    cg.labels         = 0;

    // Reset all the next pointers inside the scopes, used during typing.
    symbol_table_reset(&cg.ident_table);

    return cg;
}

void go_nuts(CodeGenerator *cg, AstCode *code) {
    check_main_exists(cg);
    emit_header(cg);
    emit_code(cg, code);
}

void check_main_exists(CodeGenerator *cg) {
    Symbol *main_symbol = symbol_lookup(&cg->function_table, "main", NULL);
    if (main_symbol == NULL) {
        printf("Missing function 'main' as entry-point to the program\n");
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
    fwrite(cg->code.buffer, 1, cg->code.cursor, f);

    fclose(f);
}

void emit_header(CodeGenerator *cg) {
    sb_append(&cg->head, "bits 64\n");
    sb_append(&cg->head, "default rel\n");
    sb_append(&cg->head, "\n");

    sb_append(&cg->data, "segment .data\n");
    sb_append(&cg->data, "   fmt_int   DB \"%s\", 10, 0\n", "%lld");
    sb_append(&cg->data, "   fmt_float DB \"%s\", 10, 0\n", "%lf");
    sb_append(&cg->data, "   fmt_string DB \"%s\", 10, 0\n", "%s");
    sb_append(&cg->data, "   fmt_address DB \"0x%s\", 10, 0\n", "%p");
    sb_append(&cg->data, "   string_false DB \"false\", 10, 0\n");
    sb_append(&cg->data, "   string_true  DB \"true\", 10, 0\n");
    sb_append(&cg->data, "   string_assert_fail  DB \"Assertion failed at line %s\", 10, 0\n", "%d");
    
    sb_append(&cg->code, "\n");

    // @Note - Make a check that function main exists.

    sb_append(&cg->code, "segment .text\n");
    sb_append(&cg->code, "   global main\n");
    sb_append(&cg->code, "   extern printf\n");
    sb_append(&cg->code, "   extern ExitProcess\n");
    sb_append(&cg->code, "   extern malloc\n");
    sb_append(&cg->code, "\n");

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

    // Print enums
    sb_append(&cg->code, "enum_str:\n");
    sb_append(&cg->code, "   mov\t\trcx, fmt_string\n");
    sb_append(&cg->code, "   jmp\t\tenum_end\n");
    sb_append(&cg->code, "enum_int:\n");
    sb_append(&cg->code, "   mov\t\trcx, fmt_int\n");
    sb_append(&cg->code, "   mov\t\trdx, rax\n");
    sb_append(&cg->code, "enum_end:\n");
    sb_append(&cg->code, "   pop\t\trbx\n"); // @Hack @Investigate - for some reason the next printf call messes up the stack, so here we save the return address to rbx before the call to push it again after the printf call
    sb_append(&cg->code, "   call\t\tprintf\n");
    sb_append(&cg->code, "   push\t\trbx\n");
    sb_append(&cg->code, "   ret\n\n");
}

void emit_code(CodeGenerator *cg, AstCode *code) {
    for (unsigned int i = 0; i < code->statements.count; i++) {
        Ast *stmt = ((Ast **)(code->statements.items))[i];
        emit_statement(cg, stmt);
    }
}

void emit_block(CodeGenerator *cg, AstBlock *block, bool open_lexical_scope) {
    if (open_lexical_scope) enter_scope(&cg->ident_table);
    for (int i = 0; i < block->num_of_statements; i++) {
        emit_statement(cg, block->statements[i]);
    }
    if (open_lexical_scope) exit_scope(&cg->ident_table);
}

void emit_statement(CodeGenerator *cg, Ast *node) {
    switch (node->type) {
        case AST_DECLARATION:       emit_declaration(cg, (AstDeclaration *)(node)); break;
        case AST_ASSIGNMENT:        emit_assignment(cg, (AstAssignment *)(node)); break;
        case AST_FUNCTION_DEFN:     emit_function_defn(cg, (AstFunctionDefn *)(node)); break;
        case AST_FUNCTION_CALL:     emit_function_call(cg, (AstFunctionCall *)(node)); break;
        case AST_RETURN:            emit_return(cg, (AstReturn *)(node)); break;
        case AST_BREAK_OR_CONTINUE: emit_break_or_continue(cg, (AstBreakOrContinue *)(node)); break;
        case AST_BLOCK:             emit_block(cg, (AstBlock *)(node), true); break;
        case AST_PRINT:             emit_print(cg, (AstPrint *)(node)); break;
        case AST_ASSERT:            emit_assert(cg, (AstAssert *)(node)); break;
        case AST_TYPEOF:            emit_typeof(cg, (AstTypeof *)(node)); break;
        case AST_IF:                emit_if(cg, (AstIf *)(node)); break;
        case AST_FOR:               emit_for(cg, (AstFor *)(node)); break;
        case AST_ENUM:              emit_enum(cg, (AstEnum *)(node)); break;
        case AST_STRUCT:        break;
        default:
            printf("compiler-error: emit_statement not implemented for %s\n", ast_type_name(node->type));
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
    for (unsigned int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->data, "   __%s.%s DB \"%s.%s\", 0\n", ast_enum->identifier->name, etor->name, ast_enum->identifier->name, etor->name);
    }

    int case_label = make_label_number(cg);
    int fallthrough_label = make_label_number(cg);

    sb_append(&cg->code, "print_enum_%s:\n", ast_enum->identifier->name);
    for (unsigned int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->code, "   mov\t\trbx, %d\n", etor->value);
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   jz\t\t\tenum_case_%d\n", case_label);

        etor->label = case_label;
        case_label  = make_label_number(cg);
    }
    // Fallthrough case. Use integer value
    sb_append(&cg->code, "   jmp\t\tenum_int\n", fallthrough_label);

    // Success case. Use name of enum
    for (unsigned int i = 0; i < ast_enum->enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(ast_enum->enumerators.items))[i];
        sb_append(&cg->code, "enum_case_%d:\n", etor->label);
        sb_append(&cg->code, "   mov\t\trdx, __%s.%s\n", ast_enum->identifier->name, etor->name);
        sb_append(&cg->code, "   jmp\t\tenum_str\n");
    }
}

void emit_assignment(CodeGenerator *cg, AstAssignment *assign) {
    emit_expression(cg, assign->expr);

    bool offset_is_runtime_computed = false;
    int base_offset = 0;
    if (assign->lhs->head.type == AST_LITERAL) {
        AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(assign->lhs))->as.value.identifier.name, (Ast *)assign->lhs)->as.identifier;

        if (ident->type->kind == TYPE_POINTER) {
            if (assign->expr->evaluated_type->kind != TYPE_POINTER) {
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
    else if (assign->lhs->head.type == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)(assign->lhs);
        MemberAccessResult result = emit_member_access(cg, ma);
        base_offset = result.base_offset;
        offset_is_runtime_computed = result.is_runtime_computed;
    }
    else if (assign->lhs->head.type == AST_ARRAY_ACCESS) {
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
        emit_simple_initialization(cg, base_offset, offset_is_runtime_computed, assign->lhs->evaluated_type, assign->expr->evaluated_type);
        return;
    }

    if (assign->lhs->evaluated_type->kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
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
    else if (assign->lhs->evaluated_type->kind == TYPE_FLOAT) {

        if (assign->expr->evaluated_type->kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trbx\n");
            sb_append(&cg->code, "   cvtsi2ss\txmm1, rbx\n");
        } else {
            sb_append(&cg->code, "   movss\t\txmm1, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
        }
        
        sb_append(&cg->code, "   movss\t\txmm0, DWORD %s\n", address_str);

        switch (assign->op) {
        case ASSIGN_PLUS_EQUAL:   sb_append(&cg->code, "   addss\t\txmm0, xmm1\n"); break;
        case ASSIGN_MINUS_EQUAL:  sb_append(&cg->code, "   subss\t\txmm0, xmm1\n"); break;
        case ASSIGN_TIMES_EQUAL:  sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n"); break;
        case ASSIGN_DIVIDE_EQUAL: sb_append(&cg->code, "   divss\t\txmm0, xmm1\n"); break;
        default: exit(1); // Unreachable
        }

        sb_append(&cg->code, "   movss\t\tDWORD %s, xmm0\n", address_str);
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
        emit_block(cg, ast_for->body, true);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);
        sb_append(&cg->code, "L%d:\n", done_label);
    }
    else if (ast_for->iterable->head.type == AST_RANGE_EXPR) {
        AstRangeExpr * range = (AstRangeExpr *)(ast_for->iterable);
        
        // allocate space for start and end range values 
        int size_start    = range->start->evaluated_type->size;
        int size_end      = range->end->evaluated_type->size;
        int size_iterator = ast_for->iterator->type->size;

        int offset_start    = (cg->base_ptr - (size_start));
        int offset_end      = (cg->base_ptr - (size_start + size_end));
        int offset_iterator = (cg->base_ptr - (size_start + size_end + size_iterator));
        cg->base_ptr -= (size_start + size_end + size_iterator);

        emit_expression(cg, range->start);
        emit_expression(cg, range->end);

        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\t%d[rbp], eax\n", offset_start);
        sb_append(&cg->code, "   mov\t\t%d[rbp], ebx\n", offset_end);

        // initialize iterator
        ast_for->iterator->stack_offset = offset_iterator;
        sb_append(&cg->code, "   mov\t\teax, %d[rbp]\n", offset_start);
        sb_append(&cg->code, "   mov\t\t%d[rbp], eax\n", offset_iterator);

        sb_append(&cg->code, "L%d:\n", cond_label);
        sb_append(&cg->code, "   mov\t\teax, %d[rbp]\n", offset_end);
        sb_append(&cg->code, "   cmp\t\t%d[rbp], eax\n", offset_iterator);
        sb_append(&cg->code, "   %s\t\tL%d\n", range->inclusive ? "jg" : "jge", done_label);

        emit_block(cg, ast_for->body, true);
        
        
        sb_append(&cg->code, "L%d:\n", post_expression_label);
        sb_append(&cg->code, "   inc\t\tDWORD %d[rbp]\n", offset_iterator);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    } 
    else {
        AstIdentifier *iterator = ast_for->iterator;
        AstExpr *iterable       = ast_for->iterable;

        // Allocate space for iterator, pointer to head of array, stop condition (count) and index

        int aligned_iterator_size = align_value(iterator->type->size, 8);
        int offset_iterator       = cg->base_ptr - aligned_iterator_size;
        int offset_data           = offset_iterator - 8;
        int offset_count          = offset_iterator - 16;
        int offset_index          = offset_iterator - 24;
        cg->base_ptr             -= 24 + aligned_iterator_size;

        iterator->stack_offset = offset_iterator;

        emit_expression(cg, iterable);

        sb_append(&cg->code, "   ; For-loop\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   pop\t\trbx\n");
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

        emit_block(cg, ast_for->body, true);
        
        sb_append(&cg->code, "L%d:\n", post_expression_label);
        sb_append(&cg->code, "   inc\t\tQWORD %d[rbp]\n", offset_index);
        sb_append(&cg->code, "   jmp\t\tL%d\n", cond_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    }
}

void emit_return(CodeGenerator *cg, AstReturn *ast_return) {
    emit_expression(cg, ast_return->expr);
    sb_append(&cg->code, "   jmp\t\tL%d\n", ast_return->enclosing_function->return_label);
}

void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "%s:\n", func_defn->identifier->name);
    sb_append(&cg->code, "   push\t\trbp\n");
    sb_append(&cg->code, "   mov\t\trbp, rsp\n");

    // Enter the scope of the function body to know how much stack space we should allocate for the function
    enter_scope(&cg->ident_table);

    size_t bytes_allocated = func_defn->bytes_allocated;;
    if (strcmp("main", func_defn->identifier->name) == 0) {
        bytes_allocated += 32; // shadow space that is for some reason needed for main??? @Investigate
    }
    size_t aligned_allocated = align_value((int)(bytes_allocated), 32); // :WrongForLoopSizing @Temporary @Hack Would like it if this was only 16 byte aligned instead of 32, but it seems like there is some problem with sizing for-loops correctly that causes not enough space to be allocated
    
    if (bytes_allocated) sb_append(&cg->code, "   sub\t\trsp, %d\n", aligned_allocated);

    for (unsigned int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        param->identifier->stack_offset = 16 + (i * 8);
    }

    // Make a label for the return code so that any return statements within the function can jump to this. e.g for conditionals
    int return_label = make_label_number(cg);
    func_defn->return_label = return_label;

    emit_block(cg, func_defn->body, false);
    exit_scope(&cg->ident_table);
    

    sb_append(&cg->code, "L%d:\n", return_label);
    if (func_defn->return_type->kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
    } else if (func_defn->return_type->kind == TYPE_VOID) {
        sb_append(&cg->code, "   mov\t\trax, 0\n");
        // Do nothing
    } else {
        XXX;
    }
    if (bytes_allocated) sb_append(&cg->code, "   add\t\trsp, %d\n", aligned_allocated);
    sb_append(&cg->code, "   pop\t\trbp\n");
    sb_append(&cg->code, "   ret\n");
}

void emit_function_call(CodeGenerator *cg, AstFunctionCall *call) {
    for (int i = (int)(call->arguments.count) - 1; i >= 0; i--) {
        AstExpr *arg = ((AstExpr **)(call->arguments.items))[i];
        emit_expression(cg, arg);
    }

    sb_append(&cg->code, "   call\t\t%s\n", call->identifer->name);
    sb_append(&cg->code, "   add\t\trsp, %d\n", call->arguments.count * 8);
}

void emit_if(CodeGenerator *cg, AstIf *ast_if) {
    emit_expression(cg, ast_if->condition);

    int done_label = make_label_number(cg);
    int first_else_if_label = -1;
    int else_label = -1;
    if (ast_if->else_ifs.count > 0)  first_else_if_label = make_label_number(cg);
    if (ast_if->else_block)          else_label          = make_label_number(cg);

    sb_append(&cg->code, "   pop\t\trax\n");
    sb_append(&cg->code, "   cmp\t\tal, 0\n");

    if (first_else_if_label != -1) {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", first_else_if_label);
    } else if (else_label != -1) {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", else_label);
    } else {
        sb_append(&cg->code, "   jz\t\t\tL%d\n", done_label);
    }

    sb_append(&cg->code, "   ; block of if\n");
    emit_block(cg, ast_if->block, true);
    sb_append(&cg->code, "   jmp L%d\n", done_label);

    int next_if_else_label = first_else_if_label;
    for (unsigned int i = 0; i < ast_if->else_ifs.count; i++) {
        AstIf *else_if = &((AstIf *)(ast_if->else_ifs.items))[i];

        sb_append(&cg->code, ";#%zu else if\n", i + 1);
        sb_append(&cg->code, "L%d:\n", next_if_else_label);

        emit_expression(cg, else_if->condition);

        sb_append(&cg->code, "   pop\t\trax\n");
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

        emit_block(cg, else_if->block, true);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    if (ast_if->else_block) {
        sb_append(&cg->code, "; else\n");
        sb_append(&cg->code, "L%d:\n", else_label);
        emit_block(cg, ast_if->else_block, true);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    sb_append(&cg->code, "; done\n");
    sb_append(&cg->code, "L%d:\n", done_label);
}

void emit_assert(CodeGenerator *cg, AstAssert *assertion) {
    emit_expression(cg, assertion->expr);

    sb_append(&cg->code, "   pop\t\trcx\n");
    sb_append(&cg->code, "   mov\t\trdx, %d\n", assertion->head.start.line);
    sb_append(&cg->code, "   call\t\tassert\n");
}

void emit_typeof(CodeGenerator *cg, AstTypeof *ast_typeof) {
    Type *expr_type = ast_typeof->expr->evaluated_type;

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

void emit_print(CodeGenerator *cg, AstPrint *print_stmt) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; expression of print\n");
    
    emit_expression(cg, print_stmt->expr);

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; call to print\n");

    Type *expr_type = print_stmt->expr->evaluated_type;
    if (expr_type->kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_int\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type->kind == TYPE_FLOAT) {
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
        sb_append(&cg->code, "   movapd\txmm1, xmm0\n");
        sb_append(&cg->code, "   movq\t\trdx, xmm0\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_float\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type->kind == TYPE_BOOL) {
        int true_label        = make_label_number(cg);
        int fallthrough_label = make_label_number(cg);
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   cmp\t\tal, 0\n");
        sb_append(&cg->code, "   jnz\t\tL%d\n", true_label);
        sb_append(&cg->code, "   mov\t\trcx, string_false\n");
        sb_append(&cg->code, "   jmp\t\tL%d\n", fallthrough_label);
        sb_append(&cg->code, "L%d:\n", true_label);
        sb_append(&cg->code, "   mov\t\trcx, string_true\n");
        sb_append(&cg->code, "L%d:\n", fallthrough_label);
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type->kind == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_string\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type->kind == TYPE_ENUM) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   call\t\tprint_enum_%s\n", ((TypeEnum *)(expr_type))->identifier->name);
    }
    else if (expr_type->kind == TYPE_POINTER) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        // sb_append(&cg->code, "   mov\t\trdx, [rdx]\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_address\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type->kind == TYPE_ARRAY) {
        XXX;
    }
    else {
        // Unhandled cases
        XXX;
    }
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

        DynamicArray members = get_struct_members(struct_defn);
        for (unsigned int i = 0; i < members.count; i++) {
            AstDeclaration *member = ((AstDeclaration **)(members.items))[i];
            zero_initialize(cg, dest_offset + member->member_offset, member->declared_type, member->declared_type->kind == TYPE_ARRAY);
        }
        free(members.items);
        break;
    }
    case TYPE_ARRAY : {
        // @Note - Maybe it should only be done when
        // the inner type is a struct, where the struct might contrain default values that we would
        // want to initialize

        TypeArray *array = (TypeArray *)(type);
        if (is_array_initialization) {
            for (unsigned int i = 0; i < array->capicity; i++) {
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
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tBYTE %s, al\n", address_str);
        return;
    }
    case TYPE_INTEGER: {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\t%s %s, %s\n", WIDTH(lhs_type), address_str, REG_A(lhs_type));
        return;
    }
    case TYPE_FLOAT: {
        if (rhs_type->kind == TYPE_INTEGER) {
            // Int to float conversion 
            // @Note - I feel like this conversion from int to float should be dealt with at the typing level instead of here. 
            // One idea is to just change it at the check_delcarartion level. If we have a float on the left and an integer on the right, just change the right hand side type to be integer
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   cvtsi2ss\txmm0, rax\n");
            sb_append(&cg->code, "   movss\t\tDWORD %s, xmm0\n", address_str);
        } else {
            sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\tDWORD %s, xmm0\n", address_str);
        }
        return;
    }
    case TYPE_STRING: {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tQWORD %s, rax\n", address_str);
        return;
    }
    case TYPE_ENUM: {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tDWORD %s, eax\n", address_str);
        return;
    }
    case TYPE_POINTER: {
        if (rhs_type->kind == TYPE_POINTER) {
            sb_append(&cg->code, "   pop\t\trax\n");
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
        sb_append(&cg->code, "   pop\t\trax\n"); // data
        sb_append(&cg->code, "   pop\t\trcx\n"); // count
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
    for (unsigned int i = 0; i < lit->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(lit->initializers.items))[i];

        int member_offset = dest_offset + init->member->member_offset;

        emit_initialization(cg, member_offset, init->value, init->member->declared_type, init->value->evaluated_type);
    }
}

void emit_array_initialization(CodeGenerator *cg, int dest_offset, AstArrayLiteral *array_lit, Type *lhs_type) {
    assert(lhs_type->kind == TYPE_ARRAY);
    
    int elem_size   = ((TypeArray *)(array_lit->head.evaluated_type))->elem_type->size;
    for (unsigned int i = 0; i < array_lit->expressions.count; i++) {
        AstExpr *expr = ((AstExpr **)(array_lit->expressions.items))[i];

        int elem_offset = dest_offset + (i * elem_size);

        emit_initialization(cg, elem_offset, expr, ((TypeArray *)(lhs_type))->elem_type, expr->evaluated_type);
    }
}

void emit_initialization(CodeGenerator *cg, int dest_offset, AstExpr *expr, Type *lhs_type, Type *rhs_type) {
    if (expr->head.type == AST_STRUCT_LITERAL) {
        emit_struct_initialization(cg, dest_offset, (AstStructLiteral *)(expr));
    }
    else if (expr->head.type == AST_ARRAY_LITERAL) {
        emit_array_initialization(cg, dest_offset, (AstArrayLiteral *)(expr), lhs_type);
    }
    else {
        emit_expression(cg, expr);
        emit_simple_initialization(cg, dest_offset, false, lhs_type, rhs_type);
    }
}

void emit_declaration(CodeGenerator *cg, AstDeclaration *decl) {
    if (decl->flags & DECLARATION_CONSTANT) {
        assert(decl->expr->head.type == AST_LITERAL);
        AstLiteral *lit = (AstLiteral *)(decl->expr);

        switch (lit->kind) {
        case LITERAL_BOOLEAN: break; // Immediate value is used
        case LITERAL_INTEGER: break; // Immediate value is used 
        case LITERAL_FLOAT:   sb_append(&cg->data, "   C_%s DD %lf\n", decl->identifier->name, lit->as.value.floating); break; // :IdentifierNameAsConstant @Cleanup
        case LITERAL_STRING:  sb_append(&cg->data, "   C_%s DB \"%s\"\n", decl->identifier->name, lit->as.value.string.data); break; // :IdentifierNameAsConstant @Cleanup
        case LITERAL_NIL:     XXX;
        case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
        }

        return;
    }

    int base_array_offset = -1;
    bool is_array_initialization = false;

    int type_size = decl->declared_type->size;
    if (decl->declared_type->kind == TYPE_ARRAY) {
        // We are still storing the full type and size information about the array
        // on the declaration on the stack, but also reserve 16 bytes for the data + count that we have to 
        // be aware of whenever we are dealing with the array.
        type_size = 16;
        if (decl->expr && decl->expr->head.type == AST_ARRAY_LITERAL) {
            is_array_initialization = true;
        } else {
            // Could just be an assignment to a variable of type array. In that case, we DON'T want to allocate
            // the size of the array
        }
    }
    cg->base_ptr -= type_size;
    cg->base_ptr  = align_value(cg->base_ptr, type_size);

    decl->identifier->stack_offset = cg->base_ptr;
    int identifier_offset          = cg->base_ptr;

    if (is_array_initialization) {
        cg->base_ptr -= decl->declared_type->size;
        cg->base_ptr  = align_value(cg->base_ptr, 8);

        base_array_offset = cg->base_ptr;
        ((AstArrayLiteral *)decl->expr)->base_offset = base_array_offset; // @Incomplete - Need a function to set the base array offset on all child arrays of the array literal!
    }

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; initialization of '%s'\n", decl->identifier->name);
    
    zero_initialize(cg, identifier_offset, decl->declared_type, false);
    if (is_array_initialization) {
        zero_initialize(cg, base_array_offset, decl->declared_type, true);
    }

    if (decl->expr) {
        if (is_array_initialization) {
            // @Cleanup - This is a little mehh. Too implicit that we are actually doing two different things. Maybe split up into different functions for array initialization???
            emit_initialization(cg, base_array_offset, decl->expr, decl->declared_type, decl->expr->evaluated_type);        // to initialize array
            emit_expression(cg, decl->expr);
            emit_simple_initialization(cg, identifier_offset, false, decl->declared_type, decl->expr->evaluated_type); // to initialize data + count
        } else {
            emit_initialization(cg, identifier_offset, decl->expr, decl->declared_type, decl->expr->evaluated_type);
        }
    } 
}

void emit_arithmetic_operator(CodeGenerator *cg, AstBinary *bin) {
    Type *l_type = bin->left->evaluated_type;
    Type *r_type = bin->right->evaluated_type;

    TypeKind l_kind = l_type->kind;
    TypeKind r_kind = r_type->kind;

    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    if (l_kind == TYPE_ENUM) l_kind = TYPE_INTEGER;
    if (r_kind == TYPE_ENUM) r_kind = TYPE_INTEGER;

    // Integer case
    if (l_kind == TYPE_INTEGER && r_kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        if      (bin->operator == '+') sb_append(&cg->code, "   add\t\trax, rbx\n");
        else if (bin->operator == '-') sb_append(&cg->code, "   sub\t\trax, rbx\n");
        else if (bin->operator == '*') sb_append(&cg->code, "   imul\t\trax, rbx\n");
        else if (bin->operator == '/') {
            sb_append(&cg->code, "   cqo\n");           // sign extend rax through rax:rdx needed for division for some reason???
            sb_append(&cg->code, "   idiv\t\trbx\n");
        } else exit(1); // Should not happen

        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }

    // Float case
    if ((l_kind == TYPE_FLOAT   && r_kind == TYPE_FLOAT) ||   
        (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) ||
        (l_kind == TYPE_FLOAT   && r_kind == TYPE_INTEGER)) {
        
        emit_integer_to_float_conversion(cg, l_kind, r_kind);
        if      (bin->operator == '+') sb_append(&cg->code, "   addss\t\txmm0, xmm1\n");
        else if (bin->operator == '-') sb_append(&cg->code, "   subss\t\txmm0, xmm1\n");
        else if (bin->operator == '*') sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n");
        else if (bin->operator == '/') sb_append(&cg->code, "   divss\t\txmm0, xmm1\n");
        else exit(1); // Should not happen

        sb_append(&cg->code, "   sub\t\trsp, 4\n");
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_arithmetic_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_to_str(l_type), token_type_to_str(bin->operator), type_to_str(r_type));
    exit(1);
}


void emit_comparison_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_comparison_operator(bin->operator));

    Type *l_type = bin->left->evaluated_type;
    Type *r_type = bin->right->evaluated_type;

    TypeKind l_kind = l_type->kind;
    TypeKind r_kind = r_type->kind;

    if (l_kind == TYPE_ENUM) l_kind = TYPE_INTEGER;
    if (r_kind == TYPE_ENUM) r_kind = TYPE_INTEGER;

    const char *set_instruction = comparison_operator_to_set_instruction(bin->operator);
    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    if (l_kind == TYPE_INTEGER && r_kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        sb_append(&cg->code, "   push\t\trax\n");

        return;
    }
    if ((l_kind == TYPE_FLOAT   && r_kind == TYPE_FLOAT) ||   
        (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) ||
        (l_kind == TYPE_FLOAT   && r_kind == TYPE_INTEGER)) 
    { 
        emit_integer_to_float_conversion(cg, l_kind, r_kind);

        sb_append(&cg->code, "   comiss\txmm0, xmm1\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        sb_append(&cg->code, "   push\t\trax\n");

        return;
    }
    if (l_kind == TYPE_POINTER && r_kind == TYPE_POINTER) {
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_comparison_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_to_str(l_type), token_type_to_str(bin->operator), type_to_str(r_type));
    exit(1);
}

void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_kind, TypeKind r_kind) {
    if (l_kind == TYPE_FLOAT && r_kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   cvtsi2ss\txmm1, rbx\n");
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
    }
    else if (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) {
        sb_append(&cg->code, "   movss\t\txmm1, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   cvtsi2ss\txmm0, rax\n");
    } else {
        sb_append(&cg->code, "   movss\t\txmm1, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
    }
}

void emit_boolean_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_boolean_operator(bin->operator));

    emit_expression(cg, bin->left);
    emit_expression(cg, bin->right);

    const char *bool_instruction = boolean_operator_to_instruction(bin->operator);
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   pop\t\trbx\n");
    sb_append(&cg->code, "   pop\t\trax\n");
    sb_append(&cg->code, "   %s\t\tal, bl\n", bool_instruction);
    sb_append(&cg->code, "   push\t\trax\n");
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
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   imul\t\trax, %d\n", current->head.evaluated_type->size);
        sb_append(&cg->code, "   push\t\trax\n");

        if (current->accessing->head.type == AST_ARRAY_ACCESS) {
            current = (AstArrayAccess *)(current->accessing);
        } else {
            break;
        }
    }

    current = array_ac;
    while (true) {
        sb_append(&cg->code, "   pop\t\trax\n");

        if (current->accessing->head.type == AST_ARRAY_ACCESS) {
            sb_append(&cg->code, "   pop\t\trbx\n");
            sb_append(&cg->code, "   add\t\trax, rbx\n");
            sb_append(&cg->code, "   push\t\trax\n");

            current = (AstArrayAccess *)(current->accessing);
        } else {
            break;
        }
    }
}

void emit_unary_inside_member_access(CodeGenerator *cg, AstUnary *unary, AstMemberAccess *ma) {
    if (unary->operator == OP_POINTER_DEREFERENCE) {
        assert(unary->expr->evaluated_type->kind == TYPE_POINTER);

        if (unary->expr->head.type == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
            AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(unary->expr))->as.value.identifier.name, (Ast *)unary->expr)->as.identifier;
            assert(ident->type->kind == TYPE_POINTER);

            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);
            sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
        }
        else if (unary->expr->head.type == AST_MEMBER_ACCESS) {
            MemberAccessResult result = emit_member_access(cg, (AstMemberAccess *)(unary->expr));
            if (result.is_runtime_computed) {
                sb_append(&cg->code, "   mov\t\trbx, [rbx]\n");
            } else {
                sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", result.base_offset);
            }

            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset);
        } 
        else if (unary->expr->head.type == AST_UNARY) {
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
    if (ma->left->head.type == AST_MEMBER_ACCESS) {
        AstMemberAccess *left = (AstMemberAccess *)(ma->left);

        MemberAccessResult result = emit_member_access(cg, left);

        Type *left_type = left->head.evaluated_type;

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
    if (ma->left->head.type == AST_LITERAL && ((AstLiteral *)(ma->left))->kind == LITERAL_IDENTIFIER) {
        AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(ma->left))->as.value.identifier.name, (Ast *)(ma->left))->as.identifier;

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
    if (ma->left->head.type == AST_ARRAY_ACCESS) {
        AstArrayAccess *array_ac = (AstArrayAccess *)(ma->left);

        AstExpr *cursor = array_ac->accessing;
        while(cursor->head.type == AST_ARRAY_ACCESS) {
            cursor = ((AstArrayAccess *)(cursor))->accessing;
        }

        if (cursor->head.type == AST_LITERAL) {
            assert(((AstLiteral *)(cursor))->kind == LITERAL_IDENTIFIER);
            AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(cursor))->as.value.identifier.name, (Ast *)cursor)->as.identifier;
            assert(ident && ident->type->kind == TYPE_ARRAY);

            emit_array_access_offset(cg, array_ac);


            sb_append(&cg->code, "\n   ; Left is basic identifier\n");
            sb_append(&cg->code, "   mov\t\trbx, %d[rbp]\n", ident->stack_offset);         // load address to beginning of array
            sb_append(&cg->code, "   add\t\trbx, rax\n");                                  // offset into array
            sb_append(&cg->code, "   add\t\trbx, %d\n", ma->struct_member->member_offset); // offset into member
            sb_append(&cg->code, "\n");

            return (MemberAccessResult){0, true};
        }
        else if (cursor->head.type == AST_MEMBER_ACCESS) {
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
    else if (ma->left->head.type == AST_UNARY) {
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
    while (expr->head.type == AST_ARRAY_ACCESS) {
        expr = ((AstArrayAccess *)(expr))->accessing;
    }
    if (expr->head.type == AST_LITERAL && ((AstLiteral *)(expr))->kind == LITERAL_IDENTIFIER) {
        AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(expr))->as.value.identifier.name, (Ast *)expr)->as.identifier;

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
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }
    case TYPE_INTEGER: {
        sb_append(&cg->code, "   mov\t\t%s, %s %s\n", REG_A(src_type), WIDTH(src_type), src);
        if (is_signed_integer(src_type) && src_type->size != 8) {
            sb_append(&cg->code, "   movsx\t\trax, %s\n", REG_A(src_type));
        }
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }
    case TYPE_FLOAT: {
        sb_append(&cg->code, "   %s\t\txmm0, %s\n", src_type->size == 4 ? "movss" : "movsd", src);
        sb_append(&cg->code, "   sub\t\trsp, %d\n", src_type->size);
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
        return;
    }
    case TYPE_POINTER: {
        if (lvalue) {
            sb_append(&cg->code, "   push\t\trbx\n");
        } else {
            sb_append(&cg->code, "   mov\t\trbx, %s\n", src);
            sb_append(&cg->code, "   push\t\trbx\n");
        }
        return;
    }
    case TYPE_ARRAY: {
        sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", src_offset);
        sb_append(&cg->code, "   mov\t\trbx, QWORD %d[rbp]\n", src_offset + 8);
        sb_append(&cg->code, "   push\t\trbx\n");
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }
    case TYPE_STRUCT:
    default:
        printf("internal-compiler-error: Unhandled case %s in emit_move_and_push()", type_to_str(src_type));
        exit(1);
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
    switch (expr->head.type) {
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
            sb_append(&cg->code, "   pop\t\trax\n");

            if (unary->expr->evaluated_type->kind == TYPE_BOOL) {
                sb_append(&cg->code, "   not\t\trax\n");
            }
            else if (unary->expr->evaluated_type->kind == TYPE_POINTER) {
                sb_append(&cg->code, "   test\t\trax, rax\n");
                sb_append(&cg->code, "   sete\t\tal\n");
                sb_append(&cg->code, "   movzx\t\trax, al\n");
            } else {
                printf("Internal Compiler Error: Unexpected unary expression type '%s' in emit_expression<AST_UNARY>", type_to_str(unary->expr->evaluated_type));
                exit(1);
            }

            sb_append(&cg->code, "   push\t\trax\n");
            return;
        }
        if (unary->operator == OP_UNARY_MINUS) {
            emit_expression(cg, unary->expr);
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   neg\t\trax\n");
            sb_append(&cg->code, "   push\t\trax\n");
            return;
        }
        if (unary->operator == OP_ADDRESS_OF) {
            if (unary->expr->head.type == AST_LITERAL && ((AstLiteral *)(unary->expr))->kind == LITERAL_IDENTIFIER) {
                AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(unary->expr))->as.value.identifier.name, (Ast *)unary->expr)->as.identifier;
                assert(ident);

                sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            else if (unary->expr->head.type == AST_ARRAY_ACCESS) {
                emit_array_access(cg, (AstArrayAccess *)(unary->expr), true);
                sb_append(&cg->code, "   push\t\trbx\n");
                return;
            }
            else if (unary->expr->head.type == AST_MEMBER_ACCESS) {
                MemberAccessResult result = emit_member_access(cg, (AstMemberAccess *)unary->expr);
                if (result.is_runtime_computed) {
                    sb_append(&cg->code, "   push\t\trbx\n");
                } else {
                    sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", result.base_offset);
                    sb_append(&cg->code, "   push\t\trbx\n");
                }
                return;
            } else {
                XXX;
            }
        }
        if (unary->operator == OP_POINTER_DEREFERENCE) {
            emit_expression(cg, unary->expr);

            Type *dereferenced_type = unary->head.evaluated_type;
            sb_append(&cg->code, "   pop\t\trbx\n");

            emit_move_and_push(cg, 0, true, dereferenced_type, false);

            return;
        }

        XXX;
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(expr);
        emit_function_call(cg, call);
        sb_append(&cg->code, "   push\t\trax\n");
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
            emit_move_and_push(cg, result.base_offset, result.is_runtime_computed, member->declared_type, false);
        } else if (ma->access_kind == MEMBER_ACCESS_ENUM) {
            sb_append(&cg->code, "   push\t\t%d\n", ma->enum_member->value);
            return;
        }

        return;
    }
    case AST_ENUM_LITERAL : {
        AstEnumLiteral *elit = (AstEnumLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", elit->enum_member->value);
        return;
    }
    case AST_ARRAY_LITERAL : {
        AstArrayLiteral *array_lit = (AstArrayLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", array_lit->expressions.count);
        sb_append(&cg->code, "   lea\t\trax, %d[rbp]\n", array_lit->base_offset);
        sb_append(&cg->code, "   push\t\trax\n");
        break;
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)(expr);
        switch (lit->kind) {
        case LITERAL_INTEGER: {
            sb_append(&cg->code, "   mov\t\trax, %llu\n", lit->as.value.integer); // @Incomplete - Need to check if signed or unsigned!!!
            sb_append(&cg->code, "   push\t\trax\n");
            return;
        }
        case LITERAL_FLOAT: {
            sb_append(&cg->data, "   CF%d DD %lf\n", cg->constants, lit->as.value.floating);
            sb_append(&cg->code, "   movss\t\txmm0, [CF%d]\n", cg->constants);
            sb_append(&cg->code, "   sub\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
            cg->constants++;
            return;
        }
        case LITERAL_STRING: {
            sb_append(&cg->data, "   CS%d DB \"%s\", 0 \n", cg->constants, lit->as.value.string.data);
            sb_append(&cg->code, "   mov\t\trax, CS%d\n", cg->constants);
            sb_append(&cg->code, "   push\t\trax\n", cg->constants);
            cg->constants++;
            return;
        }
        case LITERAL_BOOLEAN: {
            sb_append(&cg->code, "   push\t\t%d\n", lit->as.value.boolean ? -1 : 0);
            return;
        }
        case LITERAL_NIL: {
            sb_append(&cg->code, "   push\t\t0\n");
            return;
        }
        case LITERAL_IDENTIFIER: {
            Symbol *ident_symbol = symbol_lookup(&cg->ident_table, lit->as.value.identifier.name, (Ast *)lit);
            assert(ident_symbol != NULL);

            AstIdentifier *ident = ident_symbol->as.identifier;

            if (ident->belongs_to_decl && ident->belongs_to_decl->flags & DECLARATION_CONSTANT) {
                assert(ident->belongs_to_decl->expr->head.type == AST_LITERAL);
                AstLiteral *lit = (AstLiteral *)(ident->belongs_to_decl->expr);
                switch (lit->kind) {
                    case LITERAL_BOOLEAN: {
                        sb_append(&cg->code, "   push\t\t%d\n", lit->as.value.boolean ? -1 : 0);
                        return;
                    }
                    case LITERAL_INTEGER: {
                        sb_append(&cg->code, "   push\t\t%lld\n", lit->as.value.integer);
                        return;
                    }
                    case LITERAL_FLOAT: {
                        // @Incomplete - We need to check if float is 32 or 64 bit constant. Right now we just assume 32 bit
                        sb_append(&cg->code, "   movss\t\txmm0, [C_%s]\n", ident->name); // :IdentifierNameAsConstant @Cleanup - Should the identifier name really be used as the constant name??? Not good if we have the same identifier name in two seperate blocks inside same function!
                        sb_append(&cg->code, "   sub\t\trsp, 4\n"); 
                        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                        return;
                    }
                    case LITERAL_STRING: {
                        sb_append(&cg->code, "   mov\t\trax, C_%s\n", ident->name); // :IdentifierNameAsConstant @Cleanup
                        sb_append(&cg->code, "   push\t\trax");
                        return;
                    }
                    case LITERAL_NIL: XXX;
                    case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
                }
            }

            emit_move_and_push(cg, ident->stack_offset, false, ident->type, false);
            return;
        }}
        
        XXX;
    }
    case AST_STRUCT_LITERAL:
        // Handled in either the assignment or declaration
        break;
    default:
        printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression' with %s\n", __FILE__, __LINE__, ast_type_name(expr->head.type));
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