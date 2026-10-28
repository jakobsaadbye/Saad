//////////////////////////////////////////////////////////////////////////////////////////
//
// x64 backend (Win64 ABI, NASM syntax)
//
// Register usage
// --------------
//   rsp, rbp           stack and frame pointer
//   rax, rcx, rdx      reserved: return values, div/idiv, shift counts, call setup
//   r10, r11           reserved: scratch for spilled operands and parallel moves
//   xmm14, xmm15       reserved: scratch for spilled float operands
//   everything else    allocatable (see x64_target)
//
// Frame layout (rbp is 16 byte aligned after `push rbp`)
// -----------------------------------------------------
//   [rbp + 16 + 8*i]   incoming argument i (i >= 4 is passed on the stack, 0..3 is shadow space)
//   [rbp + 8]          return address
//   [rbp]              saved rbp
//   [rbp - ...]        saved callee-saved xmm registers (16 bytes each, aligned)
//                      saved callee-saved gprs
//                      IR stack slots
//                      spill slots (8 bytes each)
//   [rsp + 8*i]        outgoing argument i (32 bytes of shadow space + stack arguments)
//
// Frames of 4 KB or more are probed page by page in the prologue (required on Windows).
//
//////////////////////////////////////////////////////////////////////////////////////////

typedef enum X64Reg {
    X64_RAX, X64_RCX, X64_RDX, X64_RBX, X64_RSP, X64_RBP, X64_RSI, X64_RDI,
    X64_R8,  X64_R9,  X64_R10, X64_R11, X64_R12, X64_R13, X64_R14, X64_R15,
    X64_XMM0, X64_XMM1, X64_XMM2,  X64_XMM3,  X64_XMM4,  X64_XMM5,  X64_XMM6,  X64_XMM7,
    X64_XMM8, X64_XMM9, X64_XMM10, X64_XMM11, X64_XMM12, X64_XMM13, X64_XMM14, X64_XMM15,
    X64_REG_COUNT,
    X64_NO_REG = -1,
} X64Reg;

static const char *x64_gpr_names[16][4] = {
    { "rax", "eax",  "ax",   "al"   },
    { "rcx", "ecx",  "cx",   "cl"   },
    { "rdx", "edx",  "dx",   "dl"   },
    { "rbx", "ebx",  "bx",   "bl"   },
    { "rsp", "esp",  "sp",   "spl"  },
    { "rbp", "ebp",  "bp",   "bpl"  },
    { "rsi", "esi",  "si",   "sil"  },
    { "rdi", "edi",  "di",   "dil"  },
    { "r8",  "r8d",  "r8w",  "r8b"  },
    { "r9",  "r9d",  "r9w",  "r9b"  },
    { "r10", "r10d", "r10w", "r10b" },
    { "r11", "r11d", "r11w", "r11b" },
    { "r12", "r12d", "r12w", "r12b" },
    { "r13", "r13d", "r13w", "r13b" },
    { "r14", "r14d", "r14w", "r14b" },
    { "r15", "r15d", "r15w", "r15b" },
};

static const char *x64_xmm_names[16] = {
    "xmm0", "xmm1", "xmm2",  "xmm3",  "xmm4",  "xmm5",  "xmm6",  "xmm7",
    "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
};

static bool x64_is_xmm(int reg) { return reg >= X64_XMM0 && reg <= X64_XMM15; }

// Name of a register at a width in bytes (1, 2, 4, 8). Width is ignored for xmm registers
static const char *x64_reg(int reg, int width) {
    if (x64_is_xmm(reg)) return x64_xmm_names[reg - X64_XMM0];
    switch (width) {
    case 1:  return x64_gpr_names[reg][3];
    case 2:  return x64_gpr_names[reg][2];
    case 4:  return x64_gpr_names[reg][1];
    default: return x64_gpr_names[reg][0];
    }
}

static const char *x64_register_name(int reg) { return x64_reg(reg, 8); }

static const int x64_gpr_arg_regs[4] = { X64_RCX, X64_RDX, X64_R8, X64_R9 };

TargetDesc x64_target(void) {
    TargetDesc target = {0};
    target.name = "x64-win64";
    target.register_name = x64_register_name;

    // Caller-saved registers first, so short lived values don't force a callee-saved register save
    int gprs[] = { X64_R8, X64_R9, X64_RBX, X64_RSI, X64_RDI, X64_R12, X64_R13, X64_R14, X64_R15 };
    for (int i = 0; i < (int)(sizeof(gprs) / sizeof(gprs[0])); i++) {
        target.allocatable[REG_CLASS_GPR][target.allocatable_count[REG_CLASS_GPR]++] = gprs[i];
    }
    for (int r = X64_XMM0; r <= X64_XMM13; r++) {
        target.allocatable[REG_CLASS_FPR][target.allocatable_count[REG_CLASS_FPR]++] = r;
    }

    int callee_saved[] = { X64_RBX, X64_RBP, X64_RSI, X64_RDI, X64_R12, X64_R13, X64_R14, X64_R15 };
    for (int i = 0; i < (int)(sizeof(callee_saved) / sizeof(callee_saved[0])); i++) target.is_callee_saved[callee_saved[i]] = true;
    for (int r = X64_XMM6; r <= X64_XMM15; r++) target.is_callee_saved[r] = true;

    return target;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Emitter state
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct X64FloatConstant {
    uint64_t bits;
    int      size;
    int      id;
} X64FloatConstant;

typedef struct X64Emitter {
    IrModule       *module;
    TargetDesc      target;
    StringBuilder   text;
    StringBuilder   regalloc_dump;
    IrArray(X64FloatConstant) float_constants;

    // Per function
    IrFunction     *func;
    RegAllocResult  ra;
    int            *slot_offsets;   // rbp relative offset of every IR slot
    int             spill_base;     // rbp relative offset of spill slot 0
    int             frame_size;
    int             saved_gpr_offset[X64_REG_COUNT];
    int             next_local_label;
    IrBlock        *next_block;     // Block emitted after the current one (for fall-through)
} X64Emitter;

static void x64_line(X64Emitter *x, const char *mnemonic, const char *format, ...) {
    sb_append(&x->text, "\t%-12s", mnemonic);
    if (format && format[0]) {
        va_list args;
        va_start(args, format);
        sb_vappend(&x->text, format, args);
        va_end(args);
    }
    sb_append(&x->text, "\n");
}

static void x64_label(X64Emitter *x, const char *format, ...) {
    va_list args;
    va_start(args, format);
    sb_vappend(&x->text, format, args);
    va_end(args);
    sb_append(&x->text, ":\n");
}

static const char *x64_symbol(X64Emitter *x, int global) {
    // '$' makes NASM treat the name as an identifier even if it collides with a keyword (e.g a function named 'add')
    static char buffers[4][160];
    static int  next = 0;
    char *buffer = buffers[next++ & 3];
    snprintf(buffer, 160, "$%s", x->module->globals.items[global]->name);
    return buffer;
}

static int x64_float_constant(X64Emitter *x, double value, int size) {
    uint64_t bits = 0;
    if (size == 4) {
        float f = (float) value;
        uint32_t b32;
        memcpy(&b32, &f, 4);
        bits = b32;
    } else {
        memcpy(&bits, &value, 8);
    }
    for (int i = 0; i < x->float_constants.count; i++) {
        if (x->float_constants.items[i].bits == bits && x->float_constants.items[i].size == size) return x->float_constants.items[i].id;
    }
    X64FloatConstant c = { bits, size, x->float_constants.count };
    ir_array_push(x->float_constants, c);
    return c.id;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Operand access
//////////////////////////////////////////////////////////////////////////////////////////

static Location x64_loc(X64Emitter *x, int vreg) {
    return x->ra.locations[vreg];
}

static int x64_spill_offset(X64Emitter *x, int spill_slot) {
    return x->spill_base - spill_slot * 8;
}

static IrType x64_vtype(X64Emitter *x, int vreg) {
    return ir_vreg_type(x->func, vreg);
}

static int x64_width(IrType type) {
    int size = ir_type_size(type);
    return size > 0 ? size : 8;
}

static const char *x64_size_keyword(int width) {
    switch (width) {
    case 1:  return "byte";
    case 2:  return "word";
    case 4:  return "dword";
    default: return "qword";
    }
}

// Returns the register holding 'vreg', loading it into 'scratch' if it is spilled
static int x64_use(X64Emitter *x, int vreg, int scratch) {
    Location loc = x64_loc(x, vreg);
    if (loc.kind == LOCATION_REG) return loc.reg;
    assert(loc.kind == LOCATION_SPILL);
    int offset = x64_spill_offset(x, loc.spill_slot);
    if (x64_is_xmm(scratch)) {
        x64_line(x, "movsd", "%s, qword [rbp%+d]", x64_reg(scratch, 8), offset);
    } else {
        x64_line(x, "mov", "%s, qword [rbp%+d]", x64_reg(scratch, 8), offset);
    }
    return scratch;
}

// Returns the register an instruction should write 'vreg' to. Call x64_finish_def afterwards
static int x64_def(X64Emitter *x, int vreg, int scratch) {
    Location loc = x64_loc(x, vreg);
    if (loc.kind == LOCATION_REG) return loc.reg;
    return scratch;
}

static void x64_finish_def(X64Emitter *x, int vreg, int reg) {
    Location loc = x64_loc(x, vreg);
    if (loc.kind != LOCATION_SPILL) return;
    int offset = x64_spill_offset(x, loc.spill_slot);
    if (x64_is_xmm(reg)) {
        x64_line(x, "movsd", "qword [rbp%+d], %s", offset, x64_reg(reg, 8));
    } else {
        x64_line(x, "mov", "qword [rbp%+d], %s", offset, x64_reg(reg, 8));
    }
}

// Full register copy (upper bits of narrow values are don't-care)
static void x64_move_reg(X64Emitter *x, int dst, int src) {
    if (dst == src) return;
    if (x64_is_xmm(dst) && x64_is_xmm(src))       x64_line(x, "movaps", "%s, %s", x64_reg(dst, 8), x64_reg(src, 8));
    else if (x64_is_xmm(dst))                     x64_line(x, "movq", "%s, %s", x64_reg(dst, 8), x64_reg(src, 8));
    else if (x64_is_xmm(src))                     x64_line(x, "movq", "%s, %s", x64_reg(dst, 8), x64_reg(src, 8));
    else                                          x64_line(x, "mov", "%s, %s", x64_reg(dst, 8), x64_reg(src, 8));
}

// Formats the memory operand of a LOAD / STORE. 'scratch' is used if the base vreg is spilled
static void x64_address(X64Emitter *x, IrAddr addr, int scratch, char *out, int out_size) {
    switch (addr.kind) {
    case IR_ADDR_SLOT:
        snprintf(out, out_size, "[rbp%+d]", x->slot_offsets[addr.base] + addr.offset);
        return;
    case IR_ADDR_GLOBAL:
        if (addr.offset) snprintf(out, out_size, "[rel %s%+d]", x64_symbol(x, addr.base), addr.offset);
        else             snprintf(out, out_size, "[rel %s]", x64_symbol(x, addr.base));
        return;
    case IR_ADDR_VREG: {
        int base = x64_use(x, addr.base, scratch);
        if (addr.offset) snprintf(out, out_size, "[%s%+d]", x64_reg(base, 8), addr.offset);
        else             snprintf(out, out_size, "[%s]", x64_reg(base, 8));
        return;
    }
    default:
        assert(false);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Parallel moves (parameters, call arguments, memcpy/memset arguments)
//////////////////////////////////////////////////////////////////////////////////////////

typedef enum MoveOperandKind {
    MOVE_REG,
    MOVE_MEM,   // [base + offset], base is rbp or rsp
    MOVE_IMM,
} MoveOperandKind;

typedef struct MoveOperand {
    MoveOperandKind kind;
    int             reg;      // MOVE_REG, or the base register for MOVE_MEM
    int             offset;
    int64_t         imm;
} MoveOperand;

typedef struct Move {
    MoveOperand dst;
    MoveOperand src;
    bool        done;
} Move;

static MoveOperand move_reg(int reg)                { return (MoveOperand){ MOVE_REG, reg, 0, 0 }; }
static MoveOperand move_mem(int base, int offset)   { return (MoveOperand){ MOVE_MEM, base, offset, 0 }; }
static MoveOperand move_imm(int64_t imm)            { return (MoveOperand){ MOVE_IMM, X64_NO_REG, 0, imm }; }

static MoveOperand move_vreg(X64Emitter *x, int vreg) {
    Location loc = x64_loc(x, vreg);
    if (loc.kind == LOCATION_REG) return move_reg(loc.reg);
    return move_mem(X64_RBP, x64_spill_offset(x, loc.spill_slot));
}

static void x64_emit_move(X64Emitter *x, MoveOperand dst, MoveOperand src) {
    if (dst.kind == MOVE_REG && src.kind == MOVE_REG) {
        x64_move_reg(x, dst.reg, src.reg);
        return;
    }
    if (dst.kind == MOVE_REG && src.kind == MOVE_MEM) {
        x64_line(x, x64_is_xmm(dst.reg) ? "movsd" : "mov", "%s, qword [%s%+d]", x64_reg(dst.reg, 8), x64_reg(src.reg, 8), src.offset);
        return;
    }
    if (dst.kind == MOVE_REG && src.kind == MOVE_IMM) {
        assert(!x64_is_xmm(dst.reg));
        x64_line(x, "mov", "%s, %lld", x64_reg(dst.reg, 8), (long long) src.imm);
        return;
    }
    if (dst.kind == MOVE_MEM && src.kind == MOVE_REG) {
        x64_line(x, x64_is_xmm(src.reg) ? "movsd" : "mov", "qword [%s%+d], %s", x64_reg(dst.reg, 8), dst.offset, x64_reg(src.reg, 8));
        return;
    }
    // memory/immediate -> memory goes through r11
    x64_emit_move(x, move_reg(X64_R11), src);
    x64_emit_move(x, dst, move_reg(X64_R11));
}

static bool move_reads_reg(MoveOperand src, int reg) {
    return src.kind == MOVE_REG && src.reg == reg;
}

// Performs all moves as if they happened at the same time
static void x64_parallel_move(X64Emitter *x, Move *moves, int count) {
    // 1. Moves into memory first: they only read registers, so no pending move can be clobbered
    for (int i = 0; i < count; i++) {
        if (moves[i].dst.kind == MOVE_MEM) {
            x64_emit_move(x, moves[i].dst, moves[i].src);
            moves[i].done = true;
        }
    }

    // 2. Register destinations. A move is ready once no other pending move reads its destination
    while (true) {
        bool pending = false, progress = false;
        for (int i = 0; i < count; i++) {
            if (moves[i].done) continue;
            if (moves[i].src.kind == MOVE_REG && moves[i].src.reg == moves[i].dst.reg) { moves[i].done = true; continue; }
            pending = true;

            bool blocked = false;
            for (int j = 0; j < count; j++) {
                if (j != i && !moves[j].done && move_reads_reg(moves[j].src, moves[i].dst.reg)) { blocked = true; break; }
            }
            if (!blocked) {
                x64_emit_move(x, moves[i].dst, moves[i].src);
                moves[i].done = true;
                progress = true;
            }
        }
        if (!pending) break;
        if (progress) continue;

        // Only cycles of register to register moves remain. Break one by saving a source in a scratch register
        for (int i = 0; i < count; i++) {
            if (moves[i].done) continue;
            int reg = moves[i].src.reg;
            int scratch = x64_is_xmm(reg) ? X64_XMM15 : X64_R11;
            x64_move_reg(x, scratch, reg);
            for (int j = 0; j < count; j++) {
                if (!moves[j].done && move_reads_reg(moves[j].src, reg)) moves[j].src = move_reg(scratch);
            }
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Calls
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct X64CallArg {
    MoveOperand src;
    bool        is_float;
    bool        is_vararg;
} X64CallArg;

static void x64_emit_call_with_args(X64Emitter *x, X64CallArg *args, int count, const char *target) {
    Move moves[64];
    int  move_count = 0;
    assert(count * 2 <= 64);

    for (int i = 0; i < count; i++) {
        if (i < 4) {
            if (args[i].is_float) {
                moves[move_count++] = (Move){ move_reg(X64_XMM0 + i), args[i].src, false };
                // Win64: floats passed through '...' are also passed in the matching integer register
                if (args[i].is_vararg) moves[move_count++] = (Move){ move_reg(x64_gpr_arg_regs[i]), args[i].src, false };
            } else {
                moves[move_count++] = (Move){ move_reg(x64_gpr_arg_regs[i]), args[i].src, false };
            }
        } else {
            moves[move_count++] = (Move){ move_mem(X64_RSP, 8 * i), args[i].src, false };
        }
    }

    x64_parallel_move(x, moves, move_count);
    x64_line(x, "call", "%s", target);
}

static void x64_emit_call(X64Emitter *x, IrInst *inst) {
    IrCall *call = inst->call;

    // An indirect callee goes into rax first: rax is never an argument register
    if (call->callee_kind == IR_CALLEE_VREG) {
        x64_emit_move(x, move_reg(X64_RAX), move_vreg(x, call->callee));
    }

    X64CallArg args[32];
    assert(call->arg_count <= 32);
    for (int i = 0; i < call->arg_count; i++) {
        int vreg = call->args[i];
        args[i].src       = move_vreg(x, vreg);
        args[i].is_float  = ir_type_is_float(x64_vtype(x, vreg));
        args[i].is_vararg = call->arg_is_vararg[i];
    }

    x64_emit_call_with_args(x, args, call->arg_count, call->callee_kind == IR_CALLEE_VREG ? "rax" : x64_symbol(x, call->callee));

    if (inst->dst != IR_NO_VREG) {
        int result = ir_type_is_float(call->return_type) ? X64_XMM0 : X64_RAX;
        x64_emit_move(x, move_vreg(x, inst->dst), move_reg(result));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Instructions
//////////////////////////////////////////////////////////////////////////////////////////

static const char *x64_icmp_cc(IrCmp cmp) {
    switch (cmp) {
    case IR_CMP_EQ:  return "e";
    case IR_CMP_NE:  return "ne";
    case IR_CMP_SLT: return "l";
    case IR_CMP_SLE: return "le";
    case IR_CMP_SGT: return "g";
    case IR_CMP_SGE: return "ge";
    case IR_CMP_ULT: return "b";
    case IR_CMP_ULE: return "be";
    case IR_CMP_UGT: return "a";
    case IR_CMP_UGE: return "ae";
    default: assert(false); return "?";
    }
}

static void x64_emit_int_binop(X64Emitter *x, IrInst *inst) {
    int width = x64_width(inst->type);
    if (width < 4) width = 4; // Narrow ops run at 32 bits, only the low bits matter

    int a = x64_use(x, inst->a, X64_R10);
    int b = x64_use(x, inst->b, X64_R11);
    int d = x64_def(x, inst->dst, X64_R10);

    const char *op = NULL;
    switch (inst->op) {
    case IR_ADD: op = "add";  break;
    case IR_SUB: op = "sub";  break;
    case IR_MUL: op = "imul"; break;
    case IR_AND: op = "and";  break;
    case IR_OR:  op = "or";   break;
    case IR_XOR: op = "xor";  break;
    default: assert(false);
    }

    // d never shares a register with b (see the regalloc invariant), so this is safe
    if (d != a) x64_line(x, "mov", "%s, %s", x64_reg(d, width), x64_reg(a, width));
    x64_line(x, op, "%s, %s", x64_reg(d, width), x64_reg(b, width));
    x64_finish_def(x, inst->dst, d);
}

// Moves 'src' into 'dst' extended to (at least) 32 bits
static void x64_extend_to_32(X64Emitter *x, int dst, int src, int width, bool is_signed) {
    if (width == 1 || width == 2) {
        x64_line(x, is_signed ? "movsx" : "movzx", "%s, %s", x64_reg(dst, 4), x64_reg(src, width));
    } else if (dst != src) {
        x64_line(x, "mov", "%s, %s", x64_reg(dst, width), x64_reg(src, width));
    }
}

static void x64_emit_divide(X64Emitter *x, IrInst *inst) {
    bool is_signed = inst->op == IR_SDIV || inst->op == IR_SREM;
    bool is_rem    = inst->op == IR_SREM || inst->op == IR_UREM;
    int  size      = x64_width(inst->type);
    int  width     = size < 4 ? 4 : size;

    int a = x64_use(x, inst->a, X64_R10);
    x64_extend_to_32(x, X64_RAX, a, size, is_signed);

    int b = x64_use(x, inst->b, X64_R11);
    if (size < 4) {
        x64_extend_to_32(x, X64_R11, b, size, is_signed);
        b = X64_R11;
    }

    if (is_signed) x64_line(x, width == 8 ? "cqo" : "cdq", "");
    else           x64_line(x, "xor", "edx, edx");
    x64_line(x, is_signed ? "idiv" : "div", "%s", x64_reg(b, width));

    int d = x64_def(x, inst->dst, X64_R10);
    x64_line(x, "mov", "%s, %s", x64_reg(d, width), x64_reg(is_rem ? X64_RDX : X64_RAX, width));
    x64_finish_def(x, inst->dst, d);
}

static void x64_emit_shift(X64Emitter *x, IrInst *inst) {
    int size  = x64_width(inst->type);
    int width = size < 4 ? 4 : size;

    int b = x64_use(x, inst->b, X64_R11);
    x64_line(x, "mov", "ecx, %s", x64_reg(b, 4));

    int a = x64_use(x, inst->a, X64_R10);
    int d = x64_def(x, inst->dst, X64_R10);

    const char *op = "shl";
    if (inst->op == IR_LSHR) op = "shr";
    if (inst->op == IR_ASHR) op = "sar";

    if (inst->op == IR_SHL) {
        if (d != a) x64_line(x, "mov", "%s, %s", x64_reg(d, width), x64_reg(a, width));
    } else {
        // Right shifts read the upper bits, so narrow values are extended first
        x64_extend_to_32(x, d, a, size, inst->op == IR_ASHR);
    }
    x64_line(x, op, "%s, cl", x64_reg(d, width));
    x64_finish_def(x, inst->dst, d);
}

static void x64_emit_float_binop(X64Emitter *x, IrInst *inst) {
    bool is_double = inst->type == IR_F64;
    int a = x64_use(x, inst->a, X64_XMM14);
    int b = x64_use(x, inst->b, X64_XMM15);
    int d = x64_def(x, inst->dst, X64_XMM14);

    const char *op = NULL;
    switch (inst->op) {
    case IR_FADD: op = is_double ? "addsd" : "addss"; break;
    case IR_FSUB: op = is_double ? "subsd" : "subss"; break;
    case IR_FMUL: op = is_double ? "mulsd" : "mulss"; break;
    case IR_FDIV: op = is_double ? "divsd" : "divss"; break;
    default: assert(false);
    }

    x64_move_reg(x, d, a);
    x64_line(x, op, "%s, %s", x64_reg(d, 8), x64_reg(b, 8));
    x64_finish_def(x, inst->dst, d);
}

static void x64_emit_fcmp(X64Emitter *x, IrInst *inst) {
    const char *ucomis = inst->type == IR_F64 ? "ucomisd" : "ucomiss";
    int a = x64_use(x, inst->a, X64_XMM14);
    int b = x64_use(x, inst->b, X64_XMM15);
    int d = x64_def(x, inst->dst, X64_R10);
    const char *d8 = x64_reg(d, 1);

    // ucomis sets ZF, PF and CF to 1 when the operands are unordered (NaN). The predicates below
    // are chosen so that the unordered case gives the right answer
    switch (inst->cmp) {
    case IR_CMP_OEQ:
        x64_line(x, ucomis, "%s, %s", x64_reg(a, 8), x64_reg(b, 8));
        x64_line(x, "sete", "%s", d8);
        x64_line(x, "setnp", "al");
        x64_line(x, "and", "%s, al", d8);
        break;
    case IR_CMP_UNE:
        x64_line(x, ucomis, "%s, %s", x64_reg(a, 8), x64_reg(b, 8));
        x64_line(x, "setne", "%s", d8);
        x64_line(x, "setp", "al");
        x64_line(x, "or", "%s, al", d8);
        break;
    case IR_CMP_OGT: x64_line(x, ucomis, "%s, %s", x64_reg(a, 8), x64_reg(b, 8)); x64_line(x, "seta",  "%s", d8); break;
    case IR_CMP_OGE: x64_line(x, ucomis, "%s, %s", x64_reg(a, 8), x64_reg(b, 8)); x64_line(x, "setae", "%s", d8); break;
    case IR_CMP_OLT: x64_line(x, ucomis, "%s, %s", x64_reg(b, 8), x64_reg(a, 8)); x64_line(x, "seta",  "%s", d8); break;
    case IR_CMP_OLE: x64_line(x, ucomis, "%s, %s", x64_reg(b, 8), x64_reg(a, 8)); x64_line(x, "setae", "%s", d8); break;
    default: assert(false);
    }
    x64_finish_def(x, inst->dst, d);
}

static void x64_emit_conversion(X64Emitter *x, IrInst *inst) {
    IrType from = x64_vtype(x, inst->a);
    IrType to   = inst->type;
    int fs = x64_width(from), ts = x64_width(to);

    bool src_is_float = ir_type_is_float(from);
    bool dst_is_float = ir_type_is_float(to);

    int a = x64_use(x, inst->a, src_is_float ? X64_XMM14 : X64_R10);
    int d = x64_def(x, inst->dst, dst_is_float ? X64_XMM14 : X64_R10);

    switch (inst->op) {
    case IR_SEXT:
        if (fs == 4) x64_line(x, "movsxd", "%s, %s", x64_reg(d, 8), x64_reg(a, 4));
        else         x64_line(x, "movsx", "%s, %s", x64_reg(d, ts < 4 ? 4 : ts), x64_reg(a, fs));
        break;
    case IR_ZEXT:
        if (fs == 4) x64_line(x, "mov", "%s, %s", x64_reg(d, 4), x64_reg(a, 4)); // 32-bit writes zero the upper half
        else         x64_line(x, "movzx", "%s, %s", x64_reg(d, 4), x64_reg(a, fs));
        break;
    case IR_TRUNC:
        // Only the low bits of narrow values are defined, so truncation is a plain copy
        if (d != a) x64_line(x, "mov", "%s, %s", x64_reg(d, 4), x64_reg(a, 4));
        break;
    case IR_SITOFP:
    case IR_UITOFP: {
        const char *cvt = to == IR_F64 ? "cvtsi2sd" : "cvtsi2ss";
        bool is_signed = inst->op == IR_SITOFP;
        if (fs < 4) {
            x64_line(x, is_signed ? "movsx" : "movzx", "r11d, %s", x64_reg(a, fs));
            x64_line(x, cvt, "%s, r11d", x64_reg(d, 8));
        } else if (fs == 4 && !is_signed) {
            x64_line(x, "mov", "r11d, %s", x64_reg(a, 4)); // zero extend, then convert as a (positive) 64-bit value
            x64_line(x, cvt, "%s, r11", x64_reg(d, 8));
        } else if (fs == 8 && !is_signed) {
            // u64 -> float: values >= 2^63 are halved (keeping the lowest bit for rounding), converted and doubled
            int big  = x->next_local_label++;
            int done = x->next_local_label++;
            x64_line(x, "test", "%s, %s", x64_reg(a, 8), x64_reg(a, 8));
            x64_line(x, "js", ".Lx%d", big);
            x64_line(x, cvt, "%s, %s", x64_reg(d, 8), x64_reg(a, 8));
            x64_line(x, "jmp", ".Lx%d", done);
            x64_label(x, ".Lx%d", big);
            x64_line(x, "mov", "r11, %s", x64_reg(a, 8));
            x64_line(x, "shr", "r11, 1");
            x64_line(x, "mov", "rax, %s", x64_reg(a, 8));
            x64_line(x, "and", "eax, 1");
            x64_line(x, "or", "r11, rax");
            x64_line(x, cvt, "%s, r11", x64_reg(d, 8));
            x64_line(x, to == IR_F64 ? "addsd" : "addss", "%s, %s", x64_reg(d, 8), x64_reg(d, 8));
            x64_label(x, ".Lx%d", done);
        } else {
            x64_line(x, cvt, "%s, %s", x64_reg(d, 8), x64_reg(a, fs));
        }
        break;
    }
    case IR_FPTOSI:
    case IR_FPTOUI: {
        const char *cvt = from == IR_F64 ? "cvttsd2si" : "cvttss2si";
        if (inst->op == IR_FPTOUI && ts == 8) {
            // float -> u64: values >= 2^63 are shifted down by 2^63 before converting, then the top bit is set
            int limit = x64_float_constant(x, 9223372036854775808.0, fs);
            int big  = x->next_local_label++;
            int done = x->next_local_label++;
            x64_line(x, from == IR_F64 ? "ucomisd" : "ucomiss", "%s, [rel __float%d]", x64_reg(a, 8), limit);
            x64_line(x, "jae", ".Lx%d", big);
            x64_line(x, cvt, "%s, %s", x64_reg(d, 8), x64_reg(a, 8));
            x64_line(x, "jmp", ".Lx%d", done);
            x64_label(x, ".Lx%d", big);
            x64_line(x, "movaps", "xmm15, %s", x64_reg(a, 8));
            x64_line(x, from == IR_F64 ? "subsd" : "subss", "xmm15, [rel __float%d]", limit);
            x64_line(x, cvt, "%s, xmm15", x64_reg(d, 8));
            x64_line(x, "btc", "%s, 63", x64_reg(d, 8));
            x64_label(x, ".Lx%d", done);
        } else {
            // Unsigned 32-bit results come from a 64-bit conversion so the full range is covered
            int width = (ts == 8 || inst->op == IR_FPTOUI) ? 8 : 4;
            x64_line(x, cvt, "%s, %s", x64_reg(d, width), x64_reg(a, 8));
        }
        break;
    }
    case IR_FPEXT:
        x64_line(x, "cvtss2sd", "%s, %s", x64_reg(d, 8), x64_reg(a, 8));
        break;
    case IR_FPTRUNC:
        x64_line(x, "cvtsd2ss", "%s, %s", x64_reg(d, 8), x64_reg(a, 8));
        break;
    case IR_BITCAST:
        if (src_is_float != dst_is_float) x64_line(x, ts == 8 ? "movq" : "movd", "%s, %s", x64_reg(d, ts), x64_reg(a, fs));
        else                              x64_move_reg(x, d, a);
        break;
    default:
        assert(false);
    }
    x64_finish_def(x, inst->dst, d);
}

static void x64_emit_inst(X64Emitter *x, IrInst *inst) {
    switch (inst->op) {
    case IR_NOP:
        return;

    case IR_CONST: {
        int d = x64_def(x, inst->dst, X64_R10);
        int width = x64_width(inst->type);
        if (inst->imm == 0) {
            x64_line(x, "xor", "%s, %s", x64_reg(d, 4), x64_reg(d, 4));
        } else if (width < 8) {
            x64_line(x, "mov", "%s, %lld", x64_reg(d, 4), (long long)(int32_t) inst->imm);
        } else {
            x64_line(x, "mov", "%s, %lld", x64_reg(d, 8), (long long) inst->imm);
        }
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_CONST_FLOAT: {
        int d = x64_def(x, inst->dst, X64_XMM14);
        if (inst->fimm == 0.0 && !signbit(inst->fimm)) {
            x64_line(x, "xorps", "%s, %s", x64_reg(d, 8), x64_reg(d, 8));
        } else {
            int id = x64_float_constant(x, inst->fimm, ir_type_size(inst->type));
            x64_line(x, inst->type == IR_F64 ? "movsd" : "movss", "%s, [rel __float%d]", x64_reg(d, 8), id);
        }
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_COPY: {
        bool is_float = ir_type_is_float(inst->type);
        int a = x64_use(x, inst->a, is_float ? X64_XMM14 : X64_R10);
        int d = x64_def(x, inst->dst, is_float ? X64_XMM14 : X64_R10);
        x64_move_reg(x, d, a);
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_SLOT_ADDR: {
        int d = x64_def(x, inst->dst, X64_R10);
        x64_line(x, "lea", "%s, [rbp%+d]", x64_reg(d, 8), x->slot_offsets[inst->imm]);
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_GLOBAL_ADDR: {
        int d = x64_def(x, inst->dst, X64_R10);
        x64_line(x, "lea", "%s, [rel %s]", x64_reg(d, 8), x64_symbol(x, (int) inst->imm));
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_LOAD: {
        char addr[128];
        x64_address(x, inst->addr, X64_R11, addr, sizeof(addr));
        int width = x64_width(inst->type);
        if (ir_type_is_float(inst->type)) {
            int d = x64_def(x, inst->dst, X64_XMM14);
            x64_line(x, width == 8 ? "movsd" : "movss", "%s, %s %s", x64_reg(d, 8), x64_size_keyword(width), addr);
            x64_finish_def(x, inst->dst, d);
        } else {
            int d = x64_def(x, inst->dst, X64_R10);
            if (width < 4) x64_line(x, "movzx", "%s, %s %s", x64_reg(d, 4), x64_size_keyword(width), addr);
            else           x64_line(x, "mov", "%s, %s %s", x64_reg(d, width), x64_size_keyword(width), addr);
            x64_finish_def(x, inst->dst, d);
        }
        return;
    }
    case IR_STORE: {
        int width = x64_width(inst->type);
        bool is_float = ir_type_is_float(inst->type);
        int value = x64_use(x, inst->a, is_float ? X64_XMM14 : X64_R10);
        char addr[128];
        x64_address(x, inst->addr, X64_R11, addr, sizeof(addr));
        if (is_float) x64_line(x, width == 8 ? "movsd" : "movss", "%s %s, %s", x64_size_keyword(width), addr, x64_reg(value, 8));
        else          x64_line(x, "mov", "%s %s, %s", x64_size_keyword(width), addr, x64_reg(value, width));
        return;
    }
    case IR_PTR_ADD: {
        int a = x64_use(x, inst->a, X64_R10);
        int b = x64_use(x, inst->b, X64_R11);
        int d = x64_def(x, inst->dst, X64_R10);
        x64_line(x, "lea", "%s, [%s + %s]", x64_reg(d, 8), x64_reg(a, 8), x64_reg(b, 8));
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_MEMCPY:
    case IR_MEMSET: {
        X64CallArg args[3] = {0};
        args[0].src = move_vreg(x, inst->a);
        args[1].src = inst->op == IR_MEMCPY ? move_vreg(x, inst->b) : move_imm(inst->imm2);
        args[2].src = move_imm(inst->imm);
        x64_emit_call_with_args(x, args, 3, inst->op == IR_MEMCPY ? "$memcpy" : "$memset");
        return;
    }

    case IR_ADD: case IR_SUB: case IR_MUL:
    case IR_AND: case IR_OR:  case IR_XOR:
        x64_emit_int_binop(x, inst);
        return;
    case IR_SDIV: case IR_UDIV: case IR_SREM: case IR_UREM:
        x64_emit_divide(x, inst);
        return;
    case IR_SHL: case IR_LSHR: case IR_ASHR:
        x64_emit_shift(x, inst);
        return;
    case IR_NEG:
    case IR_NOT: {
        int width = x64_width(inst->type);
        if (width < 4) width = 4;
        int a = x64_use(x, inst->a, X64_R10);
        int d = x64_def(x, inst->dst, X64_R10);
        if (d != a) x64_line(x, "mov", "%s, %s", x64_reg(d, width), x64_reg(a, width));
        x64_line(x, inst->op == IR_NEG ? "neg" : "not", "%s", x64_reg(d, width));
        x64_finish_def(x, inst->dst, d);
        return;
    }

    case IR_FADD: case IR_FSUB: case IR_FMUL: case IR_FDIV:
        x64_emit_float_binop(x, inst);
        return;
    case IR_FNEG: {
        int a = x64_use(x, inst->a, X64_XMM14);
        int d = x64_def(x, inst->dst, X64_XMM14);
        x64_move_reg(x, d, a);
        x64_line(x, "xorps", "%s, [rel %s]", x64_reg(d, 8), inst->type == IR_F64 ? "__sign_mask_f64" : "__sign_mask_f32");
        x64_finish_def(x, inst->dst, d);
        return;
    }

    case IR_ICMP: {
        int width = x64_width(inst->type);
        int a = x64_use(x, inst->a, X64_R10);
        int b = x64_use(x, inst->b, X64_R11);
        int d = x64_def(x, inst->dst, X64_R10);
        x64_line(x, "cmp", "%s, %s", x64_reg(a, width), x64_reg(b, width));
        char setcc[8];
        snprintf(setcc, sizeof(setcc), "set%s", x64_icmp_cc(inst->cmp));
        x64_line(x, setcc, "%s", x64_reg(d, 1));
        x64_finish_def(x, inst->dst, d);
        return;
    }
    case IR_FCMP:
        x64_emit_fcmp(x, inst);
        return;

    case IR_SEXT: case IR_ZEXT: case IR_TRUNC:
    case IR_SITOFP: case IR_UITOFP: case IR_FPTOSI: case IR_FPTOUI:
    case IR_FPEXT: case IR_FPTRUNC: case IR_BITCAST:
        x64_emit_conversion(x, inst);
        return;

    case IR_CALL:
        x64_emit_call(x, inst);
        return;

    default:
        printf("Compiler Error: x64 backend can not emit '%s'\n", ir_op_name(inst->op));
        assert(false);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////

static int align_to(int value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

static void x64_layout_frame(X64Emitter *x) {
    IrFunction *func = x->func;
    int depth = 0; // Bytes below rbp

    // Saved xmm registers (movaps needs 16 byte alignment, rbp is 16 byte aligned)
    for (int r = X64_XMM0; r <= X64_XMM15; r++) {
        if (!x->ra.used_callee_saved[r]) continue;
        depth = align_to(depth + 16, 16);
        x->saved_gpr_offset[r] = -depth;
    }
    // Saved gprs
    for (int r = 0; r < 16; r++) {
        if (!x->ra.used_callee_saved[r]) continue;
        depth += 8;
        x->saved_gpr_offset[r] = -depth;
    }
    // IR slots
    x->slot_offsets = calloc(func->slots.count > 0 ? func->slots.count : 1, sizeof(int));
    for (int i = 0; i < func->slots.count; i++) {
        IrSlot *slot = &func->slots.items[i];
        int align = slot->align > 16 ? 16 : slot->align;
        depth = align_to(depth + slot->size, align);
        x->slot_offsets[i] = -depth;
    }
    // Spill slots
    depth = align_to(depth, 8);
    x->spill_base = -(depth + 8);
    depth += 8 * x->ra.spill_slot_count;

    // Outgoing arguments: shadow space plus stack arguments of the biggest call
    int max_args = -1;
    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            if (inst->op == IR_CALL && inst->call->arg_count > max_args) max_args = inst->call->arg_count;
            if ((inst->op == IR_MEMCPY || inst->op == IR_MEMSET) && max_args < 3) max_args = 3;
        }
    }
    int outgoing = 0;
    if (max_args >= 0) outgoing = 32 + 8 * (max_args > 4 ? max_args - 4 : 0);

    x->frame_size = align_to(depth, 16) + align_to(outgoing, 16);
}

static void x64_emit_epilogue(X64Emitter *x) {
    for (int r = 0; r < X64_REG_COUNT; r++) {
        if (!x->ra.used_callee_saved[r]) continue;
        if (x64_is_xmm(r)) x64_line(x, "movaps", "%s, [rbp%+d]", x64_reg(r, 8), x->saved_gpr_offset[r]);
        else               x64_line(x, "mov", "%s, [rbp%+d]", x64_reg(r, 8), x->saved_gpr_offset[r]);
    }
    x64_line(x, "leave", "");
    x64_line(x, "ret", "");
}

static void x64_emit_terminator(X64Emitter *x, IrBlock *block) {
    IrTerm *term = &block->term;
    switch (term->kind) {
    case IR_TERM_JMP:
        if (term->target != x->next_block) x64_line(x, "jmp", ".L%d", term->target->id);
        return;
    case IR_TERM_BR: {
        int cond = x64_use(x, term->cond, X64_R10);
        x64_line(x, "test", "%s, %s", x64_reg(cond, 1), x64_reg(cond, 1));
        if (term->target == x->next_block) {
            x64_line(x, "jz", ".L%d", term->else_target->id);
        } else if (term->else_target == x->next_block) {
            x64_line(x, "jnz", ".L%d", term->target->id);
        } else {
            x64_line(x, "jnz", ".L%d", term->target->id);
            x64_line(x, "jmp", ".L%d", term->else_target->id);
        }
        return;
    }
    case IR_TERM_RET:
        if (term->value != IR_NO_VREG) {
            bool is_float = ir_type_is_float(x64_vtype(x, term->value));
            x64_emit_move(x, move_reg(is_float ? X64_XMM0 : X64_RAX), move_vreg(x, term->value));
        } else if (x->func->is_entry_point) {
            x64_line(x, "xor", "eax, eax"); // main returns 0 to the C runtime
        }
        x64_emit_epilogue(x);
        return;
    default:
        assert(false);
    }
}

static void x64_emit_function(X64Emitter *x, IrFunction *func) {
    x->func = func;
    x->ra   = regalloc_function(func, &x->target);
    x->next_local_label = 0;
    for (int r = 0; r < X64_REG_COUNT; r++) x->saved_gpr_offset[r] = 0;

    regalloc_dump(func, &x->ra, &x->target, &x->regalloc_dump);

    x64_layout_frame(x);

    sb_append(&x->text, "\n");
    x64_label(x, "%s", x64_symbol(x, func->global));

    // Prologue
    x64_line(x, "push", "rbp");
    x64_line(x, "mov", "rbp, rsp");
    if (x->frame_size >= 4096) {
        // Windows commits the stack one guard page at a time, so big frames have to touch every page in order
        x64_line(x, "mov", "r11d, %d", x->frame_size / 4096);
        x64_label(x, ".Lstack_probe");
        x64_line(x, "sub", "rsp, 4096");
        x64_line(x, "mov", "qword [rsp], 0");
        x64_line(x, "dec", "r11d");
        x64_line(x, "jnz", ".Lstack_probe");
        if (x->frame_size % 4096) x64_line(x, "sub", "rsp, %d", x->frame_size % 4096);
    } else if (x->frame_size > 0) {
        x64_line(x, "sub", "rsp, %d", x->frame_size);
    }
    for (int r = 0; r < X64_REG_COUNT; r++) {
        if (!x->ra.used_callee_saved[r]) continue;
        if (x64_is_xmm(r)) x64_line(x, "movaps", "[rbp%+d], %s", x->saved_gpr_offset[r], x64_reg(r, 8));
        else               x64_line(x, "mov", "[rbp%+d], %s", x->saved_gpr_offset[r], x64_reg(r, 8));
    }

    // Incoming parameters -> their allocated locations
    {
        Move moves[64];
        int move_count = 0;
        for (int i = 0; i < func->params.count && move_count < 64; i++) {
            int vreg = func->params.items[i];
            if (x->ra.intervals[vreg].start > x->ra.intervals[vreg].end) continue; // Unused parameter
            MoveOperand src;
            if (i < 4) src = move_reg(ir_type_is_float(x64_vtype(x, vreg)) ? X64_XMM0 + i : x64_gpr_arg_regs[i]);
            else       src = move_mem(X64_RBP, 16 + 8 * i);
            moves[move_count++] = (Move){ move_vreg(x, vreg), src, false };
        }
        x64_parallel_move(x, moves, move_count);
    }

    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        x->next_block = b + 1 < func->blocks.count ? func->blocks.items[b + 1] : NULL;

        x64_label(x, ".L%d", block->id);
        for (int i = 0; i < block->insts.count; i++) {
            x64_emit_inst(x, &block->insts.items[i]);
        }
        x64_emit_terminator(x, block);
    }

    free(x->slot_offsets);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Module
//////////////////////////////////////////////////////////////////////////////////////////

static void x64_emit_global_data(X64Emitter *x, StringBuilder *sb, IrGlobal *global) {
    sb_append(sb, "\talign %d\n", global->align);
    sb_append(sb, "%s:\n", x64_symbol(x, global->index));

    int offset = 0;
    while (offset < global->size) {
        // A relocation at this offset?
        IrReloc *reloc = NULL;
        for (int r = 0; r < global->relocs.count; r++) {
            if (global->relocs.items[r].offset == offset) { reloc = &global->relocs.items[r]; break; }
        }
        if (reloc) {
            sb_append(sb, "\tdq %s\n", x64_symbol(x, reloc->target));
            offset += 8;
            continue;
        }

        // Next relocation (bytes before it are emitted as plain data)
        int stop = global->size;
        for (int r = 0; r < global->relocs.count; r++) {
            int ro = global->relocs.items[r].offset;
            if (ro > offset && ro < stop) stop = ro;
        }

        // Runs of zeros are compressed
        int zeros = 0;
        while (offset + zeros < stop && global->data[offset + zeros] == 0) zeros++;
        if (zeros >= 8 || (zeros > 0 && offset + zeros == stop)) {
            sb_append(sb, "\ttimes %d db 0\n", zeros);
            offset += zeros;
            continue;
        }

        sb_append(sb, "\tdb ");
        int n = 0;
        while (offset < stop && n < 16) {
            if (n > 0 && global->data[offset] == 0) {
                int z = 0;
                while (offset + z < stop && global->data[offset + z] == 0) z++;
                if (z >= 8) break;
            }
            sb_append(sb, "%s%u", n ? ", " : "", global->data[offset]);
            offset++;
            n++;
        }
        sb_append(sb, "\n");
    }
    if (global->size == 0) sb_append(sb, "\tdb 0\n");
}

typedef struct X64Output {
    StringBuilder assembly;
    StringBuilder regalloc_dump;
} X64Output;

X64Output x64_emit_module(IrModule *module) {
    X64Emitter *x = calloc(1, sizeof(X64Emitter));
    x->module = module;
    x->target = x64_target();
    x->text = sb_init(1 << 16);
    x->regalloc_dump = sb_init(1 << 12);

    // memcpy/memset are used by IR_MEMCPY / IR_MEMSET
    ir_find_or_add_extern(module, "memcpy");
    ir_find_or_add_extern(module, "memset");

    for (int i = 0; i < module->functions.count; i++) {
        x64_emit_function(x, module->functions.items[i]);
    }

    StringBuilder out = sb_init(1 << 16);
    sb_append(&out, "; Generated by the Saad compiler (IR backend, x64 Win64)\n");
    sb_append(&out, "bits 64\n");
    sb_append(&out, "default rel\n\n");

    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind == IR_GLOBAL_EXTERN) sb_append(&out, "extern %s\n", x64_symbol(x, i));
        if (global->kind == IR_GLOBAL_FUNCTION && global->function->is_entry_point) sb_append(&out, "global %s\n", x64_symbol(x, i));
    }

    sb_append(&out, "\nsection .data\n");
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind == IR_GLOBAL_DATA && !global->readonly) x64_emit_global_data(x, &out, global);
    }

    sb_append(&out, "\nsection .rdata\n");
    sb_append(&out, "\talign 16\n__sign_mask_f32:\n\tdd 0x80000000, 0, 0, 0\n");
    sb_append(&out, "\talign 16\n__sign_mask_f64:\n\tdq 0x8000000000000000, 0\n");
    for (int i = 0; i < x->float_constants.count; i++) {
        X64FloatConstant *c = &x->float_constants.items[i];
        if (c->size == 4) sb_append(&out, "\talign 4\n__float%d:\n\tdd 0x%08x\n", c->id, (uint32_t) c->bits);
        else              sb_append(&out, "\talign 8\n__float%d:\n\tdq 0x%016llx\n", c->id, (unsigned long long) c->bits);
    }
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind == IR_GLOBAL_DATA && global->readonly) x64_emit_global_data(x, &out, global);
    }

    sb_append(&out, "\nsection .text\n");
    sb_copy(&out, x->text.buffer, (int) x->text.cursor);

    X64Output output = { out, x->regalloc_dump };
    return output;
}
