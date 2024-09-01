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
void emit_enum(CodeGenerator *cg, AstEnum *ast_enum);
void emit_struct(CodeGenerator *cg, AstStruct *ast_struct);
void emit_declaration(CodeGenerator *cg, AstDeclaration *decl);
void emit_assignment(CodeGenerator *cg, AstAssignment *assign);
void emit_expression(CodeGenerator *cg, AstExpr *expr);
void emit_integer_to_float_conversion(CodeGenerator *cg, TypeKind l_kind, TypeKind r_kind);

void check_main_exists(CodeGenerator *cg);
int make_label_number(CodeGenerator *cg);
const char *comparison_operator_to_set_instruction(TokenType op);
const char *boolean_operator_to_instruction(TokenType op);
int member_access_address(CodeGenerator *cg, AstMemberAccess *ma);
char *DT(Type *type);


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
    sb_append(&cg->data, "   fmt_int   DB \"%s\", 10, 0\n", "%lld");
    sb_append(&cg->data, "   fmt_float DB \"%s\", 10, 0\n", "%lf");
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
        case AST_DECLARATION:   emit_declaration(cg, (AstDeclaration *)(node)); break;
        case AST_ASSIGNMENT:    emit_assignment(cg, (AstAssignment *)(node)); break;
        case AST_FUNCTION_DEFN: emit_function_defn(cg, (AstFunctionDefn *)(node)); break;
        case AST_FUNCTION_CALL: emit_function_call(cg, (AstFunctionCall *)(node)); break;
        case AST_RETURN:        emit_return(cg, (AstReturn *)(node)); break;
        case AST_BLOCK:         emit_block(cg, (AstBlock *)(node), true); break;
        case AST_PRINT:         emit_print(cg, (AstPrint *)(node)); break;
        case AST_ASSERT:        emit_assert(cg, (AstAssert *)(node)); break;
        case AST_TYPEOF:        emit_typeof(cg, (AstTypeof *)(node)); break;
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

    int address = 0;
    if (assign->lhs->head.type == AST_LITERAL) {
        AstIdentifier *ident = symbol_lookup(&cg->ident_table, ((AstLiteral *)(assign->lhs))->as.value.identifier.name)->as.identifier;
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
        switch (assign->expr->evaluated_type->kind) {
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
        case TYPE_BOOL: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], al\n", address);
            break;
        }
        case TYPE_STRING: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], rax\n", address);
            break;
        }
        case TYPE_ENUM: {
            sb_append(&cg->code, "   pop\t\trax\n");
            sb_append(&cg->code, "   mov\t\t%d[rbp], eax\n", address);
            break;
        }
        default: XXX;
    }
    } break;
    case ASSIGN_PLUS_EQUAL: {
        if (assign->expr->evaluated_type->kind == TYPE_INTEGER) {
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
        if (assign->expr->evaluated_type->kind == TYPE_INTEGER) {
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
        if (assign->expr->evaluated_type->kind == TYPE_INTEGER) {
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
        if (assign->expr->evaluated_type->kind == TYPE_INTEGER) {
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

    size_t bytes_allocated = func_defn->bytes_allocated;;
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

    if (!is_primitive_type(expr_type->kind)) {
        report_error_ast(cg->parser, LABEL_ERROR, (Ast *)(ast_typeof), "typeof() of type other than primitive types is not yet implemented");
        exit(1);
    }

    PrimitiveKind type_kind  = ((TypePrimitive *)(expr_type))->kind;
    char         *type_name  = primitive_type_names[type_kind];
    int           type_label = make_label_number(cg);

    sb_append(&cg->data, "   T%d\tDB \"%s\", 10, 0", type_label, type_name);
    sb_append(&cg->code, "   mov\t\trcx, T%d\n", type_label);
    sb_append(&cg->code, "   call\t\tprintf\n");
}

const char *REG_A(Type *type) {
    if (type->size == 1) return "al";
    if (type->size == 2) return "ax";
    if (type->size == 4) return "eax";
    if (type->size == 8) return "rax";
    XXX;
}

const char *REG_B(Type *type) {
    if (type->size == 1) return "bl";
    if (type->size == 2) return "bx";
    if (type->size == 4) return "ebx";
    if (type->size == 8) return "rbx";
    XXX;
}

const char *REG_C(Type *type) {
    if (type->size == 1) return "cl";
    if (type->size == 2) return "cx";
    if (type->size == 4) return "ecx";
    if (type->size == 8) return "rcx";
    XXX;
}

const char *REG_D(Type *type) {
    if (type->size == 1) return "dl";
    if (type->size == 2) return "dx";
    if (type->size == 4) return "edx";
    if (type->size == 8) return "rdx";
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
    else {
        // Unhandled cases
        XXX;
    }
}

void zero_initialize(CodeGenerator *cg, AstDeclaration *decl, int stack_offset) {
    switch (decl->declared_type->kind) {
        case TYPE_INTEGER: sb_append(&cg->code, "   mov\t\t%s %d[rbp], 0\n", DT(decl->declared_type), stack_offset); break;
        case TYPE_FLOAT:   sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_BOOL:    sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], 0\n",  stack_offset); break;
        case TYPE_STRING:  sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_ENUM:    sb_append(&cg->code, "   mov\t\tDWORD %d[rbp], 0\n", stack_offset); break;
        case TYPE_STRUCT: {
            AstStruct *struct_defn = ((TypeStruct *)(decl->declared_type))->node;
            assert(struct_defn != NULL);

            DynamicArray members = get_struct_members(struct_defn);
            for (unsigned int i = 0; i < members.count; i++) {
                AstDeclaration *member = ((AstDeclaration **)(members.items))[i];
                zero_initialize(cg, member, stack_offset + member->member_offset);
            }
            free(members.items);
            break;
        }
        default:
            printf("%s:%d: compiler-error: There were unhandled cases in 'zero_initialize'\n", __FILE__, __LINE__);
            exit(1);
    }
}

void assign_simple_value(CodeGenerator *cg, int address, Type *lhs_type, Type *rhs_type) {
    if (lhs_type->kind == TYPE_INTEGER) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\t%s %d[rbp], %s\n", DT(lhs_type), address, REG_A(lhs_type)); // nocheckin
    }
    else if (lhs_type->kind == TYPE_FLOAT) {
        if (rhs_type->kind == TYPE_INTEGER) {
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
    else if (lhs_type->kind == TYPE_BOOL) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tBYTE %d[rbp], al\n", address);
    }
    else if (lhs_type->kind == TYPE_STRING) {
        sb_append(&cg->code, "   pop\t\trax\n");
        sb_append(&cg->code, "   mov\t\tQWORD %d[rbp], rax\n", address);
    }
    else if (lhs_type->kind == TYPE_ENUM) {
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
        if (init->value->head.type == AST_STRUCT_LITERAL) {
            emit_struct_initialization(cg, (AstStructLiteral *)(init->value), member_address);
        } else {
            assign_simple_value(cg, member_address, init->member->declared_type, init->value->evaluated_type);
        }
    }
}

void emit_declaration(CodeGenerator *cg, AstDeclaration *decl) {
    if (decl->flags & DECLARATION_CONSTANT) {
        assert(decl->expr->head.type == AST_LITERAL);
        AstLiteral *lit = (AstLiteral *)(decl->expr);

        switch (lit->kind) {
            case LITERAL_BOOLEAN: break; // Immediate value is used
            case LITERAL_INTEGER: break; // Immediate value is used
            case LITERAL_FLOAT:   sb_append(&cg->data, "   C_%s DD %lf\n", decl->identifier->name, lit->as.value.floating); break;
            case LITERAL_STRING:  sb_append(&cg->data, "   C_%s DB \"%s\"\n", decl->identifier->name, lit->as.value.string.data); break;
            case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
        }

        return;
    }

    int type_size = decl->declared_type->size;
    cg->base_ptr -= type_size;
    cg->base_ptr  = align_value(cg->base_ptr, type_size);

    decl->identifier->stack_offset = cg->base_ptr;
    int stack_offset               = cg->base_ptr;

    sb_append(&cg->code, "\n");
    sb_append(&cg->code, "   ; initialization of '%s'\n", decl->identifier->name);
    zero_initialize(cg, decl, stack_offset);
    
    if (!(decl->flags & DECLARATION_TYPED_NO_EXPR)) {
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


    if (l_kind == TYPE_INTEGER && r_kind == TYPE_INTEGER) {
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

    if ((l_kind == TYPE_FLOAT   && r_kind == TYPE_FLOAT) ||   
        (l_kind == TYPE_INTEGER && r_kind == TYPE_FLOAT) ||
        (l_kind == TYPE_FLOAT   && r_kind == TYPE_INTEGER)) { 

        emit_expression(cg, bin->left);
        emit_expression(cg, bin->right);

        emit_integer_to_float_conversion(cg, l_kind, r_kind);

        const char *set_instruction = comparison_operator_to_set_instruction(bin->operator);
        sb_append(&cg->code, "   comiss\txmm0, xmm1\n");
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

int member_access_address(CodeGenerator *cg, AstMemberAccess *ma) {
    AstIdentifier *ident = symbol_lookup(&cg->ident_table, ma->ident->name)->as.identifier;
    int stack_offset     = ident->stack_offset;
    for (unsigned int i = 0; i < ma->chain.count; i++) {
        AstAccessor *ac = ((AstAccessor **)(ma->chain.items))[i];
        stack_offset += ac->struct_member->member_offset;
    }

    return stack_offset;
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
        if (ac->kind == ACCESSOR_STRUCT) {
            AstDeclaration *member = ac->struct_member;
            if (member->declared_type->kind == TYPE_INTEGER) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type->kind == TYPE_FLOAT) {
                sb_append(&cg->code, "   movss\t\txmm0, %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   sub\t\trsp, 4\n");
                sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                return;
            }
            if (member->declared_type->kind == TYPE_BOOL) {
                sb_append(&cg->code, "   mov\t\tal, BYTE %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type->kind == TYPE_STRING) {
                sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (member->declared_type->kind == TYPE_ENUM) {
                sb_append(&cg->code, "   mov\t\teax, DWORD %d[rbp]\n", stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            XXX;
        } else if (ac->kind == ACCESSOR_ENUM) {
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
        case LITERAL_IDENTIFIER: {
            Symbol *ident_symbol = symbol_lookup(&cg->ident_table, lit->as.value.identifier.name);
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
                        sb_append(&cg->code, "   movss\t\txmm0, [C_%s]\n", ident->name);
                        sb_append(&cg->code, "   sub\t\trsp, 4\n"); 
                        sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                        return;
                    }
                    case LITERAL_STRING: {
                        sb_append(&cg->code, "   mov\t\trax, C_%s\n", ident->name);
                        sb_append(&cg->code, "   push\t\trax");
                        return;
                    }
                    case LITERAL_IDENTIFIER: assert(false); // Shouldn't happen
                }
            }

            // @Improvement - Could probably be @Refactored. Looks very similar to 'assign_simple_value'
            if (ident->type->kind == TYPE_INTEGER) {
                sb_append(&cg->code, "   mov\t\trax, 0\n");
                sb_append(&cg->code, "   mov\t\t%s, %s %d[rbp]\n", REG_A(ident->type), DT(ident->type), ident->stack_offset);
                if (is_signed_integer(ident->type) && ident->type->size != 8) {
                    sb_append(&cg->code, "   movsx\t\trax, %s\n", REG_A(ident->type));
                }
                sb_append(&cg->code, "   push\t\trax\n", ident->stack_offset);
                return;
            }
            if (ident->type->kind == TYPE_FLOAT) {
                sb_append(&cg->code, "   movss\t\txmm0, %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   sub\t\trsp, 4\n");
                sb_append(&cg->code, "   movss\t\t[rsp], xmm0\n");
                return;
            }
            if (ident->type->kind == TYPE_BOOL) {
                sb_append(&cg->code, "   mov\t\tal, BYTE %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (ident->type->kind == TYPE_STRING) {
                sb_append(&cg->code, "   mov\t\trax, QWORD %d[rbp]\n", ident->stack_offset);
                sb_append(&cg->code, "   push\t\trax\n");
                return;
            }
            if (ident->type->kind == TYPE_ENUM) {
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

// Convert type to one of the four intel data types
char *DT(Type *type) {
    switch (type->kind) {
    case TYPE_INVALID: XXX;
    case TYPE_NAME: XXX;
    case TYPE_VOID: XXX;
    case TYPE_BOOL: 
    case TYPE_INTEGER:
    case TYPE_FLOAT:
    case TYPE_STRING: {
        TypePrimitive *prim_type = (TypePrimitive *)(type);
        switch (prim_type->kind) {
            case PRIMITIVE_INVALID: XXX;
            case PRIMITIVE_INT:     return "DWORD";
            case PRIMITIVE_U8:      return "BYTE";
            case PRIMITIVE_U16:     return "WORD";
            case PRIMITIVE_U32:     return "DWORD";
            case PRIMITIVE_U64:     return "QWORD";
            case PRIMITIVE_S8:      return "BYTE";
            case PRIMITIVE_S16:     return "WORD";
            case PRIMITIVE_S32:     return "DWORD";
            case PRIMITIVE_S64:     return "QWORD";
            case PRIMITIVE_FLOAT:   XXX;
            case PRIMITIVE_F32:     XXX;
            case PRIMITIVE_F64:     XXX;
            case PRIMITIVE_STRING:  return "QWORD";
            case PRIMITIVE_BOOL:    return "BYTE";
            case PRIMITIVE_VOID:    XXX;
            case PRIMITIVE_COUNT:   XXX;      
        }
        XXX; // unreachable
    }
    case TYPE_STRUCT: XXX;
    case TYPE_ENUM: {
        TypeEnum *enum_type = (TypeEnum *)(type);
        return DT(enum_type->backing_type);
    };
    }
    XXX; // unreachable
}