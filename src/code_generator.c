#include "typer.c"

typedef struct CodeGenerator {
    StringBuilder head;     // Declaring bit target and other misc stuff
    StringBuilder data;     // Corresponding to section .data
    StringBuilder code;     // Corresponding to section .text

    Parser *parser;

    SymbolTable ident_table;
    SymbolTable function_table;
    SymbolTable type_table;

    size_t base_ptr;

    size_t constants;   // For float and string literals
    size_t labels;      // For coditional jumps
    
} CodeGenerator;

void go_nuts(CodeGenerator *cg, AstCode *code);
void emit_builtin_functions(CodeGenerator *cg);
void emit_header(CodeGenerator *cg);
void emit_footer(CodeGenerator *cg);
void emit_code(CodeGenerator *cg, AstCode *code);
void emit_statement(CodeGenerator *cg, AstNode *node);
void emit_function_defn(CodeGenerator *cg, AstFunctionDefn *func_defn);
void emit_function_call(CodeGenerator *cg, AstFunctionCall *call);
void emit_return(CodeGenerator *cg, AstReturn *ast_return);
void emit_print(CodeGenerator *cg, AstPrint *print_stmt);
void emit_assert(CodeGenerator *cg, AstAssert *assertion);
void emit_if(CodeGenerator *cg, AstIf *ast_if);
void emit_for(CodeGenerator *cg, AstFor *ast_for);
void emit_enum(CodeGenerator *cg, AstEnum *ast_enum);
void emit_struct(CodeGenerator *cg, AstStruct *ast_struct);
void emit_declaration(CodeGenerator *cg, AstDeclaration *decl);
void emit_assignment(CodeGenerator *cg, AstAssignment *assign);
void emit_expression(CodeGenerator *cg, AstExpr *expr);
void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_type, TypeKind r_type);
void check_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *comparison_operator_to_set_instruction(TokenType op);
const char *boolean_operator_to_instruction(TokenType op);
int member_access_address(CodeGenerator *cg, AstMemberAccess *ma);


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
    sb_append(&cg->data, "   string_assert_fail  DB \"Assertion failed at line %s\", 10, 0\n", "%d");
    
    sb_append(&cg->code, "\n");

    // @Note - Make a check that function main exists.

    sb_append(&cg->code, "segment .text\n");
    sb_append(&cg->code, "   global main\n");
    sb_append(&cg->code, "   extern printf\n");
    sb_append(&cg->code, "   extern ExitProcess\n");
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
        case AST_ASSIGNMENT:    emit_assignment(cg, (AstAssignment *)(node)); break;
        case AST_FUNCTION_DEFN: emit_function_defn(cg, (AstFunctionDefn *)(node)); break;
        case AST_FUNCTION_CALL: emit_function_call(cg, (AstFunctionCall *)(node)); break;
        case AST_RETURN:        emit_return(cg, (AstReturn *)(node)); break;
        case AST_BLOCK:         emit_block(cg, (AstBlock *)(node), true); break;
        case AST_PRINT:         emit_print(cg, (AstPrint *)(node)); break;
        case AST_ASSERT:        emit_assert(cg, (AstAssert *)(node)); break;
        case AST_IF:            emit_if(cg, (AstIf *)(node)); break;
        case AST_FOR:           emit_for(cg, (AstFor *)(node)); break;
        case AST_ENUM:          emit_enum(cg, (AstEnum *)(node)); break;
        case AST_STRUCT:        break;
        default:
            printf("compiler-error: emit_statement not implemented for %s\n", ast_type_name(node->type));
            XXX;
    }
}

void emit_enum(CodeGenerator *cg, AstEnum *ast_enum) {
    DynamicArray enumerators = hash_table_get_items(&ast_enum->enumerators);
    for (unsigned int i = 0; i < enumerators.count; i++) {
        AstEnumerator *etor = ((AstEnumerator **)(enumerators.items))[i];
        sb_append(&cg->data, "   __%s.%s DB \"%s.%s\", 10, 0\n", ast_enum->identifier->name, etor->name, ast_enum->identifier->name, etor->name);
    }
}

void emit_assignment(CodeGenerator *cg, AstAssignment *assign) {
    emit_expression(cg, assign->expr);

    int address = 0;
    if (assign->lhs->head.type == AST_LITERAL) {
        AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(assign->lhs))->as.identifier.name)->as.identifier;
        address = ident->stack_offset;
    }
    else if (assign->lhs->head.type == AST_MEMBER_ACCESS) {
        AstMemberAccess *ma = (AstMemberAccess *)(assign->lhs);
        address = member_access_address(cg, ma);
    } else {
        XXX;
    }

    switch (assign->op) {
    case ASSIGN_EQUAL: {
        switch (assign->expr->evaluated_type.kind) {
        case TYPE_INTEGER: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], eax\n", address);
            break;
        }
        case TYPE_FLOAT: {
            sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
            break;
        }
        case TYPE_STRING: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", address);
            break;
        }
        case TYPE_BOOL: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], al\n", address);
            break;
        }
        default: XXX;
    }
    } break;
    case ASSIGN_PLUS_EQUAL: {
        if (assign->expr->evaluated_type.kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   add\t\t%d[rbp], eax\n", address);
        } else {
            sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\txmm1, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   addss\t\txmm0, xmm1\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        }
    } break;
    case ASSIGN_MINUS_EQUAL: {
        if (assign->expr->evaluated_type.kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   sub\t\t%d[rbp], eax\n", address);
        } else {
            sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\txmm1, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   subss\t\txmm1, xmm0\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        }
    } break;
    case ASSIGN_TIMES_EQUAL: {
        if (assign->expr->evaluated_type.kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\tebx, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   imul\t\teax, ebx\n");
            sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], eax\n", address);

        } else {
            sb_append(&cg->code, "   movss\t\txmm1, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\txmm0, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   mulss\t\txmm0, xmm1\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        }
    } break;
    case ASSIGN_DIVIDE_EQUAL: {
        if (assign->expr->evaluated_type.kind == TYPE_INTEGER) {
            sb_append(&cg->code, "   pop\t\trbx\n");
            sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   cqo\n");
            sb_append(&cg->code, "   idiv\t\tebx\n");
            sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], eax\n", address);
        } else {
            sb_append(&cg->code, "   movss\t\txmm1, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\txmm0, DWORD %d[rbp]\n", address);
            sb_append(&cg->code, "   divss\t\txmm0, xmm1\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        }
    } break;
}
}

void emit_for(CodeGenerator *cg, AstFor *ast_for) {
    if (ast_for->iterable->head.type == AST_RANGE_EXPR) {
        AstRangeExpr * range = (AstRangeExpr *)(ast_for->iterable);

        // allocate space for start and end range values 
        int offset_start = -(cg->base_ptr + size_of_type(cg->parser, type(TYPE_INTEGER)));
        int offset_end   = -(cg->base_ptr + size_of_type(cg->parser, type(TYPE_INTEGER)) * 2);
        int offset_iterator = -(cg->base_ptr + size_of_type(cg->parser, type(TYPE_INTEGER)) * 3);
        cg->base_ptr += size_of_type(cg->parser, type(TYPE_INTEGER)) * 3;

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

        int for_label = make_label_number(cg);
        int done_label = make_label_number(cg);
        sb_append(&cg->code, "L%d:\n", for_label);
        sb_append(&cg->code, "   mov\t\teax, %d[rbp]\n", offset_end);
        sb_append(&cg->code, "   cmp\t\t%d[rbp], eax\n", offset_iterator);
        sb_append(&cg->code, "   %s\t\tL%d\n", range->inclusive ? "jg" : "jge", done_label);

        emit_block(cg, ast_for->body, true);

        sb_append(&cg->code, "   inc\t\tDWORD %d[rbp]\n", offset_iterator);
        sb_append(&cg->code, "   jmp\t\tL%d\n", for_label);

        sb_append(&cg->code, "L%d:\n", done_label);
    } else {
        XXX;
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

    size_t bytes_allocated = cg->ident_table.current_scope->bytes_allocated;
    if (strcmp("main", func_defn->identifier->name) == 0) {
        bytes_allocated += 32; // shadow space that is for some reason needed for main??? @Investigate
    }
    size_t aligned_allocated = align_value((int)(bytes_allocated), 16);
    
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
    if (func_defn->return_type.kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
    } else if (func_defn->return_type.kind == TYPE_VOID) {
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

void emit_assert(CodeGenerator *cg, AstAssert *assertion) {
    emit_expression(cg, assertion->expr);

    sb_append(&cg->code, "   pop\t\trcx\n");
    sb_append(&cg->code, "   mov\t\trdx, %d\n", assertion->head.start.line + 1);
    sb_append(&cg->code, "   call\t\tassert\n");
}

void emit_print(CodeGenerator *cg, AstPrint *print_stmt) {
    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; expression of print\n");
    
    emit_expression(cg, print_stmt->expr);

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; call to print\n");

    TypeInfo expr_type = print_stmt->expr->evaluated_type;
    if (expr_type.kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_int\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type.kind == TYPE_FLOAT) {
        sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
        sb_append(&cg->code, "   add\t\trsp, 4\n");
        sb_append(&cg->code, "   cvtss2sd\txmm0, xmm0\n");
        sb_append(&cg->code, "   movapd\txmm1, xmm0\n");
        sb_append(&cg->code, "   movq\t\trdx, xmm0\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_float\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type.kind == TYPE_BOOL) {
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
    else if (expr_type.kind == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_string\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else if (expr_type.kind == TYPE_ENUM) {
        sb_append(&cg->code, "   pop\t\trdx\n");
        sb_append(&cg->code, "   mov\t\trcx, fmt_int\n");
        sb_append(&cg->code, "   call\t\tprintf\n");
    }
    else {
        // Unhandled cases
        XXX;
    }
}

void zero_initialize(CodeGenerator *cg, AstDeclaration *decl, int stack_offset) {
    switch (decl->declared_type.kind) {
        case TYPE_INTEGER: sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_FLOAT:   sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_BOOL:    sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], 0\n",  stack_offset); break;
        case TYPE_STRING:  sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_ENUM:    sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_STRUCT: {
            AstStruct *struct_defn = get_struct(&cg->parser->type_table, decl->declared_type.as.identifier);
            assert(struct_defn != NULL);

            DynamicArray members = get_struct_members(struct_defn);
            for (unsigned int i = 0; i < members.count; i++) {
                AstDeclaration *member = ((AstDeclaration **)(members.items))[i];
                zero_initialize(cg, member, stack_offset + member->member_offset);
            }
            break;
        }
        default:
            printf("%s:%d: compiler-error: There were unhandled cases in 'zero_initialize'\n", __FILE__, __LINE__);
            exit(1);
    }
}

void assign_simple_value(CodeGenerator *cg, int address, TypeInfo lhs_type, TypeInfo rhs_type) {
    if (lhs_type.kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], eax\n", address);
    }
    else if (lhs_type.kind == TYPE_FLOAT) {
        if (rhs_type.kind == TYPE_INTEGER) {
            // Int to float conversion
            sb_append(&cg->code, "   pop\trax\n");
            sb_append(&cg->code, "   cvtsi2ss\txmm0, rax\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        } else {
            sb_append(&cg->code, "   movss\t\txmm0, [rsp]\n");
            sb_append(&cg->code, "   add\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\tDWORD %d[rbp], xmm0\n", address);
        }
    }
    else if (lhs_type.kind == TYPE_BOOL) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], al\n", address);
    }
    else if (lhs_type.kind == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", address);
    }
    else if (lhs_type.kind == TYPE_ENUM) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], eax\n", address);
    }
    else {
        XXX;
    }
}

void emit_struct_initialization(CodeGenerator *cg, AstStructLiteral *lit, int start_offset) {
    for (unsigned int i = 0; i < lit->initializers.count; i++) {
        AstStructInitializer *init = ((AstStructInitializer **)(lit->initializers.items))[i];

        emit_expression(cg, init->value);

        // Move value into respective members' address
        int member_address  = start_offset + init->member->member_offset;
        TypeInfo value_type = init->value->evaluated_type;

        if (init->value->head.type == AST_STRUCT_LITERAL) {
            emit_struct_initialization(cg, (AstStructLiteral *)(init->value), member_address);
        } else {
            assign_simple_value(cg, member_address, init->member->declared_type, value_type);
        }
    }
}

void emit_declaration(CodeGenerator *cg, AstDeclaration *decl) {
    int type_size = size_of_type(cg->parser, decl->declared_type);
    cg->base_ptr += type_size;

    cg->base_ptr = align_value(cg->base_ptr, type_size);
    decl->identifier->stack_offset = -cg->base_ptr;
    int stack_offset = decl->identifier->stack_offset;

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; initialization of '%s'\n", decl->identifier->name);
    zero_initialize(cg, decl, stack_offset);
    
    if (decl->declaration_type != DECLARATION_TYPED_NO_EXPR) {
        if (decl->expr->head.type == AST_STRUCT_LITERAL) {
            // Struct assignment
            emit_struct_initialization(cg, (AstStructLiteral *)(decl->expr), stack_offset);
        } else {
            // Simple assignment
            emit_expression(cg, decl->expr);

            sb_append(&cg->code, "\n\n");
            sb_append(&cg->code, "   ; putting result into '%s'\n", decl->identifier->name);
            assign_simple_value(cg, stack_offset, decl->declared_type, decl->expr->evaluated_type);
        }
    }
}

void emit_operator_divide(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '/');

    TypeKind l_type = bin->left->evaluated_type.kind;
    TypeKind r_type = bin->right->evaluated_type.kind;

    if (l_type == TYPE_INTEGER && r_type == TYPE_INTEGER) {
        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);
        sb_append(&cg->code, "   pop\t\trbx\n");    // divisor
        sb_append(&cg->code, "   pop\t\trax\n");    // dividend
        sb_append(&cg->code, "   cqo\n");           // sign extend rax through rax:rdx needed for division for some reason???
        sb_append(&cg->code, "   idiv\t\trbx\n");
        sb_append(&cg->code, "   push\t\trax\n");   // quotient is in rax
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

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_divide', while doing %s / %s\n", __FILE__, __LINE__, type_to_str(type(l_type)), type_to_str(type(r_type)));
    exit(1);
}

void emit_operator_times(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '*');

    TypeKind l_type = bin->left->evaluated_type.kind;
    TypeKind r_type = bin->right->evaluated_type.kind;

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

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_times', while doing %s * %s\n", __FILE__, __LINE__, type_to_str(type(l_type)), type_to_str(type(r_type)));
    exit(1);
}

void emit_operator_minus(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '-');

    TypeKind l_type = bin->left->evaluated_type.kind;
    TypeKind r_type = bin->right->evaluated_type.kind;

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

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_minus', while doing %s - %s\n", __FILE__, __LINE__, type_to_str(type(l_type)), type_to_str(type(r_type)));
    exit(1);
}

void emit_operator_plus(CodeGenerator *cg, AstBinary *bin) {
    assert(bin->operator == '+');

    TypeKind l_type = bin->left->evaluated_type.kind;
    TypeKind r_type = bin->right->evaluated_type.kind;

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

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_plus', while doing %s + %s\n", __FILE__, __LINE__, type_to_str(type(l_type)), type_to_str(type(r_type)));
    exit(1);
}

void emit_comparison_operator(CodeGenerator *cg, AstBinary *bin) {
    assert(is_comparison_operator(bin->operator));

    TypeKind l_type = bin->left->evaluated_type.kind;
    TypeKind r_type = bin->right->evaluated_type.kind;

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

    printf("%s:%d: compiler-error: There were unhandled cases in 'emit_comparison_operator', while doing %s '%s' %s\n", __FILE__, __LINE__, type_to_str(type(l_type)), token_type_to_str(bin->operator), type_to_str(type(r_type)));
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
    if (op == TOKEN_LOGICAL_OR)  return "or ";
    if (op == '!')               return "not";

    printf("%s:%d: compiler-error: There were unhandled cases in 'boolean_operator_to_instruction'\n", __FILE__, __LINE__);
    exit(1);
}


int make_label_number(CodeGenerator *cg) {
    return cg->labels++;
}

int member_access_address(CodeGenerator *cg, AstMemberAccess *ma) {
    AstIdentifier *ident = symbol_lookup(&cg->ident_table, ma->ident->name)->as.identifier;
    int stack_offset     = ident->stack_offset;
    for (unsigned int i = 0; i < ma->chain.count; i++) {
        AstAccessor *ac = ((AstAccessor **)(ma->chain.items))[i];
        stack_offset += ac->struct_member->member_offset;
    }

    return stack_offset;
}

void emit_expression(CodeGenerator *cg, AstExpr *expr) {
    switch (expr->head.type) {
    case AST_BINARY: {
        AstBinary *bin = (AstBinary *)(expr);
        TokenType op = bin->operator;
        if (op == '+') {emit_operator_plus(cg, bin); return;}
        if (op == '-') {emit_operator_minus(cg, bin); return;}
        if (op == '*') {emit_operator_times(cg, bin); return;}
        if (op == '/') {emit_operator_divide(cg, bin); return;}
        if (is_comparison_operator(op)) {emit_comparison_operator(cg, bin); return;}
        if (is_boolean_operator(op)) {emit_boolean_operator(cg, bin); return;}

        XXX;
    }
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *)(expr);
        if (unary->operator == OP_NOT) {
            emit_expression(cg, unary->expr);
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   not\t\trax\n");
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

        XXX;
    }
    case AST_FUNCTION_CALL: {
        AstFunctionCall *call = (AstFunctionCall *)(expr);
        emit_function_call(cg, call);
        sb_append(&cg->code, "   push\t\trax\n");
        return;
    }
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma  = (AstMemberAccess *)(expr);
        int stack_offset = member_access_address(cg, ma);

        AstAccessor *ac = ((AstAccessor **)(ma->chain.items))[ma->chain.count - 1];
        if (ac->kind == Accessor_Struct) {
            AstDeclaration *member = ac->struct_member;
            if (member->declared_type.kind == TYPE_INTEGER) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type.kind == TYPE_FLOAT) {
                sb_append(&cg->code, "   movss\t\txmm0, %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   sub\t\trsp, 4\n");
                sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                return;
            }
            if (member->declared_type.kind == TYPE_BOOL) {
                sb_append(&cg->code, "   mov\t\tal, BYTE %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type.kind == TYPE_STRING) {
                sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type.kind == TYPE_ENUM) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            XXX;
        } else if (ac->kind == Accessor_Enum) {
            sb_append(&cg->code, "   push\t\t%d\n", ac->enum_member->value);
            return;
        } else {
            XXX; // Should not happen
        }

        return;
    }
    case AST_ENUM_LITERAL : {
        AstEnumLiteral *elit = (AstEnumLiteral *)(expr);
        sb_append(&cg->code, "   push\t\t%d\n", elit->enum_member->value);
        return;
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)(expr);
        switch (lit->kind) {
        case LITERAL_INTEGER: {
            sb_append(&cg->code, "   push\t\t%d\n", lit->as.integer);
            return;
        }
        case LITERAL_FLOAT: {
            sb_append(&cg->data, "   CF%d DD %lf\n", cg->constants, lit->as.floating);
            sb_append(&cg->code, "   movss\t\txmm0, [CF%d]\n", cg->constants);
            sb_append(&cg->code, "   sub\t\trsp, 4\n");
            sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
            cg->constants++;
            return;
        }
        case LITERAL_STRING: {
            sb_append(&cg->data, "   CS%d DB \"%s\", 0 \n", cg->constants, lit->as.string.data);
            sb_append(&cg->code, "   mov\t\trax, CS%d\n", cg->constants);
            sb_append(&cg->code, "   push\t\trax\n", cg->constants);
            cg->constants++;
            return;
        }
        case LITERAL_BOOLEAN: {
            bool value = lit->as.boolean;
            if (value == true) sb_append(&cg->code, "   push\t\t-1\n");
            else               sb_append(&cg->code, "   push\t\t0\n"); 
            return;
        }
        case LITERAL_IDENTIFIER: {
            Symbol *ident_symbol = symbol_lookup(&cg->ident_table, lit->as.identifier.name);
            assert(ident_symbol != NULL);

            AstIdentifier *ident = ident_symbol->as.identifier;
            assert(ident != NULL);

            // @Improvement - Could probably be @Refactored. Looks very similar to 'assign_simple_value'
            if (ident->type.kind == TYPE_INTEGER) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n", ident->stack_offset);
                return;
            }
            if (ident->type.kind == TYPE_FLOAT) {
                sb_append(&cg->code, "   movss\t\txmm0, %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   sub\t\trsp, 4\n");
                sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                return;
            }
            if (ident->type.kind == TYPE_BOOL) {
                sb_append(&cg->code, "   mov\t\tal, BYTE %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (ident->type.kind == TYPE_STRING) {
                sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (ident->type.kind == TYPE_ENUM) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }

            XXX;

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