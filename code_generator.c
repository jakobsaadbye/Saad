// @Cleanup - When looking up identifiers, right now we do [rbp - offset] for new allocations and for lookup. This leads to wrong
//            results when inside a function as the offset is positive. Instead we should do something like this 'offset[rbp]'. That way it
//            it will work for both positive and negative offsets!

#include "typer.c"

typedef struct CodeGenerator {
    StringBuilder head; // Declaring bit target and other misc stuff
    StringBuilder data; // Corresponding to section .data
    StringBuilder code; // Corresponding to section .text

    Parser *parser;

    SymbolTable ident_table;
    SymbolTable function_table;

    size_t base_ptr;
    int stack_space;

    size_t constants;   // For float and string literals
    size_t labels;      // For coditional jumps
    bool inside_print_expression;
    
} CodeGenerator;


void go_nuts(CodeGenerator *cg, AstCode *code);
void emit_header(CodeGenerator *cg);
void emit_footer(CodeGenerator *cg);
void emit_code(CodeGenerator *cg, AstCode *code);
void emit_statement(CodeGenerator *cg, AstNode *node);
void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn);
void emit_function_call(CodeGenerator *cg, AstFunctionCall *call);
void emit_return(CodeGenerator *cg, AstReturn *ast_return);
void emit_print(CodeGenerator *cg, AstPrint *print_stmt);
void emit_if(CodeGenerator *cg, AstIf *ast_if);
void emit_declaration(CodeGenerator *cg, AstDeclaration *decl);
void emit_expression(CodeGenerator *cg, AstExpr *expr);
void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_type, TypeKind r_type);
void check_that_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *comparison_operator_to_set_instruction(TokenType op);
const char *boolean_operator_to_instruction(TokenType op);
char *func_parameter_to_register(TypeKind param_type, int index);


CodeGenerator code_generator_init(Parser *parser) {
    CodeGenerator cg = {0};
    cg.head        = string_builder_init(1024);
    cg.data        = string_builder_init(1024);
    cg.code        = string_builder_init(1024);

    cg.parser         = parser;
    cg.ident_table    = parser->ident_table;
    cg.function_table = parser->function_table;
    cg.base_ptr       = 0;
    cg.constants      = 0;
    cg.labels         = 0;
    cg.inside_print_expression = false;

    // Reset all the next pointers inside the scopes, used during typing.
    symbol_table_reset(&cg.ident_table);

    return cg;
}

void go_nuts(CodeGenerator *cg, AstCode *code) {
    check_that_main_exists(cg);
    emit_header(cg);
    emit_code(cg, code);
}

void check_that_main_exists(CodeGenerator *cg) {
    Symbol *main_symbol = symbol_lookup(&cg->function_table, "main");
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

int align_value(int value, int alignment) {
    int rem = value % alignment;
    if (rem == 0) return value;
    else          return value + (alignment - rem);
    
}

void emit_header(CodeGenerator *cg) {
    sb_append(&cg->head, "bits 64\n");
    sb_append(&cg->head, "default rel\n");
    sb_append(&cg->head, "\n");

    sb_append(&cg->data, "segment .data\n");
    sb_append(&cg->data, "   fmt_int   DB \"%s\", 10, 0\n", "%d");
    sb_append(&cg->data, "   fmt_float DB \"%s\", 10, 0\n", "%f");
    sb_append(&cg->data, "   fmt_string DB \"%s\", 10, 0\n", "%s");
    sb_append(&cg->data, "   string_false DB \"false\", 10, 0\n");
    sb_append(&cg->data, "   string_true  DB \"true\", 10, 0\n");
    sb_append(&cg->code, "\n");

    // @Note - Make a check that function main exists.

    sb_append(&cg->code, "segment .text\n");
    sb_append(&cg->code, "   global main\n");
    sb_append(&cg->code, "   extern printf\n");
    sb_append(&cg->code, "\n");
}

void emit_code(CodeGenerator *cg, AstCode *code) {
    for (unsigned int i = 0; i < code->statements.count; i++) {
        AstNode *stmt = ((AstNode **)(code->statements.items))[i];
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

void emit_statement(CodeGenerator *cg, AstNode *node) {
    switch (node->type) {
        case AST_DECLARATION:   emit_declaration(cg, (AstDeclaration *)(node)); break;
        case AST_FUNCTION_DEFN: emit_function_defn(cg, (AstFunctionDefn *)(node)); break;
        case AST_FUNCTION_CALL: emit_function_call(cg, (AstFunctionCall *)(node)); break;
        case AST_RETURN:        emit_return(cg, (AstReturn *)(node)); break;
        case AST_BLOCK:         emit_block(cg, (AstBlock *)(node), true); break;
        case AST_PRINT:         emit_print(cg, (AstPrint *)(node)); break;
        case AST_IF:            emit_if(cg, (AstIf *)(node)); break;
        default:
            printf("compiler-error: emit_statement not implemented for %s\n", ast_type_name(node->type));
            XXX;
    }
}

void emit_return(CodeGenerator *cg, AstReturn *ast_return) {
    emit_expression(cg, ast_return->expr);

    if (ast_return->expr->evaluated_type == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   pop\t\trbp\n");    // @Hack - Find a way to avoid multiple returns statements!!!
        sb_append(&cg->code, "   ret\n");
    }
    if (ast_return->expr->evaluated_type == TYPE_FLOAT) {
        XXX;
    }
}

void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "%s:\n", func_defn->identifier->name);
    sb_append(&cg->code, "   push\t\trbp\n");
    sb_append(&cg->code, "   mov\t\trbp, rsp\n");

    // Enter the scope of the function body to know how much stack space we should allocate for the function
    enter_scope(&cg->ident_table);

    size_t bytes_allocated = cg->ident_table.current_scope->bytes_allocated;
    size_t shadow_space = 32;
    size_t aligned_allocated = align_value((int)(bytes_allocated), 16);
    
    sb_append(&cg->code, "   sub\t\trsp, %d\n", aligned_allocated + shadow_space);

    for (unsigned int i = 0; i < func_defn->parameters.count; i++) {
        AstDeclaration *param = ((AstDeclaration **)(func_defn->parameters.items))[i];
        if (param->declared_type == TYPE_INTEGER) {
            const char *reg  = func_parameter_to_register(param->declared_type, i);
            int stack_offset = 16 + (i * 8);
            sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], %s\n", stack_offset, reg);

            param->identifier->stack_offset = stack_offset;
        } else {
            XXX;
        }
    }

    emit_block(cg, func_defn->body, false);
    exit_scope(&cg->ident_table);

    sb_append(&cg->code, "   add\t\trsp, %d\n", aligned_allocated + shadow_space);

    bool returns_void = func_defn->return_type == TYPE_VOID;
    if (returns_void) {
        sb_append(&cg->code, "   pop\t\trbp\n");
        sb_append(&cg->code, "   ret\n");
    }
}

static char temp_register[6];
char *func_parameter_to_register(TypeKind param_type, int index) {
    if (param_type == TYPE_INTEGER) {
        if (index == 0) strcpy(temp_register, "ecx");
        if (index == 1) strcpy(temp_register, "edx");
        if (index >= 2 && index <= 9) sprintf(temp_register, "r%dd", index + 6);
        if (index > 9) {
            // @Todo: Anything above r15d should be popped off the stack
            XXX;
        }

    }
    if (param_type == TYPE_FLOAT) {
        if (index <= 15) sprintf(temp_register, "xmm%d", index);
        else {
            // @Todo: Pop xmm registers off the stack
            XXX;
        }
    }

    return temp_register;
}

void emit_function_call(CodeGenerator *cg, AstFunctionCall *call) {
    for (int i = (int)(call->arguments.count) - 1; i >= 0; i--) {
        AstExpr *arg = ((AstExpr **)(call->arguments.items))[i];

        emit_expression(cg, arg);

        const char *reg = func_parameter_to_register(arg->evaluated_type, i);
        if (arg->evaluated_type == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%s, eax\n", reg);
        } else {
            XXX;
        }
    }

    sb_append(&cg->code, "   call\t\t%s\n", call->identifer->name);
}

void emit_if(CodeGenerator *cg, AstIf *ast_if) {
    emit_expression(cg, ast_if->condition);
    int done_label = make_label_number(cg);
    int first_else_if_label = -1;
    int else_label = -1;
    if (ast_if->else_ifs.count > 0)  first_else_if_label = make_label_number(cg);
    if (ast_if->has_else_block) else_label               = make_label_number(cg);

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
            if (ast_if->has_else_block) {
                sb_append(&cg->code, "   jz\t\t\tL%d\n", else_label);
            } else {
                sb_append(&cg->code, "   jz\t\t\tL%d\n", done_label);
            }
        }

        emit_block(cg, else_if->block, true);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    if (ast_if->has_else_block) {
        sb_append(&cg->code, "; else\n");
        sb_append(&cg->code, "L%d:\n", else_label);
        emit_block(cg, ast_if->else_block, true);
        sb_append(&cg->code, "   jmp L%d\n", done_label);
    }

    sb_append(&cg->code, "; done\n");
    sb_append(&cg->code, "L%d:\n", done_label);
}

void emit_print(CodeGenerator *cg, AstPrint *print_stmt) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; expression of print\n");
    
    cg->inside_print_expression = true;
    emit_expression(cg, print_stmt->expr);
    cg->inside_print_expression = false;

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; call to print\n");

    TypeKind expr_type = print_stmt->expr->evaluated_type;
    if (expr_type == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_int\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    if (expr_type == TYPE_FLOAT) {
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
        sb_append(&cg->code, "   movapd\txmm1, xmm0\n");
        sb_append(&cg->code, "   movq\t\trdx, xmm0\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_float\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    if (expr_type == TYPE_BOOL) {
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
    if (expr_type == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_string\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
}

void emit_declaration(CodeGenerator *cg, AstDeclaration *decl) {
    // Increment base pointer
    int type_size = size_of_type(decl->declared_type);
    cg->base_ptr += type_size;

    if (decl->declared_type == TYPE_INTEGER || decl->declared_type == TYPE_FLOAT || decl->declared_type == TYPE_STRING) {
        cg->base_ptr = align_value(cg->base_ptr, type_size);
    }

    decl->identifier->stack_offset = -cg->base_ptr;
    int stack_offset = decl->identifier->stack_offset;

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; initialization of '%s'\n", decl->identifier->name);

    if (decl->declared_type == TYPE_INTEGER) {
        sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset);
    }
    if (decl->declared_type == TYPE_FLOAT) {
        sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset);
    }
    if (decl->declared_type == TYPE_BOOL) {
        sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], 0\n", stack_offset);
    }
    if (decl->declared_type == TYPE_STRING) {
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", stack_offset);
    }
    
    emit_expression(cg, decl->expr);

    sb_append(&cg->code, "\n\n");
    sb_append(&cg->code, "   ; putting result into '%s'\n", decl->identifier->name);
    if (decl->declared_type == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], eax\n", stack_offset);
    }
    if (decl->declared_type == TYPE_FLOAT) {
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", stack_offset);
    }
    if (decl->declared_type == TYPE_BOOL) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], al\n", stack_offset);
    }
    if (decl->declared_type == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", stack_offset);
    }

}

void emit_operator_divide(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '/');

    TypeKind l_type = bin->left->evaluated_type;
    TypeKind r_type = bin->right->evaluated_type;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "   pop\t\trbx\n"); // dividend
        sb_append(&cg->code, "   pop\t\trax\n"); // divisor
        sb_append(&cg->code, "   cqo\n"); // sign extend rax through rax:rdx needed for division for some reason???
        sb_append(&cg->code, "   idiv\t\trbx\n");
        sb_append(&cg->code, "   push\t\trax\n"); // quotient is in rax
        return;
    }

    if ((l_type == TYPE_FLOAT   && r_type == TYPE_FLOAT) ||   
        (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) ||
        (l_type == TYPE_FLOAT   && r_type == TYPE_INTEGER)) {

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        emit_integer_to_float_conversion(cg, l_type, r_type);
        sb_append(&cg->code, "   divss\t\txmm0, xmm1\n");   
        sb_append(&cg->code, "   sub\t\trsp, 4\n");         
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_divide', while doing %s / %s\n", __FILE__, __LINE__, type_kind_to_str(l_type), type_kind_to_str(r_type));
    exit(1);
}

void emit_operator_times(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '*');

    TypeKind l_type = bin->left->evaluated_type;
    TypeKind r_type = bin->right->evaluated_type;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   imul\t\trax, rbx\n");
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }

    if ((l_type == TYPE_FLOAT   && r_type == TYPE_FLOAT) ||   
        (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) ||
        (l_type == TYPE_FLOAT   && r_type == TYPE_INTEGER)) {

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        emit_integer_to_float_conversion(cg, l_type, r_type);
        sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n");   
        sb_append(&cg->code, "   sub\t\trsp, 4\n");         
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_times', while doing %s * %s\n", __FILE__, __LINE__, type_kind_to_str(l_type), type_kind_to_str(r_type));
    exit(1);
}

void emit_operator_minus(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '-');

    TypeKind l_type = bin->left->evaluated_type;
    TypeKind r_type = bin->right->evaluated_type;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   sub\t\trax, rbx\n");
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }

    if ((l_type == TYPE_FLOAT   && r_type == TYPE_FLOAT) ||   
        (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) ||
        (l_type == TYPE_FLOAT   && r_type == TYPE_INTEGER)) {

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        emit_integer_to_float_conversion(cg, l_type, r_type);
        sb_append(&cg->code, "   subss\t\txmm0, xmm1\n");   
        sb_append(&cg->code, "   sub\t\trsp, 4\n");         
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_minus', while doing %s - %s\n", __FILE__, __LINE__, type_kind_to_str(l_type), type_kind_to_str(r_type));
    exit(1);
}

void emit_operator_plus(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '+');

    TypeKind l_type = bin->left->evaluated_type;
    TypeKind r_type = bin->right->evaluated_type;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   add\t\trax, rbx\n");
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }

    if ((l_type == TYPE_FLOAT   && r_type == TYPE_FLOAT) ||   
        (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) ||
        (l_type == TYPE_FLOAT   && r_type == TYPE_INTEGER)) { 

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "\n");
        emit_integer_to_float_conversion(cg, l_type, r_type);
        sb_append(&cg->code, "   addss\t\txmm0, xmm1\n");   
        sb_append(&cg->code, "   sub\t\trsp, 4\n");
        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");

        return; 
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_plus', while doing %s + %s\n", __FILE__, __LINE__, type_kind_to_str(l_type), type_kind_to_str(r_type));
    exit(1);
}

void emit_comparison_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_comparison_operator(bin->operator));

    TypeKind l_type = bin->left->evaluated_type;
    TypeKind r_type = bin->right->evaluated_type;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);

        const char *set_instruction = comparison_operator_to_set_instruction(bin->operator);
        sb_append(&cg->code, "\n");
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   cmp\t\trax, rbx\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        sb_append(&cg->code, "   push\t\trax\n");

        return;
    }

    if ((l_type == TYPE_FLOAT   && r_type == TYPE_FLOAT) ||   
        (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) ||
        (l_type == TYPE_FLOAT   && r_type == TYPE_INTEGER)) { 

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);

        emit_integer_to_float_conversion(cg, l_type, r_type);

        const char *set_instruction = comparison_operator_to_set_instruction(bin->operator);
        sb_append(&cg->code, "   comiss\t\txmm0, xmm1\n");
        sb_append(&cg->code, "   %s\t\tal\n", set_instruction);
        sb_append(&cg->code, "   push\t\trax\n");

        return;
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_comparison_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_kind_to_str(l_type), token_type_to_str(bin->operator), type_kind_to_str(r_type));
    exit(1);
}

void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_type, TypeKind r_type) {
    if (l_type == TYPE_FLOAT && r_type == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trbx\n");
        sb_append(&cg->code, "   cvtsi2ss\txmm1, rbx\n");
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
    }
    else if (l_type == TYPE_INTEGER && r_type == TYPE_FLOAT) {
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
    if (op == TOKEN_LOGICAL_OR)  return "or";
    if (op == '!')               return "not";

    printf("%s:%d: compiler-error: There were unhandled cases in 'boolean_operator_to_instruction'\n", __FILE__, __LINE__);
    exit(1);
}


int make_label_number(CodeGenerator *cg) {
    int label_number = cg->labels;
    cg->labels++;
    return label_number;
}

void emit_expression(CodeGenerator *cg, AstExpr *expr) {
    if (expr->head.type == AST_BINARY) {
        AstBinary *bin = (AstBinary *)(expr);
        if (bin->operator == '+') {
            emit_operator_plus(cg, bin);
            return;
        }
        if (bin->operator == '-') {
            emit_operator_minus(cg, bin);
            return;
        }
        if (bin->operator == '*') {
            emit_operator_times(cg, bin);
            return;
        }
        if (bin->operator == '/') {
            emit_operator_divide(cg, bin);
            return;
        }
        if (is_comparison_operator(bin->operator)) {
            emit_comparison_operator(cg, bin);
            return;
        }
        if (is_boolean_operator(bin->operator)) {
            emit_boolean_operator(cg, bin);
            return;
        }
        XXX;
    }
    if (expr->head.type == AST_UNARY) {
        AstUnary *unary = (AstUnary *)(expr);
        if (unary->operator == '!') {
            emit_expression(cg, unary->expr);

            sb_append(&cg->code, "\n");
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   not\t\trax\n");
            sb_append(&cg->code, "   push\t\trax\n");

            return;
        }

        printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression'\n", __FILE__, __LINE__);
        exit(1);
    }
    if (expr->head.type == AST_LITERAL) {
        AstLiteral *lit = (AstLiteral *)(expr);
        if (lit->type == TOKEN_INTEGER) {
            sb_append(&cg->code, "   push\t\t%d\n", lit->as_value.integer);
            return;
        }
        if (lit->type == TOKEN_FLOAT) {
            sb_append(&cg->data, "   CF%d DD %lf\n", cg->constants, lit->as_value.floating);
            sb_append(&cg->code, "   movss\t\txmm0, [CF%d]\n", cg->constants);
            sb_append(&cg->code, "   sub\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
            cg->constants++;
            return;
        }
        if (lit->type == TOKEN_BOOLEAN) {
            bool value = lit->as_value.boolean;
            if (value == true) sb_append(&cg->code, "   push\t\t1\n");
            else               sb_append(&cg->code, "   push\t\t0\n"); 
            return;
        }
        if (lit->type == TOKEN_STRING) {
            sb_append(&cg->data, "   CS%d DB \"%s\", 0 \n", cg->constants, lit->as_value.string.data);
            sb_append(&cg->code, "   mov\t\trax, CS%d\n", cg->constants);
            sb_append(&cg->code, "   push\t\trax\n", cg->constants);

            cg->constants++;
            return;
        }
        if (lit->type == TOKEN_IDENTIFIER) {
            Symbol *ident_symbol = symbol_lookup(&cg->ident_table, lit->as_value.identifier.name);
            assert(ident_symbol != NULL);

            AstIdentifier *ident = ident_symbol->as_value.identifier;
            assert(ident != NULL);
            if (ident->type == TYPE_FLOAT) {
                sb_append(&cg->code, "   movss\t\txmm0, %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   sub\t\trsp, 4\n");
                sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                return;
            }
            if (ident->type == TYPE_INTEGER) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n", ident->stack_offset);
                return;
            }
            if (ident->type == TYPE_STRING) {
                sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (ident->type == TYPE_BOOL) {
                sb_append(&cg->code, "   mov\t\tal, BYTE %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }

            printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression'\n", __FILE__, __LINE__);
            exit(1);
        }
        printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression'\n", __FILE__, __LINE__);
        exit(1);
    }

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_expression'\n", __FILE__, __LINE__);
    exit(1);
}