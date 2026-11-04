//////////////////////////////////////////////////////////////////////////////////////////
//
// LLVM backend: builds an LLVM module from the IR with the C API of LLVM, optimizes it and writes the object file.
// The compiler has it when it is built with SAAD_LLVM (build.sh does that when LLVM is installed)
//
//   * A vreg is an LLVM value. A vreg that is used in another block than the one defining it lives in an alloca,
//     which mem2reg makes a value again, so the order of the blocks doesn't matter
//   * A slot is an alloca in a block before the first block of the function, so the first block can be jumped to
//   * An address is a getelementptr of i8 from a vreg, a slot or a global
//   * A data global is a packed struct of its bytes and of a ptr for each relocation
//   * Every function has the Win64 calling convention, like the x64 backend: on Windows it is the C one, and for the
//     Linux harness (--elf) it is set on every function and call, as tools/linux/shim.c gives the C functions that
//     convention
//   * The runtime functions that the x64 backend writes in assembly are built here
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef SAAD_LLVM

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>

typedef struct LlvmEmitter {
    IrModule        *module;
    IrFunction      *func;
    bool             elf;          // For the Linux harness: the Win64 convention and ms_ names of the C functions
    LLVMContextRef   context;
    LLVMModuleRef    llvm;
    LLVMBuilderRef   builder;
    LLVMTypeRef      types[IR_TYPE_COUNT];
    LLVMTypeRef      i1;
    LLVMValueRef    *globals;      // Of each global of the module

    // Of the function being built
    LLVMValueRef    *values;       // Of each vreg
    LLVMValueRef    *homes;        // The alloca of a vreg used in another block, else NULL
    LLVMValueRef    *slots;
    LLVMBasicBlockRef *blocks;     // By block id
} LlvmEmitter;

static void llvm_set_cc(LlvmEmitter *e, LLVMValueRef function_or_call, bool is_call) {
    if (!e->elf) return;
    if (is_call) LLVMSetInstructionCallConv(function_or_call, LLVMWin64CallConv);
    else         LLVMSetFunctionCallConv(function_or_call, LLVMWin64CallConv);
}

static LLVMValueRef llvm_i64(LlvmEmitter *e, int64_t value) {
    return LLVMConstInt(e->types[IR_I64], (unsigned long long) value, true);
}

// A function of C, declared once, e.g printf for the asserts. The C functions get ms_ for the Linux harness
static LLVMValueRef llvm_c_function(LlvmEmitter *e, const char *name, LLVMTypeRef type) {
    char symbol[256];
    snprintf(symbol, sizeof(symbol), "%s%s", e->elf ? "ms_" : "", name);
    LLVMValueRef function = LLVMGetNamedFunction(e->llvm, symbol);
    if (!function) {
        function = LLVMAddFunction(e->llvm, symbol, type);
        llvm_set_cc(e, function, false);
    }
    return function;
}

//
// Values of vregs
//

static LLVMValueRef llvm_get(LlvmEmitter *e, int vreg) {
    if (e->homes[vreg]) return LLVMBuildLoad2(e->builder, e->types[ir_vreg_type(e->func, vreg)], e->homes[vreg], "");
    return e->values[vreg];
}

static void llvm_set(LlvmEmitter *e, int vreg, LLVMValueRef value) {
    if (vreg == IR_NO_VREG) return;
    e->values[vreg] = value;
    if (e->homes[vreg]) LLVMBuildStore(e->builder, value, e->homes[vreg]);
}

// An address as a ptr
static LLVMValueRef llvm_address(LlvmEmitter *e, IrAddr addr) {
    LLVMValueRef base;
    switch (addr.kind) {
    case IR_ADDR_VREG:   base = llvm_get(e, addr.base); break;
    case IR_ADDR_SLOT:   base = e->slots[addr.base]; break;
    case IR_ADDR_GLOBAL: base = e->globals[addr.base]; break;
    default:             base = LLVMConstNull(e->types[IR_PTR]); break;
    }
    if (LLVMTypeOf(base) != e->types[IR_PTR]) base = LLVMBuildIntToPtr(e->builder, base, e->types[IR_PTR], "");
    if (addr.offset == 0) return base;
    LLVMValueRef offset = llvm_i64(e, addr.offset);
    return LLVMBuildGEP2(e->builder, e->types[IR_I8], base, &offset, 1, "");
}

static LLVMIntPredicate llvm_icmp(IrCmp cmp) {
    switch (cmp) {
    case IR_CMP_EQ:  return LLVMIntEQ;
    case IR_CMP_NE:  return LLVMIntNE;
    case IR_CMP_SLT: return LLVMIntSLT;
    case IR_CMP_SLE: return LLVMIntSLE;
    case IR_CMP_SGT: return LLVMIntSGT;
    case IR_CMP_SGE: return LLVMIntSGE;
    case IR_CMP_ULT: return LLVMIntULT;
    case IR_CMP_ULE: return LLVMIntULE;
    case IR_CMP_UGT: return LLVMIntUGT;
    case IR_CMP_UGE: return LLVMIntUGE;
    default:         return LLVMIntEQ;
    }
}

static LLVMRealPredicate llvm_fcmp(IrCmp cmp) {
    switch (cmp) {
    case IR_CMP_OEQ: return LLVMRealOEQ;
    case IR_CMP_UNE: return LLVMRealUNE;
    case IR_CMP_OLT: return LLVMRealOLT;
    case IR_CMP_OLE: return LLVMRealOLE;
    case IR_CMP_OGT: return LLVMRealOGT;
    case IR_CMP_OGE: return LLVMRealOGE;
    default:         return LLVMRealOEQ;
    }
}

static int llvm_bits(IrType type) {
    return type == IR_PTR ? 64 : ir_type_size(type) * 8;
}

// An integer conversion between the types of a and of the instruction, e.g sext i32 to i64
static LLVMValueRef llvm_convert_int(LlvmEmitter *e, IrOp op, LLVMValueRef value, IrType from, IrType to) {
    LLVMBuilderRef b = e->builder;
    LLVMTypeRef target = e->types[to];
    if (from == IR_PTR) value = LLVMBuildPtrToInt(b, value, e->types[IR_I64], "");
    LLVMTypeRef int_target = to == IR_PTR ? e->types[IR_I64] : target;
    int from_bits = llvm_bits(from), to_bits = llvm_bits(to);
    LLVMValueRef result = value;
    if (to_bits < from_bits)       result = LLVMBuildTrunc(b, value, int_target, "");
    else if (to_bits > from_bits)  result = op == IR_SEXT ? LLVMBuildSExt(b, value, int_target, "") : LLVMBuildZExt(b, value, int_target, "");
    if (to == IR_PTR) result = LLVMBuildIntToPtr(b, result, target, "");
    return result;
}

// The function type of a call, from the types of its arguments, with '...' for the vararg ones
static LLVMTypeRef llvm_call_type(LlvmEmitter *e, IrCall *call) {
    LLVMTypeRef params[64];
    int fixed = 0;
    bool vararg = false;
    for (int i = 0; i < call->arg_count; i++) {
        if (call->arg_is_vararg[i]) { vararg = true; continue; }
        if (fixed < 64) params[fixed++] = e->types[ir_vreg_type(e->func, call->args[i])];
    }
    return LLVMFunctionType(e->types[call->return_type], params, fixed, vararg);
}

static void llvm_emit_inst(LlvmEmitter *e, IrInst *inst) {
    LLVMBuilderRef b = e->builder;
    IrFunction *func = e->func;
    LLVMTypeRef t = e->types[inst->type];
    LLVMValueRef r = NULL;

    switch (inst->op) {
    case IR_NOP:
        return;

    case IR_CONST:
        if (inst->type == IR_PTR) r = inst->imm == 0 ? LLVMConstNull(t) : LLVMConstIntToPtr(llvm_i64(e, inst->imm), t);
        else                      r = LLVMConstInt(t, (unsigned long long) inst->imm, false);
        break;

    case IR_CONST_FLOAT:
        r = LLVMConstReal(t, inst->type == IR_F32 ? (double)(float) inst->fimm : inst->fimm);
        break;

    case IR_COPY:        r = llvm_get(e, inst->a); break;
    case IR_SLOT_ADDR:   r = e->slots[inst->imm]; break;
    case IR_GLOBAL_ADDR: r = e->globals[inst->imm]; break;

    case IR_LOAD:
        r = LLVMBuildLoad2(b, t, llvm_address(e, inst->addr), "");
        LLVMSetAlignment(r, 1);
        break;

    case IR_STORE: {
        LLVMValueRef store = LLVMBuildStore(b, llvm_get(e, inst->a), llvm_address(e, inst->addr));
        LLVMSetAlignment(store, 1);
        return;
    }

    case IR_PTR_ADD: {
        LLVMValueRef base = llvm_get(e, inst->a), offset = llvm_get(e, inst->b);
        if (LLVMTypeOf(base) != e->types[IR_PTR]) base = LLVMBuildIntToPtr(b, base, e->types[IR_PTR], "");
        r = LLVMBuildGEP2(b, e->types[IR_I8], base, &offset, 1, "");
        break;
    }

    case IR_MEMCPY:
        if (inst->imm > 0) LLVMBuildMemMove(b, llvm_get(e, inst->a), 1, llvm_get(e, inst->b), 1, llvm_i64(e, inst->imm));
        return;

    case IR_MEMSET:
        if (inst->imm > 0) LLVMBuildMemSet(b, llvm_get(e, inst->a), LLVMConstInt(e->types[IR_I8], (unsigned long long) inst->imm2, false), llvm_i64(e, inst->imm), 1);
        return;

    case IR_ADD:  r = LLVMBuildAdd(b,  llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_SUB:  r = LLVMBuildSub(b,  llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_MUL:  r = LLVMBuildMul(b,  llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_SDIV: r = LLVMBuildSDiv(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_UDIV: r = LLVMBuildUDiv(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_SREM: r = LLVMBuildSRem(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_UREM: r = LLVMBuildURem(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_AND:  r = LLVMBuildAnd(b,  llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_OR:   r = LLVMBuildOr(b,   llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_XOR:  r = LLVMBuildXor(b,  llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;

    case IR_SHL:
    case IR_LSHR:
    case IR_ASHR: {
        // The count is masked like x64 does, as LLVM gives poison for a count of the width or more
        LLVMValueRef count = LLVMBuildAnd(b, llvm_get(e, inst->b), LLVMConstInt(t, llvm_bits(inst->type) - 1, false), "");
        LLVMValueRef value = llvm_get(e, inst->a);
        if (inst->op == IR_SHL)       r = LLVMBuildShl(b, value, count, "");
        else if (inst->op == IR_LSHR) r = LLVMBuildLShr(b, value, count, "");
        else                          r = LLVMBuildAShr(b, value, count, "");
        break;
    }

    case IR_NEG:  r = LLVMBuildNeg(b, llvm_get(e, inst->a), ""); break;
    case IR_NOT:  r = LLVMBuildNot(b, llvm_get(e, inst->a), ""); break;

    case IR_FADD: r = LLVMBuildFAdd(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_FSUB: r = LLVMBuildFSub(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_FMUL: r = LLVMBuildFMul(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_FDIV: r = LLVMBuildFDiv(b, llvm_get(e, inst->a), llvm_get(e, inst->b), ""); break;
    case IR_FNEG: r = LLVMBuildFNeg(b, llvm_get(e, inst->a), ""); break;

    case IR_ICMP:
    case IR_FCMP: {
        LLVMValueRef x = llvm_get(e, inst->a), y = llvm_get(e, inst->b);
        LLVMValueRef bit;
        if (inst->op == IR_FCMP) {
            bit = LLVMBuildFCmp(b, llvm_fcmp(inst->cmp), x, y, "");
        } else {
            // A ptr and an int, e.g a function value with null, are compared as ints
            if (LLVMTypeOf(x) != LLVMTypeOf(y)) {
                if (LLVMTypeOf(x) == e->types[IR_PTR]) x = LLVMBuildPtrToInt(b, x, LLVMTypeOf(y), "");
                if (LLVMTypeOf(y) == e->types[IR_PTR]) y = LLVMBuildPtrToInt(b, y, LLVMTypeOf(x), "");
            }
            bit = LLVMBuildICmp(b, llvm_icmp(inst->cmp), x, y, "");
        }
        r = LLVMBuildZExt(b, bit, e->types[IR_I8], "");
        llvm_set(e, inst->dst, r);
        return;
    }

    case IR_SEXT:
    case IR_ZEXT:
    case IR_TRUNC:
        r = llvm_convert_int(e, inst->op, llvm_get(e, inst->a), ir_vreg_type(func, inst->a), inst->type);
        break;

    case IR_SITOFP: r = LLVMBuildSIToFP(b, llvm_get(e, inst->a), t, ""); break;
    case IR_UITOFP: r = LLVMBuildUIToFP(b, llvm_get(e, inst->a), t, ""); break;
    case IR_FPTOSI: r = LLVMBuildFPToSI(b, llvm_get(e, inst->a), t, ""); break;
    case IR_FPTOUI: r = LLVMBuildFPToUI(b, llvm_get(e, inst->a), t, ""); break;

    case IR_FPEXT:
    case IR_FPTRUNC: {
        IrType from = ir_vreg_type(func, inst->a);
        LLVMValueRef value = llvm_get(e, inst->a);
        if (from == inst->type)              r = value;
        else if (llvm_bits(from) < llvm_bits(inst->type)) r = LLVMBuildFPExt(b, value, t, "");
        else                                 r = LLVMBuildFPTrunc(b, value, t, "");
        break;
    }

    case IR_BITCAST: {
        IrType from = ir_vreg_type(func, inst->a);
        LLVMValueRef value = llvm_get(e, inst->a);
        if (from == inst->type)                              r = value;
        else if (from == IR_PTR)                             r = LLVMBuildPtrToInt(b, value, t, "");
        else if (inst->type == IR_PTR)                       r = LLVMBuildIntToPtr(b, value, t, "");
        else                                                 r = LLVMBuildBitCast(b, value, t, "");
        break;
    }

    case IR_CALL: {
        IrCall *call = inst->call;
        LLVMValueRef args[64];
        int count = call->arg_count < 64 ? call->arg_count : 64;
        for (int i = 0; i < count; i++) args[i] = llvm_get(e, call->args[i]);
        LLVMValueRef callee = call->callee_kind == IR_CALLEE_DIRECT ? e->globals[call->callee] : llvm_get(e, call->callee);
        if (LLVMTypeOf(callee) != e->types[IR_PTR]) callee = LLVMBuildIntToPtr(b, callee, e->types[IR_PTR], "");
        r = LLVMBuildCall2(b, llvm_call_type(e, call), callee, args, count, "");
        llvm_set_cc(e, r, true);
        if (call->return_type == IR_VOID) return;
        break;
    }

    default:
        printf("Compiler Error: The LLVM backend has no %s\n", ir_op_name(inst->op));
        return;
    }

    llvm_set(e, inst->dst, r);
}

static void llvm_emit_term(LlvmEmitter *e, IrTerm *term) {
    LLVMBuilderRef b = e->builder;
    switch (term->kind) {
    case IR_TERM_JMP:
        LLVMBuildBr(b, e->blocks[term->target->id]);
        break;
    case IR_TERM_BR: {
        LLVMValueRef cond = llvm_get(e, term->cond);
        LLVMValueRef bit = LLVMBuildICmp(b, LLVMIntNE, cond, LLVMConstNull(LLVMTypeOf(cond)), "");
        LLVMBuildCondBr(b, bit, e->blocks[term->target->id], e->blocks[term->else_target->id]);
        break;
    }
    case IR_TERM_RET:
        if (e->func->return_type == IR_VOID) LLVMBuildRetVoid(b);
        else if (term->value != IR_NO_VREG)  LLVMBuildRet(b, llvm_get(e, term->value));
        else                                 LLVMBuildRet(b, LLVMConstNull(e->types[e->func->return_type]));
        break;
    default:
        LLVMBuildUnreachable(b);
        break;
    }
}

// Marks the vregs that are used in another block than the one defining them. They live in an alloca
static bool *llvm_find_homed(IrFunction *func) {
    int count = func->vreg_types.count;
    int *defined_in = malloc((count + 1) * sizeof(int));
    bool *homed = calloc(count + 1, sizeof(bool));
    for (int v = 0; v < count; v++) defined_in[v] = -1;
    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        for (int i = 0; i < block->insts.count; i++) {
            if (block->insts.items[i].dst != IR_NO_VREG) defined_in[block->insts.items[i].dst] = b;
        }
    }
    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            for (int u = 0; u < ir_inst_use_count(inst); u++) {
                int v = ir_inst_use(inst, u);
                if (v >= 0 && defined_in[v] >= 0 && defined_in[v] != b) homed[v] = true;
            }
        }
        for (int u = 0; u < ir_term_use_count(&block->term); u++) {
            int v = ir_term_use(&block->term, u);
            if (v >= 0 && defined_in[v] >= 0 && defined_in[v] != b) homed[v] = true;
        }
    }
    free(defined_in);
    return homed;
}

static void llvm_emit_function(LlvmEmitter *e, IrFunction *func) {
    LLVMBuilderRef b = e->builder;
    LLVMValueRef function = e->globals[func->global];
    e->func = func;

    int vreg_count = func->vreg_types.count;
    e->values = calloc(vreg_count + 1, sizeof(LLVMValueRef));
    e->homes  = calloc(vreg_count + 1, sizeof(LLVMValueRef));
    e->slots  = calloc(func->slots.count + 1, sizeof(LLVMValueRef));
    e->blocks = calloc(func->next_block_id + 1, sizeof(LLVMBasicBlockRef));

    for (int i = 0; i < func->params.count; i++) e->values[func->params.items[i]] = LLVMGetParam(function, i);

    // The allocas, in a block before the first block, which other blocks can jump to
    LLVMBasicBlockRef allocas = LLVMAppendBasicBlockInContext(e->context, function, "allocas");
    LLVMPositionBuilderAtEnd(b, allocas);
    for (int i = 0; i < func->slots.count; i++) {
        IrSlot slot = func->slots.items[i];
        LLVMValueRef alloca = LLVMBuildAlloca(b, LLVMArrayType2(e->types[IR_I8], slot.size > 0 ? slot.size : 1), "");
        LLVMSetAlignment(alloca, slot.align > 0 ? slot.align : 1);
        e->slots[i] = alloca;
    }
    bool *homed = llvm_find_homed(func);
    for (int v = 0; v < vreg_count; v++) {
        if (homed[v]) e->homes[v] = LLVMBuildAlloca(b, e->types[ir_vreg_type(func, v)], "");
    }
    free(homed);

    for (int i = 0; i < func->blocks.count; i++) {
        IrBlock *block = func->blocks.items[i];
        e->blocks[block->id] = LLVMAppendBasicBlockInContext(e->context, function, "");
    }
    if (func->blocks.count > 0) LLVMBuildBr(b, e->blocks[func->blocks.items[0]->id]);
    else                        LLVMBuildUnreachable(b);

    for (int i = 0; i < func->blocks.count; i++) {
        IrBlock *block = func->blocks.items[i];
        LLVMPositionBuilderAtEnd(b, e->blocks[block->id]);
        for (int j = 0; j < block->insts.count; j++) llvm_emit_inst(e, &block->insts.items[j]);
        llvm_emit_term(e, &block->term);
    }

    free(e->values);
    free(e->homes);
    free(e->slots);
    free(e->blocks);
}

// A data global: its bytes, with a ptr at the offset of each relocation. Gives its type, and its value once every
// global it points to exists (else NULL)
static LLVMValueRef llvm_data(LlvmEmitter *e, IrGlobal *global, LLVMTypeRef *type_out) {
    int size = global->size > 0 ? global->size : 1;

    // The relocations in the order of their offsets
    int count = global->relocs.count;
    IrReloc *relocs = calloc(count + 1, sizeof(IrReloc));
    for (int i = 0; i < count; i++) relocs[i] = global->relocs.items[i];
    for (int i = 1; i < count; i++) {
        IrReloc r = relocs[i];
        int j = i - 1;
        while (j >= 0 && relocs[j].offset > r.offset) { relocs[j + 1] = relocs[j]; j--; }
        relocs[j + 1] = r;
    }

    int capacity = 2 * count + 2;
    LLVMTypeRef  *types  = malloc(capacity * sizeof(LLVMTypeRef));
    LLVMValueRef *values = malloc(capacity * sizeof(LLVMValueRef));
    char *zeros = global->size > 0 ? NULL : calloc(1, 1);
    int at = 0, parts = 0;
    for (int r = 0; r <= count; r++) {
        int end = r < count ? relocs[r].offset : size;
        if (end > at) {
            const char *bytes = zeros ? zeros : (const char *) global->data + at;
            values[parts] = LLVMConstStringInContext(e->context, bytes, end - at, true);
            types[parts]  = LLVMTypeOf(values[parts]);
            parts++;
        }
        if (r < count) {
            values[parts] = e->globals[relocs[r].target];
            types[parts]  = e->types[IR_PTR];
            parts++;
            at = relocs[r].offset + 8;
        }
    }
    *type_out = LLVMStructTypeInContext(e->context, types, parts, true);
    LLVMValueRef value = NULL;
    bool complete = true;
    for (int i = 0; i < parts; i++) if (!values[i]) complete = false;
    if (complete) value = LLVMConstStructInContext(e->context, values, parts, true);
    free(types);
    free(values);
    free(relocs);
    free(zeros);
    return value;
}

// int runtime_compare_strings(string *a, string *b): <0, 0 or >0, comparing the bytes as signed chars like C
static void llvm_runtime_compare_strings(LlvmEmitter *e, LLVMValueRef function) {
    LLVMBuilderRef b = e->builder;
    LLVMTypeRef i8 = e->types[IR_I8], i32 = e->types[IR_I32], i64 = e->types[IR_I64], ptr = e->types[IR_PTR];
    LLVMBasicBlockRef entry  = LLVMAppendBasicBlockInContext(e->context, function, "entry");
    LLVMBasicBlockRef loop   = LLVMAppendBasicBlockInContext(e->context, function, "loop");
    LLVMBasicBlockRef body   = LLVMAppendBasicBlockInContext(e->context, function, "body");
    LLVMBasicBlockRef same   = LLVMAppendBasicBlockInContext(e->context, function, "same");
    LLVMBasicBlockRef differ = LLVMAppendBasicBlockInContext(e->context, function, "differ");
    LLVMBasicBlockRef end    = LLVMAppendBasicBlockInContext(e->context, function, "end");

    LLVMValueRef a = LLVMGetParam(function, 0), bb = LLVMGetParam(function, 1);
    LLVMValueRef eight = llvm_i64(e, 8);
    LLVMPositionBuilderAtEnd(b, entry);
    LLVMValueRef alen  = LLVMBuildLoad2(b, i64, LLVMBuildGEP2(b, i8, a, &eight, 1, ""), "");
    LLVMValueRef blen  = LLVMBuildLoad2(b, i64, LLVMBuildGEP2(b, i8, bb, &eight, 1, ""), "");
    LLVMValueRef adata = LLVMBuildLoad2(b, ptr, a, "");
    LLVMValueRef bdata = LLVMBuildLoad2(b, ptr, bb, "");
    LLVMBuildBr(b, loop);

    LLVMPositionBuilderAtEnd(b, loop);
    LLVMValueRef i = LLVMBuildPhi(b, i64, "");
    LLVMValueRef both = LLVMBuildAnd(b, LLVMBuildICmp(b, LLVMIntSLT, i, alen, ""), LLVMBuildICmp(b, LLVMIntSLT, i, blen, ""), "");
    LLVMBuildCondBr(b, both, body, end);

    LLVMPositionBuilderAtEnd(b, body);
    LLVMValueRef ca = LLVMBuildLoad2(b, i8, LLVMBuildGEP2(b, i8, adata, &i, 1, ""), "");
    LLVMValueRef cb = LLVMBuildLoad2(b, i8, LLVMBuildGEP2(b, i8, bdata, &i, 1, ""), "");
    LLVMBuildCondBr(b, LLVMBuildICmp(b, LLVMIntEQ, ca, cb, ""), same, differ);

    LLVMPositionBuilderAtEnd(b, same);
    LLVMValueRef next = LLVMBuildAdd(b, i, llvm_i64(e, 1), "");
    LLVMBuildBr(b, loop);

    LLVMValueRef zero = llvm_i64(e, 0);
    LLVMValueRef incoming[2] = { zero, next };
    LLVMBasicBlockRef from[2] = { entry, same };
    LLVMAddIncoming(i, incoming, from, 2);

    LLVMValueRef minus_one = LLVMConstInt(i32, (unsigned long long) -1, true), one = LLVMConstInt(i32, 1, false);
    LLVMPositionBuilderAtEnd(b, differ);
    LLVMBuildRet(b, LLVMBuildSelect(b, LLVMBuildICmp(b, LLVMIntSLT, ca, cb, ""), minus_one, one, ""));

    LLVMPositionBuilderAtEnd(b, end);
    LLVMValueRef longer = LLVMBuildSelect(b, LLVMBuildICmp(b, LLVMIntSGT, alen, blen, ""), one, LLVMConstNull(i32), "");
    LLVMBuildRet(b, LLVMBuildSelect(b, LLVMBuildICmp(b, LLVMIntSLT, alen, blen, ""), minus_one, longer, ""));
}

// void runtime_builtin_assert(bool condition, string *file, u64 line): prints where and ends the program when false
static void llvm_runtime_builtin_assert(LlvmEmitter *e, LLVMValueRef function) {
    LLVMBuilderRef b = e->builder;
    LLVMTypeRef i32 = e->types[IR_I32], ptr = e->types[IR_PTR];
    LLVMTypeRef printf_params[1] = { ptr };
    LLVMTypeRef printf_type = LLVMFunctionType(i32, printf_params, 1, true);
    LLVMTypeRef exit_params[1] = { i32 };
    LLVMTypeRef exit_type = LLVMFunctionType(e->types[IR_VOID], exit_params, 1, false);
    LLVMValueRef printf_function = llvm_c_function(e, "printf", printf_type);
    LLVMValueRef exit_function = llvm_c_function(e, "exit", exit_type);

    LLVMBasicBlockRef entry  = LLVMAppendBasicBlockInContext(e->context, function, "entry");
    LLVMBasicBlockRef done   = LLVMAppendBasicBlockInContext(e->context, function, "done");
    LLVMBasicBlockRef failed = LLVMAppendBasicBlockInContext(e->context, function, "failed");

    LLVMPositionBuilderAtEnd(b, entry);
    LLVMValueRef condition = LLVMGetParam(function, 0);
    LLVMBuildCondBr(b, LLVMBuildICmp(b, LLVMIntNE, condition, LLVMConstNull(e->types[IR_I8]), ""), done, failed);

    LLVMPositionBuilderAtEnd(b, done);
    LLVMBuildRetVoid(b);

    LLVMPositionBuilderAtEnd(b, failed);
    LLVMValueRef message = LLVMBuildGlobalStringPtr(b, "%s:%llu Assertion failed!\n", "");
    LLVMValueRef name = LLVMBuildLoad2(b, ptr, LLVMGetParam(function, 1), "");
    LLVMValueRef printf_args[3] = { message, name, LLVMGetParam(function, 2) };
    llvm_set_cc(e, LLVMBuildCall2(b, printf_type, printf_function, printf_args, 3, ""), true);
    LLVMValueRef exit_args[1] = { LLVMConstInt(i32, 1, false) };
    llvm_set_cc(e, LLVMBuildCall2(b, exit_type, exit_function, exit_args, 1, ""), true);
    LLVMBuildUnreachable(b);
}

// The type of an extern, from the first call of it. Any other call gives its own type, as a ptr is just a ptr
static LLVMTypeRef llvm_extern_type(LlvmEmitter *e, int index) {
    IrModule *module = e->module;
    for (int f = 0; f < module->functions.count; f++) {
        IrFunction *func = module->functions.items[f];
        for (int b = 0; b < func->blocks.count; b++) {
            IrBlock *block = func->blocks.items[b];
            for (int i = 0; i < block->insts.count; i++) {
                IrInst *inst = &block->insts.items[i];
                if (inst->op != IR_CALL || inst->call->callee_kind != IR_CALLEE_DIRECT || inst->call->callee != index) continue;
                e->func = func;
                return llvm_call_type(e, inst->call);
            }
        }
    }
    return LLVMFunctionType(e->types[IR_VOID], NULL, 0, false);
}

// Builds the LLVM module of the IR module, optimizes it at opt_level (0 to 3) and writes the object file to path:
// ELF for the Linux harness (elf), else COFF for Windows. Returns false, with the problem printed, on a failure
bool llvm_compile_module(IrModule *module, bool elf, int opt_level, const char *path) {
    LlvmEmitter e = {0};
    e.module = module;
    e.elf = elf;
    e.context = LLVMContextCreate();
    e.llvm = LLVMModuleCreateWithNameInContext("saad", e.context);
    e.builder = LLVMCreateBuilderInContext(e.context);
    e.types[IR_VOID] = LLVMVoidTypeInContext(e.context);
    e.types[IR_I8]   = LLVMInt8TypeInContext(e.context);
    e.types[IR_I16]  = LLVMInt16TypeInContext(e.context);
    e.types[IR_I32]  = LLVMInt32TypeInContext(e.context);
    e.types[IR_I64]  = LLVMInt64TypeInContext(e.context);
    e.types[IR_F32]  = LLVMFloatTypeInContext(e.context);
    e.types[IR_F64]  = LLVMDoubleTypeInContext(e.context);
    e.types[IR_PTR]  = LLVMPointerTypeInContext(e.context, 0);
    e.i1 = LLVMInt1TypeInContext(e.context);
    e.globals = calloc(module->globals.count + 1, sizeof(LLVMValueRef));

    // The externs first, so a function of the program with the name of a C function can't take its name
    int compare_strings = -1, builtin_assert = -1;
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind != IR_GLOBAL_EXTERN) continue;
        if (strcmp(global->name, "runtime_compare_strings") == 0) { compare_strings = i; continue; }
        if (strcmp(global->name, "runtime_builtin_assert") == 0)  { builtin_assert = i; continue; }
        e.globals[i] = llvm_c_function(&e, global->name, llvm_extern_type(&e, i));
    }

    // The runtime functions
    if (compare_strings >= 0) {
        LLVMTypeRef params[2] = { e.types[IR_PTR], e.types[IR_PTR] };
        e.globals[compare_strings] = LLVMAddFunction(e.llvm, "runtime_compare_strings", LLVMFunctionType(e.types[IR_I32], params, 2, false));
    }
    if (builtin_assert >= 0) {
        LLVMTypeRef params[3] = { e.types[IR_I8], e.types[IR_PTR], e.types[IR_I64] };
        e.globals[builtin_assert] = LLVMAddFunction(e.llvm, "runtime_builtin_assert", LLVMFunctionType(e.types[IR_VOID], params, 3, false));
    }

    // The functions of the program
    for (int f = 0; f < module->functions.count; f++) {
        IrFunction *func = module->functions.items[f];
        LLVMTypeRef params[64];
        int count = func->params.count < 64 ? func->params.count : 64;
        for (int i = 0; i < count; i++) params[i] = e.types[ir_vreg_type(func, func->params.items[i])];
        char symbol[512];
        snprintf(symbol, sizeof(symbol), "%s%s", func->is_entry_point && elf ? "ms_" : "", module->globals.items[func->global]->name);
        e.globals[func->global] = LLVMAddFunction(e.llvm, symbol, LLVMFunctionType(e.types[func->return_type], params, count, false));
    }

    // The data, its type first, and its value when every global it points to exists
    LLVMTypeRef *data_types = calloc(module->globals.count + 1, sizeof(LLVMTypeRef));
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind != IR_GLOBAL_DATA) continue;
        llvm_data(&e, global, &data_types[i]);
        LLVMValueRef value = LLVMAddGlobal(e.llvm, data_types[i], global->name);
        LLVMSetLinkage(value, LLVMInternalLinkage);
        LLVMSetGlobalConstant(value, global->readonly);
        LLVMSetAlignment(value, global->align > 0 ? global->align : 1);
        e.globals[i] = value;
    }
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind != IR_GLOBAL_DATA) continue;
        LLVMTypeRef type;
        LLVMSetInitializer(e.globals[i], llvm_data(&e, global, &type));
    }
    free(data_types);

    // The bodies
    for (int i = 0; i < 2; i++) {
        int index = i == 0 ? compare_strings : builtin_assert;
        if (index < 0) continue;
        LLVMSetLinkage(e.globals[index], LLVMInternalLinkage);
        llvm_set_cc(&e, e.globals[index], false);
        if (i == 0) llvm_runtime_compare_strings(&e, e.globals[index]);
        else        llvm_runtime_builtin_assert(&e, e.globals[index]);
    }
    unsigned nounwind = LLVMGetEnumAttributeKindForName("nounwind", 8);
    for (int f = 0; f < module->functions.count; f++) {
        IrFunction *func = module->functions.items[f];
        LLVMValueRef function = e.globals[func->global];
        if (!func->is_entry_point) LLVMSetLinkage(function, LLVMInternalLinkage);
        llvm_set_cc(&e, function, false);
        LLVMAddAttributeAtIndex(function, LLVMAttributeFunctionIndex, LLVMCreateEnumAttribute(e.context, nounwind, 0));
        llvm_emit_function(&e, func);
    }

    // The target
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    const char *triple = elf ? "x86_64-pc-linux-gnu" : "x86_64-pc-windows-msvc";
    char *error = NULL;
    LLVMTargetRef target;
    bool ok = false;
    LLVMTargetMachineRef machine = NULL;
    if (LLVMGetTargetFromTriple(triple, &target, &error)) {
        printf("Compiler Error: LLVM has no target %s: %s\n", triple, error);
        goto done;
    }
    LLVMCodeGenOptLevel levels[4] = { LLVMCodeGenLevelNone, LLVMCodeGenLevelLess, LLVMCodeGenLevelDefault, LLVMCodeGenLevelAggressive };
    if (opt_level < 0) opt_level = 0;
    if (opt_level > 3) opt_level = 3;
    machine = LLVMCreateTargetMachine(target, triple, "x86-64", "", levels[opt_level], elf ? LLVMRelocPIC : LLVMRelocDefault, LLVMCodeModelDefault);
    LLVMSetTarget(e.llvm, triple);
    LLVMTargetDataRef layout = LLVMCreateTargetDataLayout(machine);
    LLVMSetModuleDataLayout(e.llvm, layout);
    LLVMDisposeTargetData(layout);

    if (LLVMVerifyModule(e.llvm, LLVMReturnStatusAction, &error)) {
        printf("Compiler Error: The LLVM backend made an invalid module (./build/out.ll):\n%.2000s\n", error);
        LLVMPrintModuleToFile(e.llvm, "./build/out.ll", NULL);
        goto done;
    }
    LLVMDisposeMessage(error);
    error = NULL;

    char passes[32];
    snprintf(passes, sizeof(passes), "default<O%d>", opt_level);
    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    LLVMErrorRef pass_error = LLVMRunPasses(e.llvm, passes, machine, options);
    LLVMDisposePassBuilderOptions(options);
    if (pass_error) {
        char *message = LLVMGetErrorMessage(pass_error);
        printf("Compiler Error: LLVM couldn't optimize the program: %s\n", message);
        LLVMDisposeErrorMessage(message);
        goto done;
    }

    if (LLVMTargetMachineEmitToFile(machine, e.llvm, (char *) path, LLVMObjectFile, &error)) {
        printf("Compiler Error: LLVM couldn't write %s: %s\n", path, error);
        goto done;
    }
    ok = true;

done:
    if (error) LLVMDisposeMessage(error);
    if (machine) LLVMDisposeTargetMachine(machine);
    LLVMDisposeBuilder(e.builder);
    LLVMDisposeModule(e.llvm);
    LLVMContextDispose(e.context);
    free(e.globals);
    return ok;
}

#else

bool llvm_compile_module(IrModule *module, bool elf, int opt_level, const char *path) {
    (void) module; (void) elf; (void) opt_level; (void) path;
    printf("error: This saad was built without LLVM, so it has no --llvm. Build it with LLVM installed (see build.sh)\n");
    return false;
}

#endif
