#include "code_generator.c"

typedef struct X64Converter {
    StringBuilder head;            // Declaring bit target and other misc stuff
    StringBuilder data;            // Corresponding to section .data
    StringBuilder rdata;           // Corresponding to section .rdata
    StringBuilder rdata_string;    // Corresponding to section .rdata
    StringBuilder code;            // Corresponding to section .text. Always points to the top of the code_stack
    StringBuilder code_header;     // Header of externs, globals etc within the .text section

    ConstantPool *constant_pool;
    int           constant_data_id;

    DynamicArray     *bytecode_functions; // of *BytecodeFunction
    BytecodeFunction *current_bytecode_function;

    Arena temp_string_arena;
} X64Converter;

typedef struct OffsetOrRegister {
    bool     is_reg;
    Register reg;
    int      offset;
} OffsetOrRegister;

void x64_emit_basic_block(X64Converter *conv, BasicBlock *bb);
void x64_emit_function_defn(X64Converter *conv, BytecodeFunction *func);
void x64_emit_instruction(X64Converter *conv, Inst *inst);

X64Converter x64_converter_init(BytecodeGenerator *bcg) {
    X64Converter conv = {0};

    conv.head = sb_init(1024);
    conv.data = sb_init(1024);
    conv.rdata = sb_init(1024);
    conv.rdata_string = sb_init(1024);
    conv.code = sb_init(1024);
    conv.code_header = sb_init(1024);
    conv.temp_string_arena = arena_init(1024);
    
    conv.constant_pool = &bcg->constant_pool;

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

int x64_get_next_constant_data_id(X64Converter *conv) {
    return conv->constant_data_id++;
}

void x64_emit_constant_pool(X64Converter *conv) {
    for (int i = 0; i < conv->constant_pool->constants.count; i++) {
        Constant *constant = da_get(conv->constant_pool->constants, i);

        switch (constant->kind) {
            case CONSTANT_NONE: {
                XXX;
                break;
            }
            case CONSTANT_STRING: {
                int str_data_id = x64_get_next_constant_data_id(conv);

                sb_append(&conv->data, "\t_data_%d DB \"%s\", 0\n", str_data_id, constant->as.value_string.data);
                sb_append(&conv->data, "\talign 8\n");
                sb_append(&conv->data, "\tSTRING_%d:\n", constant->id);
                sb_append(&conv->data, "\tdq _data_%d\n", str_data_id);
                sb_append(&conv->data, "\tdq %d:\n", constant->as.value_string.len);

                break;
            }
            case CONSTANT_FLOAT: {
                sb_append(&conv->data, "\talign %d\n", constant->type->size);
                sb_append(&conv->data, "\tFLOAT_%d:\n", constant->id);
                sb_append(&conv->data, "\t%s %llf\n", constant->type->size == 8 ? "dq" : "dd", constant->as.value_float);

                break;
            }
            default: {
                XXX;
            }
        }
    }
}

void x64_begin_convert(X64Converter *conv) {
    x64_emit_header(conv);
    x64_emit_constant_pool(conv);

    for (int i = 0; i < conv->bytecode_functions->count; i++) {
        BytecodeFunction *func = da_get_deref(*conv->bytecode_functions, i);
        x64_emit_function_defn(conv, func);
    }

}

Register gpr_register_index_to_x86_register(int reg_index) {
    Register gpr_x86_registers[] = {
        REG_RCX,
        REG_RDX,
        REG_R8,
        REG_R9,
        REG_R10,
        REG_R11,
        REG_R12,
        REG_R13,
        REG_R14,
        REG_R15,
    };

    return gpr_x86_registers[reg_index];
}

Register sse_register_index_to_x86_register(int reg_index) {
    Register sse_x86_registers[] = {
        REG_XMM0,
        REG_XMM1,
        REG_XMM2,
        REG_XMM3,
        REG_XMM4,
        REG_XMM5,
        REG_XMM6,
        REG_XMM7,
    };

    return sse_x86_registers[reg_index];
}

char *register_index_to_string(int reg_index, int size) {
    Register x86_reg = gpr_register_index_to_x86_register(reg_index);
    return register_to_str(x86_reg, size);
}

char *sse_register_index_to_string(int reg_index, int size) {
    Register x86_reg = sse_register_index_to_x86_register(reg_index);
    return register_to_str(x86_reg, size);
}

char *get_register_string_sized(Operand op, int size) {
    Register x86_reg;
    if (op.is_sse) {
        x86_reg = sse_register_index_to_x86_register(op.reg);
    } else {
        x86_reg = gpr_register_index_to_x86_register(op.reg);
    }
    return register_to_str(x86_reg, size);
}

char *get_register_string(Operand op) {
    return get_register_string_sized(op, op.size);
}


int allocate_local_variable(X64Converter *conv, int size) {
    assert(conv->current_bytecode_function);

    conv->current_bytecode_function->base_ptr -= size;
    conv->current_bytecode_function->base_ptr = align_value(conv->current_bytecode_function->base_ptr, size > 8 ? 8 : size);

    return conv->current_bytecode_function->base_ptr;
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
        conv->current_bytecode_function->local_stack_size += 8;

        int offset = -conv->current_bytecode_function->local_stack_size;
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

    func->local_stack_size += func->num_spill_slots * 8;

    // Shadow space
    func->local_stack_size += 32;
}

void x64_emit_function_defn(X64Converter *conv, BytecodeFunction *func) {

    if (func->is_extern) {
        sb_append(&conv->code_header, "\textern %s\n", func->symbol_name);    
        return;
    }

    conv->current_bytecode_function = func;

    // Compute frame size
    x64_compute_function_stack_frame(conv, func);

    //
    // Prolog
    //
    sb_append(&conv->code, "%s:\n", func->symbol_name);
    sb_append(&conv->code, "\tpush\t\trbp\n");
    sb_append(&conv->code, "\tmov\t\trbp, rsp\n");
    sb_append(&conv->code, "\tsub\t\trsp, %d\n", func->local_stack_size);

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
            // XXX;
        }

        if (param->type->size <= 8) {
            sb_append(&conv->code, "   mov\t\t%d[rbp], %s\n", param_offset, register_to_str(input_reg, param->type->size));
        } else {
            // We have the source pointer in rax
            // XXX;
            // sb_append(&cg->code, "   lea\t\trbx, %d[rbp]\n", param->stack_offset);
            // emit_memcpy(cg, 0, 0, param->type->size);
        }

    }


    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);
        x64_emit_basic_block(conv, bb);
    }

    //
    // Epilog
    //
    sb_append(&conv->code, "\tmov\t\trax, 0\n");
    sb_append(&conv->code, "\tadd\t\trsp, %d\n", func->local_stack_size);
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

#define X64_CALL_SHUFFLE_SCRATCH "rax"

typedef struct ArgRegMove {
    char *dst;
    char *src;
    bool  done;
} ArgRegMove;

void x64_sequence_arg_moves(X64Converter *conv, ArgRegMove *moves, int count) {
    bool pending = true;

    while (pending) {
        pending = false;
        bool progressed = false;

        for (int i = 0; i < count; i++) {
            if (moves[i].done) continue;
            pending = true;

            bool needed_as_src_elsewhere = false;
            for (int j = 0; j < count; j++) {
                if (j == i || moves[j].done) continue;
                if (strcmp(moves[j].src, moves[i].dst) == 0) {
                    needed_as_src_elsewhere = true;
                    break;
                }
            }

            if (!needed_as_src_elsewhere) {
                if (strcmp(moves[i].dst, moves[i].src) != 0) {
                    sb_append(&conv->code, "\tmov\t\t%s, %s\n", moves[i].dst, moves[i].src);
                }
                moves[i].done = true;
                progressed = true;
            }
        }

        if (pending && !progressed) {
            // Everything left is one cycle. Stash one destination's current
            // value, then redirect whichever move depended on reading it.
            for (int i = 0; i < count; i++) {
                if (moves[i].done) continue;

                sb_append(&conv->code, "\tmov\t\t%s, %s\n", X64_CALL_SHUFFLE_SCRATCH, moves[i].dst);

                for (int j = 0; j < count; j++) {
                    if (j == i || moves[j].done) continue;
                    if (strcmp(moves[j].src, moves[i].dst) == 0) {
                        moves[j].src = X64_CALL_SHUFFLE_SCRATCH;
                    }
                }
                break; // only break one cycle per outer pass
            }
        }
    }
}

void x64_emit_call_instruction(X64Converter *conv, Inst *inst) {
    InstFunctionCall *call = inst->data;

    ArgRegMove reg_moves[4] = {0};
    int        reg_move_count = 0;

    for (int i = 0; i < call->arguments.count; i++) {
        CallArgument *arg = da_get(call->arguments, i);
        char *src_str = register_index_to_string(arg->vreg, 8);

        Register abi_reg = x64_get_argument_register_from_index(i);
        if (abi_reg == REG_NONE) {
            // Needs to be moved to the stack
            int stack_offset = 32 + (i - 4) * 8;
            sb_append(&conv->code, "\tmov\t\t%d[rsp], %s\n", stack_offset, src_str);
            continue;
        }

        reg_moves[reg_move_count].dst = register_to_str(abi_reg, 8);
        reg_moves[reg_move_count].src = src_str;
        reg_move_count++;
    }

    x64_sequence_arg_moves(conv, reg_moves, reg_move_count);

    sb_append(&conv->code, "\tcall\t\t%s\n", inst->op2.label);
}

void x64_emit_terminator(X64Converter *conv, BasicBlock *bb) {
    switch (bb->terminator.kind) {
        case TERMINATOR_NONE: break;
        case TERMINATOR_JUMP: {
            sb_append(&conv->code, "   jmp\t\tL%d\n", bb->terminator.target1->id);
            break;
        }
        case TERMINATOR_COND_JUMP: {
            char *cmp_reg = register_index_to_string(bb->terminator.condition_reg, 1);
            sb_append(&conv->code, "   cmp\t\t%s, 0\n", cmp_reg);
            sb_append(&conv->code, "   jz\t\t\tL%d\n", bb->terminator.target2->id);
            sb_append(&conv->code, "   jmp\t\tL%d\n", bb->terminator.target1->id);
            break;
        }
        case TERMINATOR_RETURN: {
            break;
        }
    }
}

void x64_emit_basic_block(X64Converter *conv, BasicBlock *bb) {

    arena_clear(&conv->temp_string_arena);

    sb_append(&conv->code, "L%d:\n", bb->id);
    for (int i = 0; i < bb->instructions.count; i++) {
        Inst *inst = da_get(bb->instructions, i);
        x64_emit_instruction(conv, inst);
    }

    x64_emit_terminator(conv, bb);
}

void x64_emit_comparrison_instruction(X64Converter *conv, Inst *inst) {
    char *dst = register_index_to_string(inst->op1.reg, 1);
    char *a   = register_index_to_string(inst->op2.reg, 8);
    char *b   = register_index_to_string(inst->op3.reg, 8);

    /*
    INST_LESS_THAN
    INST_GREATER_THAN
    INST_GREATER_THAN_EQUAL
    INST_LESS_THAN_EQUAL
    INST_DOUBLE_EQUAL
    */

    /*
    if (do_signed_comparison) {
        if (op == '<')                 return "setl";
        if (op == '>')                 return "setg";
        if (op == TOKEN_GREATER_EQUAL) return "setge";
        if (op == TOKEN_LESS_EQUAL)    return "setle";
        if (op == TOKEN_DOUBLE_EQUAL)  return "sete";
        if (op == TOKEN_NOT_EQUAL)     return "setne";
    }
    */

    // if (a != dst) {
    //     sb_append(&conv->code, "   mov\t\t%s, %s\n", dst, a);
    // }

    bool do_signed_comparison = true;

    if (do_signed_comparison) {
        char *set_inst = "";
        if (inst->kind == INST_LESS_THAN)            set_inst = "setl";
        if (inst->kind == INST_GREATER_THAN)         set_inst = "setg";
        if (inst->kind == INST_GREATER_THAN_EQUAL)   set_inst = "setge";
        if (inst->kind == INST_LESS_THAN_EQUAL)      set_inst = "setle";
        if (inst->kind == INST_DOUBLE_EQUAL)         set_inst = "sete";
        if (inst->kind == INST_NOT_EQUAL)            set_inst = "setne";

        sb_append(&conv->code, "   cmp\t\t%s, %s\n", a, b);
        sb_append(&conv->code, "   %s\t\t%s\n", set_inst, dst);

        return;
    }

}

char *get_mov_string(Operand op) {
    if (op.is_sse) {
        return op.size == 8 ? "movsd" : "movss";
    } else {
        return "mov";
    }
}

void x64_emit_arithmetic_instruction(X64Converter *conv, Inst *inst) {
    bool is_sse = inst->op1.is_sse;
    int operand_size = inst->op2.size;

    char *dst = get_register_string(inst->op1);
    char *a   = get_register_string(inst->op2);
    char *b   = get_register_string(inst->op3);

    if (a != dst) {
        sb_append(&conv->code, "   %s\t\t%s, %s\n", get_mov_string(inst->op1), dst, a);
    }

    switch(inst->kind) {
        case INST_ADD_INT: {
            sb_append(&conv->code, "   add\t\t%s, %s\n", dst, b);
            break;
        }
        case INST_SUB_INT: {
            sb_append(&conv->code, "   sub\t\t%s, %s\n", dst, b);
            break;
        }
        case INST_MUL_INT: {
            sb_append(&conv->code, "   imul\t\t%s, %s\n", dst, b);
            break;
        }
        case INST_DIV_INT: {
            XXX;
            break;
        }
        case INST_ADD_FLOAT: {
            if (operand_size == 8) {
                sb_append(&conv->code, "   addsd\t\t%s, %s\n", dst, b);
            } else {
                sb_append(&conv->code, "   addss\t\t%s, %s\n", dst, b);
            }
            break;
        }
        case INST_SUB_FLOAT: {
            if (operand_size == 8) {
                sb_append(&conv->code, "   subsd\t\t%s, %s\n", dst, b);
            } else {
                sb_append(&conv->code, "   subss\t\t%s, %s\n", dst, b);
            }
            break;
        }
        case INST_MUL_FLOAT: {
            if (operand_size == 8) {
                sb_append(&conv->code, "   mulsd\t\t%s, %s\n", dst, b);
            } else {
                sb_append(&conv->code, "   mulss\t\t%s, %s\n", dst, b);
            }
            break;
        }
        case INST_DIV_FLOAT: {
            if (operand_size == 8) {
                sb_append(&conv->code, "   divsd\t\t%s, %s\n", dst, b);
            } else {
                sb_append(&conv->code, "   divss\t\t%s, %s\n", dst, b);
            }
            break;
        }
        default: {
            XXX;
        }
    }

}

int slot_index_to_stack_offset(int slot) {
    return -((slot + 1) * 8);
}

void x64_emit_cast_instruction(X64Converter *conv, Inst *inst) {
    int from_size = inst->op2.size;
    int to_size   = inst->op1.size;

    char *dst = register_index_to_string(inst->op1.reg, inst->op1.size);
    char *src = register_index_to_string(inst->op2.reg, inst->op2.size);

    switch (inst->kind) {
        case INST_SEXT: {
            if (from_size == 4) {
                sb_append(&conv->code, "   movsxd\t\t%s, %s\n", dst, src);
            } else {
                sb_append(&conv->code, "   movsx\t\t%s, %s\n", dst, src);
            }
            break;
        }
        case INST_ZEXT: {
            if (from_size == 4) {
                char *dst4 = register_index_to_string(inst->op1.reg, 4);
                sb_append(&conv->code, "   mov\t\t%s, %s\n", dst4, src);
            } else {
                sb_append(&conv->code, "   movzx\t\t%s, %s\n", dst, src);
            }
            break;
        }
        case INST_TRUNC: {
            if (inst->op1.reg != inst->op2.reg) {
                if (to_size == 4) {
                    char *src4 = register_index_to_string(inst->op2.reg, 4);
                    sb_append(&conv->code, "   mov\t\t%s, %s\n", dst, src4);
                } else {
                    char *src_matched = register_index_to_string(inst->op2.reg, to_size);
                    sb_append(&conv->code, "   mov\t\t\t%s, %s\n", dst, src_matched);
                }
            }
            break;
        }
        case INST_SITOFP: {
            char *cvt = (to_size == 4) ? "cvtsi2ss" : "cvtsi2sd";
            sb_append(&conv->code, "   %s\t\t%s, %s\n", cvt, dst, src);
            break;
        }
        case INST_UITOFP: {
            char *cvt = (to_size == 4) ? "cvtsi2ss" : "cvtsi2sd";
            if (from_size < 8) {
                sb_append(&conv->code, "   %s\t\t%s, %s\n", cvt, dst, src);
            } else {
                XXX;
            }
            break;
        }
        case INST_FPTOSI: {
            char *cvt = (from_size == 4) ? "cvttss2si" : "cvttsd2si";
            sb_append(&conv->code, "   %s\t\t%s, %s\n", cvt, dst, src);
            break;
        }
        case INST_FPTOUI: {
            if (to_size == 8) {
                XXX;
            }
            char *cvt = (from_size == 4) ? "cvttss2si" : "cvttsd2si";
            sb_append(&conv->code, "   %s\t\t%s, %s\n", cvt, dst, src);
            break;
        }
        case INST_FPEXT: {
            sb_append(&conv->code, "   cvtss2sd\t%s, %s\n", dst, src);
            break;
        }
        case INST_FPTRUNC: {
            sb_append(&conv->code, "   cvtsd2ss\t%s, %s\n", dst, src);
            break;
        }
        case INST_BITCAST: {
            XXX;
            break;
        }

        default: {
            XXX;
            break;
        }
    }
}

void x64_load_constant(X64Converter *conv, int dst_reg, Constant *constant) {
    switch (constant->kind) {
    case CONSTANT_STRING: {
        char *dst = register_index_to_string(dst_reg, 8);
        sb_append(&conv->code, "\tmov\t\t%s, STRING_%d\n", dst, constant->id);
        break;
    }
    case CONSTANT_FLOAT: {
        char *dst = sse_register_index_to_string(dst_reg, 8);
        assert(constant->type->kind == TYPE_FLOAT);
        sb_append(&conv->code, "\t%s\t\t%s, [rel FLOAT_%d]\n", constant->type->size == 8 ? "movsd" : "movss", dst, constant->id);
        break;
    }
    default: {
        XXX;
    }
    }
}

void x64_emit_instruction(X64Converter *conv, Inst *inst) {
    Operand *op1 = &inst->op1;
    Operand *op2 = &inst->op2;

    switch (inst->kind) {
    case INST_NOOP: {
        break;
    }
    case INST_MOV: {
        if (op1->kind == OPERAND_REG && op2->kind == OPERAND_REG) {
            char *dst = get_register_string(*op1);
            char *a = get_register_string(*op2);
            char *mov_string = get_mov_string(*op1);

            sb_append(&conv->code, "\t%s\t\t%s, %s\n", mov_string, dst, a);
            return;
        }
        else if (op1->kind == OPERAND_REG && op2->kind == OPERAND_IMM_INT) {
            char *dst = register_index_to_string(op1->reg, 8);
            sb_append(&conv->code, "\tmov\t\t%s, %lld\n", dst, op2->imm_int);
        }
        else if (op1->kind == OPERAND_REG && op2->kind == OPERAND_BIG_CONSTANT) {
            x64_load_constant(conv, op1->reg, op2->constant);
        }
        else {
            printf("Compiler Error: Unhandled INST_MOV instruction. Op1 = '%s', Op2 = '%s''\n", operand_kind_to_string(op1->kind), operand_kind_to_string(op2->kind));
            XXX;
            return;
        }

        break;
    }
    case INST_LOAD: {
        char *dst = register_index_to_string(op1->reg, 8);
        int stack_offset = slot_index_to_stack_offset(op2->slot);
        sb_append(&conv->code, "\tmov\t\t%s, %d[rbp]\n", dst, stack_offset);
        break;
    }
    case INST_LOADF: {
        char *dst = sse_register_index_to_string(op1->reg, 8);
        int stack_offset = slot_index_to_stack_offset(op2->slot);
        if (op1->size == 8) {
            sb_append(&conv->code, "\tmovsd\t\t%s, %d[rbp]\n", dst, stack_offset);
        } else {
            sb_append(&conv->code, "\tmovss\t\t%s, %d[rbp]\n", dst, stack_offset);
        }
        break;
    }
    case INST_STORE: {
        char *src = register_index_to_string(op2->reg, 8);
        int stack_offset = slot_index_to_stack_offset(op1->slot);
        sb_append(&conv->code, "\tmov\t\t%d[rbp], %s\n", stack_offset, src);
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
    case INST_DEREF: {
        char *dst = register_index_to_string(op1->reg, 8);
        char *src = register_index_to_string(op2->reg, 8);

        sb_append(&conv->code, "\tmov\t\t%s, [%s]\n", dst, src);
        break;
    }
    case INST_COMMENT: {
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
    case INST_LESS_THAN:
    case INST_GREATER_THAN:
    case INST_GREATER_THAN_EQUAL:
    case INST_LESS_THAN_EQUAL:
    case INST_DOUBLE_EQUAL: {
        x64_emit_comparrison_instruction(conv, inst);
        break;
    }
    case INST_SEXT:
    case INST_ZEXT:
    case INST_TRUNC:
    case INST_SITOFP:
    case INST_UITOFP:
    case INST_FPTOSI:
    case INST_FPTOUI:
    case INST_FPEXT:
    case INST_FPTRUNC:
    case INST_BITCAST: {
        x64_emit_cast_instruction(conv, inst);
        break;
    }

    default: {
        printf("Compiler Error: Unhandled instruction '%s'\n", inst_kind_to_string(inst->kind));
        XXX;
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