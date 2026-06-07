#include "code_generator.c"

typedef struct X64Converter {
    StringBuilder head;            // Declaring bit target and other misc stuff
    StringBuilder data;            // Corresponding to section .data
    StringBuilder rdata;           // Corresponding to section .rdata
    StringBuilder rdata_string;    // Corresponding to section .rdata
    StringBuilder code;           // Corresponding to section .text. Always points to the top of the code_stack
    StringBuilder code_header;     // Header of externs, globals etc within the .text section

    DynamicArray     *bytecode_functions; // of *BytecodeFunction
    BytecodeFunction *current_bytecode_function;
} X64Converter;

void x64_emit_basic_block(X64Converter *conv, BasicBlock *bb);
void x64_emit_bytecode_function(X64Converter *conv, BytecodeFunction *func);
void x64_emit_instruction(X64Converter *conv, Inst *inst);

X64Converter x64_converter_init(BytecodeGenerator *bcg) {
    X64Converter conv = {0};

    conv.head = sb_init(1024);
    conv.data = sb_init(1024);
    conv.rdata = sb_init(1024);
    conv.rdata_string = sb_init(1024);
    conv.code = sb_init(1024);
    conv.code_header = sb_init(1024);

    conv.bytecode_functions = &bcg->bytecode_functions;

    return conv;
}

void x64_emit_header(X64Converter *conv) {
    sb_append(&conv->head, "bits 64\n");
    sb_append(&conv->head, "default rel\n");
    sb_append(&conv->head, "\n");

    sb_append(&conv->data, "segment .data\n");
    sb_append(&conv->data, "   fmt_int   DB \"%s\", 0\n", "%lld");
    sb_append(&conv->data, "   fmt_float DB \"%s\", 0\n", "%lf");
    sb_append(&conv->data, "   fmt_string DB \"%s\", 0\n", "%s");
    sb_append(&conv->data, "   fmt_address DB \"0x%s\", 0\n", "%lx");
    sb_append(&conv->data, "   string_false DB \"false\", 0\n");
    sb_append(&conv->data, "   string_true  DB \"true\", 0\n");
    sb_append(&conv->data, "   string_assert_fail  DB \"Assertion failed at line %s\", 10, 0\n", "%d");
    sb_append(&conv->data, "   enum_to_int_buffer times 20 DB 0\n"); // 20 is the length of the largest integer number 2^64

    sb_append(&conv->rdata_string, "segment .rdata\n");

    sb_append(&conv->rdata, "segment .rdata\n");

    sb_append(&conv->code_header, "segment .text\n");
    sb_append(&conv->code_header, "   global main\n");

    sb_append(&conv->code_header, "   extern ExitProcess\n");
    sb_append(&conv->code_header, "   extern printf\n");
    sb_append(&conv->code_header, "   extern sprintf\n");
    sb_append(&conv->code_header, "   extern malloc\n");
    sb_append(&conv->code_header, "   extern calloc\n");
    sb_append(&conv->code_header, "   extern free\n");
    sb_append(&conv->code_header, "   extern memset\n");
    sb_append(&conv->code_header, "   extern memcpy\n");

    // Runtime_Support types
    sb_append(&conv->code_header, "   extern Type_uint\n");
    sb_append(&conv->code_header, "   extern Type_u8\n");
    sb_append(&conv->code_header, "   extern Type_u16\n");
    sb_append(&conv->code_header, "   extern Type_u32\n");
    sb_append(&conv->code_header, "   extern Type_u64\n");
    sb_append(&conv->code_header, "   extern Type_int\n");
    sb_append(&conv->code_header, "   extern Type_s8\n");
    sb_append(&conv->code_header, "   extern Type_s16\n");
    sb_append(&conv->code_header, "   extern Type_s32\n");
    sb_append(&conv->code_header, "   extern Type_s64\n");
    sb_append(&conv->code_header, "   extern Type_float\n");
    sb_append(&conv->code_header, "   extern Type_f32\n");
    sb_append(&conv->code_header, "   extern Type_f64\n");
    sb_append(&conv->code_header, "   extern Type_string\n");
    sb_append(&conv->code_header, "   extern Type_bool\n");
    sb_append(&conv->code_header, "   extern Type_void\n");
    sb_append(&conv->code_header, "   extern Type_untyped_int\n");
    sb_append(&conv->code_header, "   extern Type_untyped_float\n");
}

void x64_begin_convert(X64Converter *conv) {
    x64_emit_header(conv);

    for (int i = 0; i < conv->bytecode_functions->count; i++) {
        BytecodeFunction *func = da_get_deref(*conv->bytecode_functions, i);
        x64_emit_bytecode_function(conv, func);
    }

}

int get_vreg_stack_offset(X64Converter *conv, int vreg) {
   int *stack_slot = da_get(conv->current_bytecode_function->vreg_to_stack_slot, vreg);
   return *stack_slot;
}

void set_vreg_stack_offset(X64Converter *conv, int vreg, int offset) {
   int *stack_slot = da_get(conv->current_bytecode_function->vreg_to_stack_slot, vreg);
   *stack_slot = offset;
}

// Sets a stack offset for the virtual register vreg
int spill_vreg(X64Converter *conv, int vreg) {
    int current_offset = get_vreg_stack_offset(conv, vreg);
    if (current_offset == 0) {
        conv->current_bytecode_function->frame_size += 8;

        int offset = -conv->current_bytecode_function->frame_size;
        set_vreg_stack_offset(conv, vreg, offset);
        return offset;
    }

    return current_offset;
}

void x64_compute_function_stack_frame(X64Converter *conv, BytecodeFunction *func) {

    for (int i = 0; i < func->params.count; i++) {
        BytecodeFunctionParameter *param = da_get(func->params, i);
        spill_vreg(conv, param->vreg);
    }

    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        for (int j = 0; j < bb->instructions.count; j++) {
            Inst *inst = da_get(bb->instructions, j);

            if (inst->kind == INST_CALL) {
                InstFunctionCall *call = inst->data;
                for (int i = 0; i < call->arg_vregs.count; i++) {
                    int *vreg = da_get(call->arg_vregs, i);
                    spill_vreg(conv, *vreg);
                }
            }

            for (u8 k = 0; k < inst->op_count; k++) {
                Operand *op = &inst->operands[k];
                if (op->kind == OPERAND_REG) {
                    spill_vreg(conv, op->vreg);
                    op->kind = OPERAND_MEM;
                }
                else if (op->kind == OPERAND_MEM) {
                    func->frame_size += 8;
                    // op->offset = -func->frame_size;
                }
            }

        }
    }

    func->frame_size += 32;
}

void x64_emit_bytecode_function(X64Converter *conv, BytecodeFunction *func) {
    conv->current_bytecode_function = func;

    // Compute frame size
    x64_compute_function_stack_frame(conv, func);

    sb_append(&conv->code, "%s:\n", func->symbol_name);
    sb_append(&conv->code, "\tpush\t\trbp\n");
    sb_append(&conv->code, "\tmov\t\trbp, rsp\n");
    sb_append(&conv->code, "\tsub\t\trsp, %d\n", func->frame_size);

    // Check if we should save arguments to the shadow space
    bool save_arguments_in_shadow_space = false;

    // Move parameters into their home-address
    for (int i = 0; i < func->params.count; i++) {
        BytecodeFunctionParameter *param = da_get(func->params, i);

        int param_offset = get_vreg_stack_offset(conv, param->vreg);

        Register input_reg = REG_RAX;
        if (i < 4) {
            if (save_arguments_in_shadow_space) {
                XXX;
            } else {
                input_reg = get_argument_register_from_index(i, param->type);
                if (param->type->size > 8) {
                    sb_append(&conv->code, "   mov\t\trax, %s\n", register_to_str(input_reg, 8));
                }  
            }
        } else {
            XXX;
        }

        if (param->type->size <= 8) {
            sb_append(&conv->code, "   mov\t\t%d[rbp], %s\n", param_offset, register_to_str(input_reg, param->type->size));
        } else {
            // We have the source pointer in rax
            XXX;
            // sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", param->stack_offset);
            // emit_memcpy(cg, 0, 0, param->type->size);
        }

    }


    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);
        x64_emit_basic_block(conv, bb);
    }

    sb_append(&conv->code, "\tadd\t\trsp, %d\n", func->frame_size);
    sb_append(&conv->code, "\tpop\t\trbp\n");
    sb_append(&conv->code, "\tret\n");
}

Register x64_get_argument_register_from_index(int index) {
    if (index == 0) return REG_RCX;
    if (index == 1) return REG_RDX;
    if (index == 2) return REG_R8;
    if (index == 3) return REG_R9;
    return REG_NONE;
}

void x64_emit_call_instruction(X64Converter *conv, Inst *inst) {
    InstFunctionCall *call = inst->data;

    for (int i = 0; i < call->arg_vregs.count; i++) {
        int *arg_vreg = da_get(call->arg_vregs, i);
        int arg_offset = get_vreg_stack_offset(conv, *arg_vreg);

        Register reg = x64_get_argument_register_from_index(i);

        sb_append(&conv->code, "\tmov\t\t%s, %d[rbp]\n", register_to_str(reg, 8), arg_offset);
    }

    sb_append(&conv->code, "\tcall\t\t%s\n", inst->op2.label);
}

void x64_emit_basic_block(X64Converter *conv, BasicBlock *bb) {
    for (int i = 0; i < bb->instructions.count; i++) {
        Inst *inst = da_get(bb->instructions, i);
        x64_emit_instruction(conv, inst);
    }
}

void x64_emit_arithmetic_instruction(X64Converter *conv, Inst *inst) {

    sb_append(&conv->code, "   mov\t\trax, %d[rbp]\n", get_vreg_stack_offset(conv, inst->op2.vreg));
    sb_append(&conv->code, "   mov\t\trbx, %d[rbp]\n", get_vreg_stack_offset(conv, inst->op3.vreg));

    if (inst->kind == INST_ADD_INT) {
        sb_append(&conv->code, "   add\t\trax, rbx\n");
        sb_append(&conv->code, "   mov\t\t%d[rbp], rax\n", get_vreg_stack_offset(conv, inst->op1.vreg));
        return;
    }

    if (inst->kind == INST_MUL_INT) {
        sb_append(&conv->code, "   imul\t\trax, rbx\n");
        sb_append(&conv->code, "   mov\t\t%d[rbp], rax\n", get_vreg_stack_offset(conv, inst->op1.vreg));
        return;
    }

    // sb_append(&cg->code, "   sub\t\trax, rbx\n");
    // sb_append(&cg->code, "   imul\t\trax, rbx\n");
}

void x64_emit_instruction(X64Converter *conv, Inst *inst) {
    Operand *op1 = &inst->op1;
    Operand *op2 = &inst->op2;

    switch (inst->kind) {
    case INST_NOOP: {
        break;
    }
    case INST_MOV: {
        if (op1->kind == OPERAND_MEM && op2->kind == OPERAND_MEM) {
            sb_append(&conv->code, "\tmov\t\trax, %d[rbp]\n", get_vreg_stack_offset(conv, op2->vreg));
            sb_append(&conv->code, "\tmov\t\t%d[rbp], rax\n", get_vreg_stack_offset(conv, op1->vreg));
        }
        else if (op1->kind == OPERAND_MEM && op2->kind == OPERAND_IMM_INT) {
            sb_append(&conv->code, "\tmov\t\t%d[rbp], %lld\n", get_vreg_stack_offset(conv, op1->vreg), op2->imm_int);
        }
        else if (op1->kind == OPERAND_MEM && op2->kind == OPERAND_IMM_UINT) {
            sb_append(&conv->code, "\tmov\t\t%d[rbp], %zu\n", get_vreg_stack_offset(conv, op1->vreg), op2->imm_uint);
        }
        else if (op1->kind == OPERAND_REG && op2->kind == OPERAND_REG) {
            XXX;
        } else {
            XXX;
        }

        break;
    }
    case INST_LOAD: {
        break;
    }
    case INST_STORE: {
        break;
    }
    case INST_CALL: {
        x64_emit_call_instruction(conv, inst);
        break;
    }
    case INST_PUSH: {
        break;
    }
    case INST_POP: {
        break;
    }
    case INST_LABEL: {
        sb_append(&conv->code, "%s:\n", inst->op1.label);
        break;
    }
    case INST_RETURN: {
        break;
    }
    case INST_ADD_INT:
    case INST_SUB_INT:
    case INST_MUL_INT:
    case INST_DIV_INT:
    case INST_ADD_FLOAT:
    case INST_SUB_FLOAT:
    case INST_MUL_FLOAT:
    case INST_DIV_FLOAT: {
        x64_emit_arithmetic_instruction(conv, inst);
        break;
    }
    default: {
        break;
    }
    }
}

void x64_output_generated_x64_to_file(X64Converter *conv, const char *output_path) {
    FILE *f = fopen(output_path, "w");
    if (f == NULL) {
        printf("%s:%d: error: Failed to open file '%s'\n", __FILE__, __LINE__, output_path);
        exit(1);
    }

    fwrite(conv->head.buffer, 1, conv->head.cursor, f);
    fwrite(conv->data.buffer, 1, conv->data.cursor, f);
    fwrite(conv->rdata_string.buffer, 1, conv->rdata_string.cursor, f);
    fwrite(conv->rdata.buffer, 1, conv->rdata.cursor, f);
    fwrite(conv->code_header.buffer, 1, conv->code_header.cursor, f);
    fwrite(conv->code.buffer, 1, conv->code.cursor, f);
    fwrite("\n", 1, 1, f);

    fclose(f);
}