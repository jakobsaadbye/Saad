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
void x64_code(X64Converter *conv, char *inst, char *format, ...);



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

                sb_append(&conv->data, "\t_data_%d DB `%s`, 0\n", str_data_id, constant->as.value_string.data);
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

static char *gpr_scatch_register[] = {
    "rax",
    "r13",
    "r14",
    "r15",
};

static char *xmm_scratch_registers[] = {
    "xmm8",
    "xmm9",
    "xmm10",
    "xmm11",
};

Register gpr_register_index_to_x86_register(int reg_index) {
    Register gpr_x86_registers[] = {
        REG_RBX,
        REG_RCX,
        REG_R8,
        REG_R9,
        REG_R10,
        REG_R11,
        REG_R12,
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
        sb_append(&conv->code_header, "   extern %s\n", func->symbol_name);    
        return;
    }

    conv->current_bytecode_function = func;

    // Compute frame size
    x64_compute_function_stack_frame(conv, func);

    //
    // Prolog
    //
    sb_append(&conv->code, "%s:\n", func->symbol_name);
    x64_code(conv, "push", "rbp");
    x64_code(conv, "mov", "rbp, rsp");
    x64_code(conv, "sub", "rsp, %d", func->local_stack_size);

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
                    x64_code(conv, "mov" "rax, %s", register_to_str(input_reg, 8));
                }  
            }
        } else {
            // XXX;
        }

        if (param->type->size <= 8) {
            x64_code(conv, "mov", "%d[rbp], %s", param_offset, register_to_str(input_reg, param->type->size));
        } else {
            XXX;
        }

    }


    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);
        x64_emit_basic_block(conv, bb);
    }

    //
    // Epilog
    //
    x64_code(conv, "mov", "rax, 0");
    x64_code(conv, "add", "rsp, %d", func->local_stack_size);
    x64_code(conv, "pop", "rbp");
    x64_code(conv, "ret", "");
}

Register x64_get_gpr_argument_register_from_index(int index) {
    if (index == 0) return REG_RCX;
    if (index == 1) return REG_RDX;
    if (index == 2) return REG_R8;
    if (index == 3) return REG_R9;
    return REG_NONE;
}

Register x64_get_xmm_argument_register_from_index(int index) {
    if (index == 0) return REG_XMM0;
    if (index == 1) return REG_XMM1;
    if (index == 2) return REG_XMM2;
    if (index == 3) return REG_XMM3;
    return REG_NONE;
}

typedef struct ArgMove {
    char *dst;
    char *src;
    bool  src_is_sse;   // true => emit movq (bitcast) instead of mov
    bool  done;
} ArgMove;

void x64_sequence_moves(X64Converter *conv, ArgMove *moves, int count, bool is_xmm_pool) {
    bool pending = true;
    while (pending) {
        pending = false;
        bool progressed = false;

        for (int i = 0; i < count; i++) {
            if (moves[i].done) continue;
            pending = true;

            bool needed_elsewhere = false;
            for (int j = 0; j < count; j++) {
                if (j == i || moves[j].done) continue;
                if (strcmp(moves[j].src, moves[i].dst) == 0) { needed_elsewhere = true; break; }
            }

            if (!needed_elsewhere) {
                if (strcmp(moves[i].dst, moves[i].src) != 0) {
                    if (moves[i].src_is_sse)
                        x64_code(conv, "movq", "%s, %s", moves[i].dst, moves[i].src);
                    else
                        x64_code(conv, is_xmm_pool ? "movaps" : "mov", "%s, %s", moves[i].dst, moves[i].src);
                }
                moves[i].done = true;
                progressed = true;
            }
        }

        if (pending && !progressed) {
            for (int i = 0; i < count; i++) {
                if (moves[i].done) continue;
                char *scratch = is_xmm_pool ? xmm_scratch_registers[i] : gpr_scatch_register[i];

                x64_code(conv, is_xmm_pool ? "movaps" : "mov", "%s, %s", scratch, moves[i].dst);
                for (int j = 0; j < count; j++) {
                    if (j == i || moves[j].done) continue;
                    if (strcmp(moves[j].src, moves[i].dst) == 0) moves[j].src = scratch;
                }
                break;
            }
        }
    }
}

void x64_emit_function_call(X64Converter *conv, Inst *inst) {
    InstFunctionCall *call = inst->data;

    ArgMove gpr_moves[4] = {0};
    ArgMove xmm_moves[4] = {0};
    int gpr_count = 0;
    int xmm_count = 0;

    for (int i = 0; i < call->arguments.count; i++) {
        CallArgument *arg = da_get(call->arguments, i);

        bool arg_is_float = arg->type->kind == TYPE_FLOAT;

        // @Cleanup: This could just be one call
        Register gpr_reg = x64_get_gpr_argument_register_from_index(i);
        Register xmm_reg  = x64_get_xmm_argument_register_from_index(i);

        if (gpr_reg == REG_NONE) {
            // 5th+ arg: goes on the stack
            int stack_offset = 32 + (i - 4) * 8;
            if (arg_is_float) {
                char *src = sse_register_index_to_string(arg->vreg, 8);

                if (arg->is_vararg && arg->type->size == 4) {
                    char *scratch = xmm_scratch_registers[i % 4];
                    x64_code(conv, "cvtss2sd", "%s, %s", scratch, src);
                    src = scratch;
                }

                x64_code(conv, "movsd", "[rsp+%d], %s", stack_offset, src);
            } else {
                char *src = register_index_to_string(arg->vreg, 8);
                x64_code(conv, "mov", "[rsp+%d], %s", stack_offset, src);
            }
            continue;
        }

        if (arg_is_float) {
            char *src = sse_register_index_to_string(arg->vreg, 8);
            xmm_moves[xmm_count++] = (ArgMove) { 
                .dst = register_to_str(xmm_reg, 8), 
                .src = src 
            };

            if (arg->is_vararg) {

                if (arg->type->size == 4) {
                    // Promote f32 -> f64
                    char *xmm_scratch_reg = xmm_scratch_registers[i];
                    x64_code(conv, "cvtss2sd", "%s, %s", xmm_scratch_reg, src);
                    src = xmm_scratch_reg;
                }

                // duplicate raw bits into the positional GPR, scheduled with the rest
                gpr_moves[gpr_count++] = (ArgMove){
                    .dst = register_to_str(gpr_reg, 8),
                    .src = src,
                    .src_is_sse = true
                };
            }
        } else {
            char *src = register_index_to_string(arg->vreg, 8);
            gpr_moves[gpr_count++] = (ArgMove) { 
                .dst = register_to_str(gpr_reg, 8), 
                .src = src 
            };
        }
    }

    x64_sequence_moves(conv, gpr_moves, gpr_count, false);
    x64_sequence_moves(conv, xmm_moves, xmm_count, true);

    x64_code(conv, "call", "%s", inst->op2.label);
}

void x64_emit_terminator(X64Converter *conv, BasicBlock *bb) {
    switch (bb->terminator.kind) {
        case TERMINATOR_NONE: break;
        case TERMINATOR_JUMP: {
            x64_code(conv, "   jmp", "L%d", bb->terminator.target1->id);
            break;
        }
        case TERMINATOR_COND_JUMP: {
            char *cmp_reg = register_index_to_string(bb->terminator.condition_reg, 1);
            x64_code(conv, "   cmp", "%s, 0", cmp_reg);
            x64_code(conv, "   jz", "L%d", bb->terminator.target2->id);
            x64_code(conv, "   jmp", "L%d", bb->terminator.target1->id);
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

    bool do_signed_comparison = true;

    if (do_signed_comparison) {
        char *set_inst = "";
        if (inst->kind == INST_LESS_THAN)            set_inst = "setl";
        if (inst->kind == INST_GREATER_THAN)         set_inst = "setg";
        if (inst->kind == INST_GREATER_THAN_EQUAL)   set_inst = "setge";
        if (inst->kind == INST_LESS_THAN_EQUAL)      set_inst = "setle";
        if (inst->kind == INST_DOUBLE_EQUAL)         set_inst = "sete";
        if (inst->kind == INST_NOT_EQUAL)            set_inst = "setne";

        x64_code(conv, "   cmp", "%s, %s", a, b);
        x64_code(conv, "   %s", "%s", set_inst, dst);

        return;
    }

}

char *get_mov_string(Operand op) {
    if (op.is_sse) {
        return op.size == 8 ? "movsd" : "movss";
    }
    return "mov";
}

void x64_emit_mov_into(X64Converter *conv, Operand dst_op, Operand src_op) {
    if (dst_op.is_sse) {
        char *inst = dst_op.size == 8 ? "movsd" : "movss";
        char *dst = get_register_string(dst_op);
        char *src = get_register_string(src_op);
        x64_code(conv, inst, "%s, %s", dst, src);
        return;
    }

    if (dst_op.size < 4) {
        // dst must be WIDER than src for movzx/movsx — always widen dst to 8 bytes
        char *inst  = is_unsigned_integer_ish(dst_op.type) ? "movzx" : "movsx";
        char *dst8 = get_register_string_sized(dst_op, 8);
        char *src_narrow = get_register_string_sized(src_op, dst_op.size);
        x64_code(conv, inst, "%s, %s", dst8, src_narrow);
        return;
    }

    // size 4 or 8: plain mov, BOTH operands at the same natural width
    char *dst = get_register_string_sized(dst_op, dst_op.size);
    char *src = get_register_string_sized(src_op, dst_op.size);
    x64_code(conv, "mov", "%s, %s", dst, src);
}

void x64_emit_arithmetic_instruction(X64Converter *conv, Inst *inst) {
    int operand_size = inst->op2.size;

    Operand dst_op = inst->op1;

    if (!dst_op.is_sse && dst_op.size < 4) {
        dst_op.size = 8;
    }

    char *dst = get_register_string(dst_op);
    char *a   = get_register_string(inst->op2);
    char *b   = get_register_string(inst->op3);

    if (a != dst) {
        x64_emit_mov_into(conv, dst_op, inst->op2);
    }

    switch(inst->kind) {
        case INST_ADD_INT: {
            x64_code(conv, "add", "%s, %s", dst, b);
            break;
        }
        case INST_SUB_INT: {
            x64_code(conv, "sub", "%s, %s", dst, b);
            break;
        }
        case INST_MUL_INT: {
            x64_code(conv, "imul", "%s, %s", dst, b);
            break;
        }
        case INST_DIV_INT: {
            bool is_signed = is_signed_integer(inst->op1.type);
            int size = dst_op.size;

            char *rax_str = register_to_str(REG_RAX, size);
            char *rdx_str = register_to_str(REG_RDX, size);

            x64_code(conv, "mov", "%s, %s", rax_str, dst);

            if (is_signed) {
                x64_code(conv, size == 8 ? "cqo" : "cdq", "");
            } else {
                x64_code(conv, "xor", "%s, %s", rdx_str, rdx_str);
            }

            x64_code(conv, is_signed ? "idiv" : "div", "%s", b);
            x64_code(conv, "mov", "%s, %s", dst, rax_str);

            break;
        }
        case INST_ADD_FLOAT: {
            if (operand_size == 8) {
                x64_code(conv, "addsd", "%s, %s", dst, b);
            } else {
                x64_code(conv, "addss", "%s, %s", dst, b);
            }
            break;
        }
        case INST_SUB_FLOAT: {
            if (operand_size == 8) {
                x64_code(conv, "subsd", "%s, %s", dst, b);
            } else {
                x64_code(conv, "subss", "%s, %s", dst, b);
            }
            break;
        }
        case INST_MUL_FLOAT: {
            if (operand_size == 8) {
                x64_code(conv, "mulsd", "%s, %s", dst, b);
            } else {
                x64_code(conv, "mulss", "%s, %s", dst, b);
            }
            break;
        }
        case INST_DIV_FLOAT: {
            if (operand_size == 8) {
                x64_code(conv, "divsd", "%s, %s", dst, b);
            } else {
                x64_code(conv, "divss", "%s, %s", dst, b);
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

    char *dst = get_register_string(inst->op1);
    char *src = get_register_string(inst->op2);

    switch (inst->kind) {
        case INST_SEXT: {
            if (from_size == 4) {
                x64_code(conv, "movsxd", "%s, %s", dst, src);
            } else {
                x64_code(conv, "movsx", "%s, %s", dst, src);
            }
            break;
        }
        case INST_ZEXT: {
            if (from_size == 4) {
                char *dst4 = get_register_string_sized(inst->op1, 4);
                x64_code(conv, "mov", "%s, %s", dst4, src);
            } else {
                x64_code(conv, "movzx", "%s, %s", dst, src);
            }
            break;
        }
        case INST_TRUNC: {
            if (inst->op1.reg != inst->op2.reg) {
                if (to_size == 4) {
                    char *src4 = get_register_string_sized(inst->op2, 4);
                    x64_code(conv, "mov", "%s, %s", dst, src4);
                } else {
                    char *src_matched = get_register_string_sized(inst->op2, to_size);
                    x64_code(conv, "mov", "%s, %s", dst, src_matched);
                }
            }
            break;
        }
        case INST_SITOFP: {
            char *cvt = (to_size == 4) ? "cvtsi2ss" : "cvtsi2sd";
            x64_code(conv, "%s", "%s, %s", cvt, dst, src);
            break;
        }
        case INST_UITOFP: {
            char *cvt = (to_size == 4) ? "cvtsi2ss" : "cvtsi2sd";
            if (from_size < 8) {
                x64_code(conv, "%s", "%s, %s", cvt, dst, src);
            } else {
                XXX;
            }
            break;
        }
        case INST_FPTOSI: {
            char *cvt = (from_size == 4) ? "cvttss2si" : "cvttsd2si";
            x64_code(conv, "%s", "%s, %s", cvt, dst, src);
            break;
        }
        case INST_FPTOUI: {
            if (to_size == 8) {
                XXX;
            }
            char *cvt = (from_size == 4) ? "cvttss2si" : "cvttsd2si";
            x64_code(conv, "%s", "%s, %s", cvt, dst, src);
            break;
        }
        case INST_FPEXT: {
            x64_code(conv, "cvtss2sd", "%s, %s", dst, src);
            break;
        }
        case INST_FPTRUNC: {
            x64_code(conv, "cvtsd2ss", "%s, %s", dst, src);
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
        x64_code(conv, "mov", "%s, STRING_%d", dst, constant->id);
        break;
    }
    case CONSTANT_FLOAT: {
        char *dst = sse_register_index_to_string(dst_reg, 8);
        assert(constant->type->kind == TYPE_FLOAT);
        x64_code(conv, constant->type->size == 8 ? "movsd" : "movss", "%s, [rel FLOAT_%d]", dst, constant->id);
        break;
    }
    default: {
        XXX;
    }
    }
}

void x64_emit_instruction(X64Converter *conv, Inst *inst) {
    Operand op1 = inst->op1;
    Operand op2 = inst->op2;

    switch (inst->kind) {
    case INST_NOOP: {
        break;
    }
    case INST_MOV: {
        if (op1.kind == OPERAND_REG && op2.kind == OPERAND_REG) {
            x64_emit_mov_into(conv, op1, op2);
            return;
        }
        else if (op1.kind == OPERAND_REG && op2.kind == OPERAND_IMM_INT) {
            char *dst = register_index_to_string(op1.reg, 8);
            x64_code(conv, "mov", "%s, %lld", dst, op2.imm_int);
            return;
        }
        else if (op1.kind == OPERAND_REG && op2.kind == OPERAND_BIG_CONSTANT) {
            x64_load_constant(conv, op1.reg, op2.constant);
            return;
        }
        else {
            printf("Compiler Error: Unhandled INST_MOV instruction. Op1 = '%s', Op2 = '%s''\n", operand_kind_to_string(op1.kind), operand_kind_to_string(op2.kind));
            XXX;
            return;
        }

        break;
    }
    case INST_LOAD: 
    case INST_LOADF: {
        char *dst = get_register_string(op1);
        int stack_offset = slot_index_to_stack_offset(op2.slot);

        if (!op1.is_sse && op1.size < 4) {
            // Widen to be in eax / rax
            char *mov_string = is_unsigned_integer_ish(op1.type) ? "movzx" : "movsx";
            char *dst64 = get_register_string_sized(op1, 8);
            char *width = word_size(op1.type);

            x64_code(conv, mov_string, "%s, %s %d[rbp]", dst64, width, stack_offset);
        } else {
            x64_code(conv, get_mov_string(op1), "%s, %d[rbp]", dst, stack_offset);
        }

        break;
    }
    case INST_STORE:
    case INST_STOREF: {
        int stack_offset = slot_index_to_stack_offset(op1.slot);
        char *src = get_register_string(op2);
        char *mov_string = get_mov_string(op2);
        x64_code(conv, mov_string, "%d[rbp], %s", stack_offset, src);

        break;
    }
    case INST_CALL: {
        x64_emit_function_call(conv, inst);
        break;
    }
    case INST_PUSH: {
        break;
    }
    case INST_POP: {
        break;
    }
    case INST_LABEL: {
        x64_code(conv, "%s:", inst->op1.label);
        break;
    }
    case INST_RETURN: {
        break;
    }
    case INST_DEREF: {
        char *dst = register_index_to_string(op1.reg, 8);
        char *src = register_index_to_string(op2.reg, 8);

        x64_code(conv, "mov", "%s, [%s]", dst, src);
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

void x64_code(X64Converter *conv, char *inst, char *format, ...) {
    va_list args;
    va_start(args, format);

    int spacing = 12;

    sb_append(&conv->code, "\t%-*s", spacing, inst);
    if (strlen(format) > 0) {
        sb_vappend(&conv->code, format, args);
    }
    sb_append(&conv->code, "\n");

    va_end(args);
}