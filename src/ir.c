#include "ir.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Types
//////////////////////////////////////////////////////////////////////////////////////////

int ir_type_size(IrType type) {
    switch (type) {
    case IR_VOID: return 0;
    case IR_I8:   return 1;
    case IR_I16:  return 2;
    case IR_I32:  return 4;
    case IR_I64:  return 8;
    case IR_F32:  return 4;
    case IR_F64:  return 8;
    case IR_PTR:  return 8;
    default:      return 0;
    }
}

bool ir_type_is_int(IrType type)        { return type >= IR_I8 && type <= IR_I64; }
bool ir_type_is_int_or_ptr(IrType type) { return ir_type_is_int(type) || type == IR_PTR; }
bool ir_type_is_float(IrType type)      { return type == IR_F32 || type == IR_F64; }

const char *ir_type_name(IrType type) {
    switch (type) {
    case IR_VOID: return "void";
    case IR_I8:   return "i8";
    case IR_I16:  return "i16";
    case IR_I32:  return "i32";
    case IR_I64:  return "i64";
    case IR_F32:  return "f32";
    case IR_F64:  return "f64";
    case IR_PTR:  return "ptr";
    default:      return "???";
    }
}

const char *ir_op_name(IrOp op) {
    switch (op) {
    case IR_NOP:         return "nop";
    case IR_CONST:       return "const";
    case IR_CONST_FLOAT: return "fconst";
    case IR_COPY:        return "copy";
    case IR_SLOT_ADDR:   return "slot_addr";
    case IR_GLOBAL_ADDR: return "global_addr";
    case IR_LOAD:        return "load";
    case IR_STORE:       return "store";
    case IR_PTR_ADD:     return "ptradd";
    case IR_MEMCPY:      return "memcpy";
    case IR_MEMSET:      return "memset";
    case IR_ADD:         return "add";
    case IR_SUB:         return "sub";
    case IR_MUL:         return "mul";
    case IR_SDIV:        return "sdiv";
    case IR_UDIV:        return "udiv";
    case IR_SREM:        return "srem";
    case IR_UREM:        return "urem";
    case IR_AND:         return "and";
    case IR_OR:          return "or";
    case IR_XOR:         return "xor";
    case IR_SHL:         return "shl";
    case IR_LSHR:        return "lshr";
    case IR_ASHR:        return "ashr";
    case IR_NEG:         return "neg";
    case IR_NOT:         return "not";
    case IR_FADD:        return "fadd";
    case IR_FSUB:        return "fsub";
    case IR_FMUL:        return "fmul";
    case IR_FDIV:        return "fdiv";
    case IR_FNEG:        return "fneg";
    case IR_ICMP:        return "icmp";
    case IR_FCMP:        return "fcmp";
    case IR_SEXT:        return "sext";
    case IR_ZEXT:        return "zext";
    case IR_TRUNC:       return "trunc";
    case IR_SITOFP:      return "sitofp";
    case IR_UITOFP:      return "uitofp";
    case IR_FPTOSI:      return "fptosi";
    case IR_FPTOUI:      return "fptoui";
    case IR_FPEXT:       return "fpext";
    case IR_FPTRUNC:     return "fptrunc";
    case IR_BITCAST:     return "bitcast";
    case IR_CALL:        return "call";
    default:             return "???";
    }
}

const char *ir_cmp_name(IrCmp cmp) {
    switch (cmp) {
    case IR_CMP_NONE: return "none";
    case IR_CMP_EQ:   return "eq";
    case IR_CMP_NE:   return "ne";
    case IR_CMP_SLT:  return "slt";
    case IR_CMP_SLE:  return "sle";
    case IR_CMP_SGT:  return "sgt";
    case IR_CMP_SGE:  return "sge";
    case IR_CMP_ULT:  return "ult";
    case IR_CMP_ULE:  return "ule";
    case IR_CMP_UGT:  return "ugt";
    case IR_CMP_UGE:  return "uge";
    case IR_CMP_OEQ:  return "oeq";
    case IR_CMP_UNE:  return "une";
    case IR_CMP_OLT:  return "olt";
    case IR_CMP_OLE:  return "ole";
    case IR_CMP_OGT:  return "ogt";
    case IR_CMP_OGE:  return "oge";
    default:          return "???";
    }
}

static bool ir_cmp_is_int(IrCmp cmp)   { return cmp >= IR_CMP_EQ && cmp <= IR_CMP_UGE; }
static bool ir_cmp_is_float(IrCmp cmp) { return cmp >= IR_CMP_OEQ && cmp <= IR_CMP_OGE; }

//////////////////////////////////////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////////////////////////////////////

IrModule *ir_module_new(void) {
    IrModule *module = calloc(1, sizeof(IrModule));
    module->arena = arena_init(1 << 16);
    return module;
}

void *ir_allocate(IrModule *module, size_t size) {
    return arena_allocate(&module->arena, size);
}

static char *ir_strdup(IrModule *module, const char *str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    char *copy = ir_allocate(module, len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

IrGlobal *ir_global_new(IrModule *module, IrGlobalKind kind, const char *name) {
    IrGlobal *global = ir_allocate(module, sizeof(IrGlobal));
    global->index = module->globals.count;
    global->kind  = kind;
    global->name  = ir_strdup(module, name);
    global->align = 1;
    ir_array_push(module->globals, global);
    return global;
}

IrGlobal *ir_global_data(IrModule *module, const char *debug_name, const void *bytes, int size, int align, bool readonly) {
    // Data globals get a generated symbol so they can never clash with user symbols
    char name[128];
    snprintf(name, sizeof(name), "__data%d%s%s", module->globals.count, debug_name ? "_" : "", debug_name ? debug_name : "");

    // Only keep characters that are valid in an assembler label
    for (char *c = name; *c; c++) {
        bool ok = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') || *c == '_';
        if (!ok) *c = '_';
    }

    IrGlobal *global = ir_global_new(module, IR_GLOBAL_DATA, name);
    global->size     = size;
    global->align    = align > 0 ? align : 1;
    global->readonly = readonly;
    global->data     = ir_allocate(module, size > 0 ? size : 1);
    if (bytes) memcpy(global->data, bytes, size);
    return global;
}

int ir_find_or_add_extern(IrModule *module, const char *name) {
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        if (global->kind != IR_GLOBAL_DATA && strcmp(global->name, name) == 0) {
            return i;
        }
    }
    return ir_global_new(module, IR_GLOBAL_EXTERN, name)->index;
}

IrFunction *ir_function_new(IrModule *module, const char *name) {
    IrFunction *func = ir_allocate(module, sizeof(IrFunction));
    func->name = ir_strdup(module, name);
    func->return_type = IR_VOID;

    // The function symbol may already exist as an extern (e.g it was referenced before being defined)
    IrGlobal *global = NULL;
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *g = module->globals.items[i];
        if (g->kind == IR_GLOBAL_EXTERN && strcmp(g->name, name) == 0) {
            global = g;
            break;
        }
    }
    if (global == NULL) {
        global = ir_global_new(module, IR_GLOBAL_FUNCTION, name);
    }
    global->kind     = IR_GLOBAL_FUNCTION;
    global->function = func;
    func->global     = global->index;

    ir_array_push(module->functions, func);
    return func;
}

int ir_new_vreg(IrFunction *func, IrType type) {
    assert(type != IR_VOID);
    int vreg = func->vreg_types.count;
    ir_array_push(func->vreg_types, type);
    return vreg;
}

IrType ir_vreg_type(IrFunction *func, int vreg) {
    if (vreg < 0 || vreg >= func->vreg_types.count) return IR_VOID;
    return func->vreg_types.items[vreg];
}

int ir_new_slot(IrFunction *func, int size, int align, const char *name) {
    IrSlot slot = {0};
    slot.size  = size > 0 ? size : 1;
    slot.align = align > 0 ? align : 1;
    slot.name  = (char *) name;
    int index = func->slots.count;
    ir_array_push(func->slots, slot);
    return index;
}

IrBlock *ir_block_new(IrFunction *func, const char *name) {
    IrBlock *block = calloc(1, sizeof(IrBlock));
    block->id   = func->next_block_id++;
    block->name = (char *) name;
    block->term.cond  = IR_NO_VREG;
    block->term.value = IR_NO_VREG;
    return block;
}

int ir_block_successor_count(IrBlock *block) {
    switch (block->term.kind) {
    case IR_TERM_JMP: return 1;
    case IR_TERM_BR:  return 2;
    default:          return 0;
    }
}

IrBlock *ir_block_successor(IrBlock *block, int i) {
    if (i == 0) return block->term.target;
    return block->term.else_target;
}

void ir_function_finalize(IrFunction *func) {
    if (func->blocks.count == 0) return;

    // Mark every block reachable from the entry block
    int max_id = func->next_block_id;
    bool *reachable = calloc(max_id, sizeof(bool));

    IrArray(IrBlock *) stack = {0};
    ir_array_push(stack, func->blocks.items[0]);
    reachable[func->blocks.items[0]->id] = true;

    while (stack.count > 0) {
        IrBlock *block = stack.items[--stack.count];
        for (int i = 0; i < ir_block_successor_count(block); i++) {
            IrBlock *succ = ir_block_successor(block, i);
            if (succ && !reachable[succ->id]) {
                reachable[succ->id] = true;
                ir_array_push(stack, succ);
            }
        }
    }

    // Keep the layout order but drop unreachable blocks
    int kept = 0;
    for (int i = 0; i < func->blocks.count; i++) {
        IrBlock *block = func->blocks.items[i];
        if (reachable[block->id]) {
            block->index = kept;
            block->preds.count = 0;
            func->blocks.items[kept++] = block;
        }
    }
    func->blocks.count = kept;

    // Recompute predecessors
    for (int i = 0; i < func->blocks.count; i++) {
        IrBlock *block = func->blocks.items[i];
        for (int j = 0; j < ir_block_successor_count(block); j++) {
            IrBlock *succ = ir_block_successor(block, j);
            if (succ) ir_array_push(succ->preds, block);
        }
    }

    free(stack.items);
    free(reachable);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Uses / definitions
//
// Every pass that needs to know which vregs an instruction reads goes through these, so
// the rules only live in one place.
//////////////////////////////////////////////////////////////////////////////////////////

int ir_inst_use_count(IrInst *inst) {
    int count = 0;
    if (inst->a != IR_NO_VREG) count++;
    if (inst->b != IR_NO_VREG) count++;
    if ((inst->op == IR_LOAD || inst->op == IR_STORE) && inst->addr.kind == IR_ADDR_VREG) count++;
    if (inst->op == IR_CALL) {
        if (inst->call->callee_kind == IR_CALLEE_VREG) count++;
        count += inst->call->arg_count;
    }
    return count;
}

int ir_inst_use(IrInst *inst, int i) {
    if (inst->a != IR_NO_VREG) { if (i == 0) return inst->a; i--; }
    if (inst->b != IR_NO_VREG) { if (i == 0) return inst->b; i--; }
    if ((inst->op == IR_LOAD || inst->op == IR_STORE) && inst->addr.kind == IR_ADDR_VREG) {
        if (i == 0) return inst->addr.base;
        i--;
    }
    if (inst->op == IR_CALL) {
        if (inst->call->callee_kind == IR_CALLEE_VREG) {
            if (i == 0) return inst->call->callee;
            i--;
        }
        if (i < inst->call->arg_count) return inst->call->args[i];
    }
    return IR_NO_VREG;
}

bool ir_inst_is_call_like(IrInst *inst) {
    return inst->op == IR_CALL || inst->op == IR_MEMCPY || inst->op == IR_MEMSET;
}

int ir_term_use_count(IrTerm *term) {
    if (term->kind == IR_TERM_BR) return 1;
    if (term->kind == IR_TERM_RET && term->value != IR_NO_VREG) return 1;
    return 0;
}

int ir_term_use(IrTerm *term, int i) {
    (void) i;
    if (term->kind == IR_TERM_BR) return term->cond;
    return term->value;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Bitsets + liveness
//////////////////////////////////////////////////////////////////////////////////////////

IrBitset ir_bitset_new(int bit_count) {
    IrBitset set = {0};
    set.word_count = (bit_count + 63) / 64;
    if (set.word_count == 0) set.word_count = 1;
    set.words = calloc(set.word_count, sizeof(uint64_t));
    return set;
}

bool ir_bitset_get(IrBitset set, int bit)   { return (set.words[bit / 64] >> (bit % 64)) & 1; }
void ir_bitset_set(IrBitset set, int bit)   { set.words[bit / 64] |=  ((uint64_t)1 << (bit % 64)); }
void ir_bitset_clear(IrBitset set, int bit) { set.words[bit / 64] &= ~((uint64_t)1 << (bit % 64)); }

bool ir_bitset_union_into(IrBitset dst, IrBitset src) {
    bool changed = false;
    for (int i = 0; i < dst.word_count; i++) {
        uint64_t merged = dst.words[i] | src.words[i];
        if (merged != dst.words[i]) {
            dst.words[i] = merged;
            changed = true;
        }
    }
    return changed;
}

void ir_bitset_copy(IrBitset dst, IrBitset src) {
    memcpy(dst.words, src.words, dst.word_count * sizeof(uint64_t));
}

IrLiveness ir_compute_liveness(IrFunction *func) {
    //   live_out[B] = union of live_in[S] for every successor S of B
    //   live_in[B]  = use[B] | (live_out[B] - def[B])
    int block_count = func->blocks.count;
    int vreg_count  = func->vreg_types.count;

    IrLiveness live = {0};
    live.block_count = block_count;
    live.live_in  = calloc(block_count, sizeof(IrBitset));
    live.live_out = calloc(block_count, sizeof(IrBitset));

    IrBitset *uses = calloc(block_count, sizeof(IrBitset));
    IrBitset *defs = calloc(block_count, sizeof(IrBitset));

    for (int b = 0; b < block_count; b++) {
        IrBlock *block = func->blocks.items[b];
        live.live_in[b]  = ir_bitset_new(vreg_count);
        live.live_out[b] = ir_bitset_new(vreg_count);
        uses[b] = ir_bitset_new(vreg_count);
        defs[b] = ir_bitset_new(vreg_count);

        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            for (int u = 0; u < ir_inst_use_count(inst); u++) {
                int vreg = ir_inst_use(inst, u);
                if (!ir_bitset_get(defs[b], vreg)) ir_bitset_set(uses[b], vreg);
            }
            if (inst->dst != IR_NO_VREG) ir_bitset_set(defs[b], inst->dst);
        }
        for (int u = 0; u < ir_term_use_count(&block->term); u++) {
            int vreg = ir_term_use(&block->term, u);
            if (!ir_bitset_get(defs[b], vreg)) ir_bitset_set(uses[b], vreg);
        }
    }

    IrBitset scratch = ir_bitset_new(vreg_count);

    bool changed = true;
    while (changed) {
        changed = false;
        for (int b = block_count - 1; b >= 0; b--) {
            IrBlock *block = func->blocks.items[b];

            for (int s = 0; s < ir_block_successor_count(block); s++) {
                IrBlock *succ = ir_block_successor(block, s);
                if (ir_bitset_union_into(live.live_out[b], live.live_in[succ->index])) changed = true;
            }

            // live_in = use | (live_out & ~def)
            for (int w = 0; w < scratch.word_count; w++) {
                scratch.words[w] = uses[b].words[w] | (live.live_out[b].words[w] & ~defs[b].words[w]);
            }
            if (ir_bitset_union_into(live.live_in[b], scratch)) changed = true;
        }
    }

    for (int b = 0; b < block_count; b++) {
        free(uses[b].words);
        free(defs[b].words);
    }
    free(uses);
    free(defs);
    free(scratch.words);

    return live;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Dump
//////////////////////////////////////////////////////////////////////////////////////////

static void ir_dump_vreg(IrFunction *func, StringBuilder *sb, int vreg) {
    if (vreg == IR_NO_VREG) {
        sb_append(sb, "<none>");
    } else {
        sb_append(sb, "v%d", vreg);
    }
    (void) func;
}

static void ir_dump_addr(IrModule *module, IrFunction *func, StringBuilder *sb, IrAddr addr) {
    switch (addr.kind) {
    case IR_ADDR_VREG:   sb_append(sb, "[v%d", addr.base); break;
    case IR_ADDR_SLOT:   sb_append(sb, "[s%d", addr.base); break;
    case IR_ADDR_GLOBAL: sb_append(sb, "[%s", module->globals.items[addr.base]->name); break;
    default:             sb_append(sb, "[???"); break;
    }
    if (addr.offset != 0) sb_append(sb, "%+d", addr.offset);
    sb_append(sb, "]");
    (void) func;
}

static void ir_dump_inst(IrModule *module, IrFunction *func, StringBuilder *sb, IrInst *inst) {
    sb_append(sb, "    ");
    if (inst->dst != IR_NO_VREG) {
        sb_append(sb, "v%d:%s = ", inst->dst, ir_type_name(ir_vreg_type(func, inst->dst)));
    }

    switch (inst->op) {
    case IR_CONST:
        sb_append(sb, "const %lld", (long long) inst->imm);
        break;
    case IR_CONST_FLOAT:
        sb_append(sb, "fconst %.17g", inst->fimm);
        break;
    case IR_SLOT_ADDR:
        sb_append(sb, "slot_addr s%lld", (long long) inst->imm);
        break;
    case IR_GLOBAL_ADDR:
        sb_append(sb, "global_addr %s", module->globals.items[inst->imm]->name);
        break;
    case IR_LOAD:
        sb_append(sb, "load.%s ", ir_type_name(inst->type));
        ir_dump_addr(module, func, sb, inst->addr);
        break;
    case IR_STORE:
        sb_append(sb, "store.%s ", ir_type_name(inst->type));
        ir_dump_addr(module, func, sb, inst->addr);
        sb_append(sb, ", v%d", inst->a);
        break;
    case IR_MEMCPY:
        sb_append(sb, "memcpy v%d, v%d, %lld", inst->a, inst->b, (long long) inst->imm);
        break;
    case IR_MEMSET:
        sb_append(sb, "memset v%d, %lld, %lld", inst->a, (long long) inst->imm2, (long long) inst->imm);
        break;
    case IR_ICMP:
    case IR_FCMP:
        sb_append(sb, "%s %s.%s v%d, v%d", ir_op_name(inst->op), ir_cmp_name(inst->cmp), ir_type_name(inst->type), inst->a, inst->b);
        break;
    case IR_CALL: {
        IrCall *call = inst->call;
        sb_append(sb, "call ");
        if (call->callee_kind == IR_CALLEE_DIRECT) {
            sb_append(sb, "%s(", module->globals.items[call->callee]->name);
        } else {
            sb_append(sb, "[v%d](", call->callee);
        }
        for (int i = 0; i < call->arg_count; i++) {
            if (i > 0) sb_append(sb, ", ");
            sb_append(sb, "%sv%d", call->arg_is_vararg && call->arg_is_vararg[i] ? "..." : "", call->args[i]);
        }
        sb_append(sb, ")");
        break;
    }
    default: {
        sb_append(sb, "%s", ir_op_name(inst->op));
        if (inst->a != IR_NO_VREG) { sb_append(sb, " "); ir_dump_vreg(func, sb, inst->a); }
        if (inst->b != IR_NO_VREG) { sb_append(sb, ", "); ir_dump_vreg(func, sb, inst->b); }
        break;
    }
    }

    if (inst->line > 0) sb_append(sb, "    ; line %d", inst->line);
    sb_append(sb, "\n");
}

void ir_dump_function(IrModule *module, IrFunction *func, StringBuilder *sb) {
    sb_append(sb, "func %s(", func->name);
    for (int i = 0; i < func->params.count; i++) {
        int vreg = func->params.items[i];
        if (i > 0) sb_append(sb, ", ");
        sb_append(sb, "v%d:%s", vreg, ir_type_name(ir_vreg_type(func, vreg)));
    }
    sb_append(sb, ") -> %s {\n", ir_type_name(func->return_type));

    for (int i = 0; i < func->slots.count; i++) {
        IrSlot *slot = &func->slots.items[i];
        sb_append(sb, "    s%d: %d bytes, align %d%s%s\n", i, slot->size, slot->align, slot->name ? "  ; " : "", slot->name ? slot->name : "");
    }

    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        sb_append(sb, "\nL%d:%s%s\n", block->id, block->name ? "    ; " : "", block->name ? block->name : "");

        for (int i = 0; i < block->insts.count; i++) {
            ir_dump_inst(module, func, sb, &block->insts.items[i]);
        }

        IrTerm *term = &block->term;
        switch (term->kind) {
        case IR_TERM_NONE: sb_append(sb, "    <no terminator>\n"); break;
        case IR_TERM_JMP:  sb_append(sb, "    jmp L%d\n", term->target->id); break;
        case IR_TERM_BR:   sb_append(sb, "    br v%d, L%d, L%d\n", term->cond, term->target->id, term->else_target->id); break;
        case IR_TERM_RET:
            if (term->value != IR_NO_VREG) sb_append(sb, "    ret v%d\n", term->value);
            else                           sb_append(sb, "    ret\n");
            break;
        }
    }

    sb_append(sb, "}\n\n");
}

void ir_dump_module(IrModule *module, StringBuilder *sb) {
    for (int i = 0; i < module->globals.count; i++) {
        IrGlobal *global = module->globals.items[i];
        switch (global->kind) {
        case IR_GLOBAL_EXTERN:
            sb_append(sb, "extern %s\n", global->name);
            break;
        case IR_GLOBAL_FUNCTION:
            break;
        case IR_GLOBAL_DATA: {
            sb_append(sb, "%s %s: %d bytes, align %d = [", global->readonly ? "const" : "data", global->name, global->size, global->align);
            int shown = global->size < 32 ? global->size : 32;
            for (int b = 0; b < shown; b++) sb_append(sb, "%s%02x", b ? " " : "", global->data[b]);
            if (shown < global->size) sb_append(sb, " ...");
            sb_append(sb, "]");
            for (int r = 0; r < global->relocs.count; r++) {
                sb_append(sb, " @%d->%s", global->relocs.items[r].offset, module->globals.items[global->relocs.items[r].target]->name);
            }
            sb_append(sb, "\n");
            break;
        }
        }
    }
    sb_append(sb, "\n");

    for (int i = 0; i < module->functions.count; i++) {
        ir_dump_function(module, module->functions.items[i], sb);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Verifier
//
// Catches lowering bugs at compile time instead of as miscompiled programs. Every rule of
// the IR described in ir.h is checked here.
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct IrVerifier {
    IrModule   *module;
    IrFunction *func;
    IrBlock    *block;
    int         errors;
} IrVerifier;

static void ir_verify_error(IrVerifier *v, IrInst *inst, const char *format, ...) {
    printf("IR verifier error in '%s'", v->func->name);
    if (v->block) printf(", block L%d", v->block->id);
    if (inst) {
        StringBuilder sb = sb_init(128);
        ir_dump_inst(v->module, v->func, &sb, inst);
        printf(", at instruction:\n%s", sb_to_string(&sb));
        sb_free(&sb);
    } else {
        printf("\n");
    }
    printf("    -> ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    v->errors++;
}

static IrType vtype(IrVerifier *v, int vreg) { return ir_vreg_type(v->func, vreg); }

static void ir_verify_operand(IrVerifier *v, IrInst *inst, int vreg, IrType expected, const char *what) {
    if (vreg == IR_NO_VREG) {
        ir_verify_error(v, inst, "missing %s operand", what);
        return;
    }
    if (vreg < 0 || vreg >= v->func->vreg_types.count) {
        ir_verify_error(v, inst, "%s operand v%d does not exist", what, vreg);
        return;
    }
    if (expected != IR_VOID && vtype(v, vreg) != expected) {
        ir_verify_error(v, inst, "%s operand v%d has type %s, expected %s", what, vreg, ir_type_name(vtype(v, vreg)), ir_type_name(expected));
    }
}

static void ir_verify_addr(IrVerifier *v, IrInst *inst) {
    IrAddr addr = inst->addr;
    switch (addr.kind) {
    case IR_ADDR_VREG:   ir_verify_operand(v, inst, addr.base, IR_PTR, "address"); break;
    case IR_ADDR_SLOT:
        if (addr.base < 0 || addr.base >= v->func->slots.count) ir_verify_error(v, inst, "slot s%d does not exist", addr.base);
        break;
    case IR_ADDR_GLOBAL:
        if (addr.base < 0 || addr.base >= v->module->globals.count) ir_verify_error(v, inst, "global %d does not exist", addr.base);
        break;
    default:
        ir_verify_error(v, inst, "invalid address");
    }
}

static void ir_verify_inst(IrVerifier *v, IrInst *inst) {
    IrType type = inst->type;

    bool needs_dst = !(inst->op == IR_STORE || inst->op == IR_MEMCPY || inst->op == IR_MEMSET || inst->op == IR_NOP || inst->op == IR_CALL);
    if (needs_dst && inst->dst == IR_NO_VREG) {
        ir_verify_error(v, inst, "instruction needs a destination");
        return;
    }
    if (inst->dst != IR_NO_VREG && inst->op != IR_ICMP && inst->op != IR_FCMP && inst->op != IR_CALL) {
        if (vtype(v, inst->dst) != type) {
            ir_verify_error(v, inst, "destination has type %s but the instruction produces %s", ir_type_name(vtype(v, inst->dst)), ir_type_name(type));
        }
    }

    switch (inst->op) {
    case IR_NOP: break;
    case IR_CONST:
        if (!ir_type_is_int_or_ptr(type)) ir_verify_error(v, inst, "const needs an integer or pointer type");
        break;
    case IR_CONST_FLOAT:
        if (!ir_type_is_float(type)) ir_verify_error(v, inst, "fconst needs a float type");
        break;
    case IR_COPY:
        ir_verify_operand(v, inst, inst->a, type, "source");
        break;
    case IR_SLOT_ADDR:
        if (type != IR_PTR) ir_verify_error(v, inst, "slot_addr produces a ptr");
        if (inst->imm < 0 || inst->imm >= v->func->slots.count) ir_verify_error(v, inst, "slot does not exist");
        break;
    case IR_GLOBAL_ADDR:
        if (type != IR_PTR) ir_verify_error(v, inst, "global_addr produces a ptr");
        if (inst->imm < 0 || inst->imm >= v->module->globals.count) ir_verify_error(v, inst, "global does not exist");
        break;
    case IR_LOAD:
        if (type == IR_VOID) ir_verify_error(v, inst, "load of void (aggregates must be copied with memcpy)");
        ir_verify_addr(v, inst);
        break;
    case IR_STORE:
        if (type == IR_VOID) ir_verify_error(v, inst, "store of void (aggregates must be copied with memcpy)");
        ir_verify_operand(v, inst, inst->a, type, "stored value");
        ir_verify_addr(v, inst);
        break;
    case IR_PTR_ADD:
        if (type != IR_PTR) ir_verify_error(v, inst, "ptradd produces a ptr");
        ir_verify_operand(v, inst, inst->a, IR_PTR, "base");
        ir_verify_operand(v, inst, inst->b, IR_I64, "offset");
        break;
    case IR_MEMCPY:
        ir_verify_operand(v, inst, inst->a, IR_PTR, "destination");
        ir_verify_operand(v, inst, inst->b, IR_PTR, "source");
        if (inst->imm <= 0) ir_verify_error(v, inst, "memcpy of %lld bytes", (long long) inst->imm);
        break;
    case IR_MEMSET:
        ir_verify_operand(v, inst, inst->a, IR_PTR, "destination");
        if (inst->imm <= 0) ir_verify_error(v, inst, "memset of %lld bytes", (long long) inst->imm);
        break;
    case IR_ADD: case IR_SUB: case IR_MUL:
    case IR_SDIV: case IR_UDIV: case IR_SREM: case IR_UREM:
    case IR_AND: case IR_OR: case IR_XOR:
    case IR_SHL: case IR_LSHR: case IR_ASHR:
        if (!ir_type_is_int(type)) ir_verify_error(v, inst, "%s needs an integer type (use ptradd for pointers)", ir_op_name(inst->op));
        ir_verify_operand(v, inst, inst->a, type, "left");
        ir_verify_operand(v, inst, inst->b, type, "right");
        break;
    case IR_NEG: case IR_NOT:
        if (!ir_type_is_int(type)) ir_verify_error(v, inst, "%s needs an integer type", ir_op_name(inst->op));
        ir_verify_operand(v, inst, inst->a, type, "operand");
        break;
    case IR_FADD: case IR_FSUB: case IR_FMUL: case IR_FDIV:
        if (!ir_type_is_float(type)) ir_verify_error(v, inst, "%s needs a float type", ir_op_name(inst->op));
        ir_verify_operand(v, inst, inst->a, type, "left");
        ir_verify_operand(v, inst, inst->b, type, "right");
        break;
    case IR_FNEG:
        if (!ir_type_is_float(type)) ir_verify_error(v, inst, "fneg needs a float type");
        ir_verify_operand(v, inst, inst->a, type, "operand");
        break;
    case IR_ICMP:
        if (!ir_cmp_is_int(inst->cmp)) ir_verify_error(v, inst, "icmp with a non-integer predicate");
        if (!ir_type_is_int_or_ptr(type)) ir_verify_error(v, inst, "icmp needs integer or pointer operands");
        if (vtype(v, inst->dst) != IR_I8) ir_verify_error(v, inst, "icmp produces an i8");
        ir_verify_operand(v, inst, inst->a, type, "left");
        ir_verify_operand(v, inst, inst->b, type, "right");
        break;
    case IR_FCMP:
        if (!ir_cmp_is_float(inst->cmp)) ir_verify_error(v, inst, "fcmp with a non-float predicate");
        if (!ir_type_is_float(type)) ir_verify_error(v, inst, "fcmp needs float operands");
        if (vtype(v, inst->dst) != IR_I8) ir_verify_error(v, inst, "fcmp produces an i8");
        ir_verify_operand(v, inst, inst->a, type, "left");
        ir_verify_operand(v, inst, inst->b, type, "right");
        break;
    case IR_SEXT: case IR_ZEXT: case IR_TRUNC:
    case IR_SITOFP: case IR_UITOFP: case IR_FPTOSI: case IR_FPTOUI:
    case IR_FPEXT: case IR_FPTRUNC: case IR_BITCAST: {
        ir_verify_operand(v, inst, inst->a, IR_VOID, "source");
        IrType from = vtype(v, inst->a);
        int fs = ir_type_size(from), ts = ir_type_size(type);
        bool ok = false;
        switch (inst->op) {
        case IR_SEXT: case IR_ZEXT: ok = ir_type_is_int(from) && ir_type_is_int(type) && fs < ts; break;
        case IR_TRUNC:              ok = ir_type_is_int(from) && ir_type_is_int(type) && fs > ts; break;
        case IR_SITOFP: case IR_UITOFP: ok = ir_type_is_int(from) && ir_type_is_float(type); break;
        case IR_FPTOSI: case IR_FPTOUI: ok = ir_type_is_float(from) && ir_type_is_int(type); break;
        case IR_FPEXT:   ok = from == IR_F32 && type == IR_F64; break;
        case IR_FPTRUNC: ok = from == IR_F64 && type == IR_F32; break;
        case IR_BITCAST: ok = fs == ts && from != type; break;
        default: break;
        }
        if (!ok) ir_verify_error(v, inst, "invalid conversion %s -> %s", ir_type_name(from), ir_type_name(type));
        break;
    }
    case IR_CALL: {
        IrCall *call = inst->call;
        if (call->callee_kind == IR_CALLEE_VREG) {
            ir_verify_operand(v, inst, call->callee, IR_PTR, "callee");
        } else if (call->callee < 0 || call->callee >= v->module->globals.count || v->module->globals.items[call->callee]->kind == IR_GLOBAL_DATA) {
            ir_verify_error(v, inst, "call to something that is not a function symbol");
        }
        for (int i = 0; i < call->arg_count; i++) {
            ir_verify_operand(v, inst, call->args[i], IR_VOID, "argument");
        }
        if (call->return_type == IR_VOID && inst->dst != IR_NO_VREG) ir_verify_error(v, inst, "void call with a destination");
        if (inst->dst != IR_NO_VREG && vtype(v, inst->dst) != call->return_type) ir_verify_error(v, inst, "call destination type does not match the return type");
        break;
    }
    default:
        ir_verify_error(v, inst, "unknown instruction");
    }
}

static bool ir_verify_function(IrModule *module, IrFunction *func) {
    IrVerifier v = { .module = module, .func = func };

    if (func->blocks.count == 0) {
        ir_verify_error(&v, NULL, "function has no blocks");
        return false;
    }

    int vreg_count = func->vreg_types.count;
    int *def_count = calloc(vreg_count > 0 ? vreg_count : 1, sizeof(int));

    for (int i = 0; i < func->params.count; i++) {
        int vreg = func->params.items[i];
        if (vreg < 0 || vreg >= vreg_count) {
            ir_verify_error(&v, NULL, "parameter %d refers to a non-existing vreg", i);
            continue;
        }
        def_count[vreg]++;
    }

    for (int b = 0; b < func->blocks.count; b++) {
        IrBlock *block = func->blocks.items[b];
        v.block = block;

        if (block->index != b) ir_verify_error(&v, NULL, "block index is out of date (did you call ir_function_finalize?)");

        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            ir_verify_inst(&v, inst);
            if (inst->dst != IR_NO_VREG && inst->dst >= 0 && inst->dst < vreg_count) def_count[inst->dst]++;
        }

        IrTerm *term = &block->term;
        switch (term->kind) {
        case IR_TERM_NONE:
            ir_verify_error(&v, NULL, "block has no terminator");
            break;
        case IR_TERM_JMP:
            if (!term->target || !term->target->is_placed) ir_verify_error(&v, NULL, "jmp to a block that is not part of the function");
            break;
        case IR_TERM_BR:
            if (!term->target || !term->else_target || !term->target->is_placed || !term->else_target->is_placed) {
                ir_verify_error(&v, NULL, "br to a block that is not part of the function");
            }
            if (term->cond == IR_NO_VREG || ir_vreg_type(func, term->cond) != IR_I8) ir_verify_error(&v, NULL, "br condition must be an i8");
            break;
        case IR_TERM_RET:
            if (func->return_type == IR_VOID && term->value != IR_NO_VREG) ir_verify_error(&v, NULL, "ret with a value in a void function");
            if (func->return_type != IR_VOID && (term->value == IR_NO_VREG || ir_vreg_type(func, term->value) != func->return_type)) {
                ir_verify_error(&v, NULL, "ret value must have type %s", ir_type_name(func->return_type));
            }
            break;
        }
    }
    v.block = NULL;

    for (int vreg = 0; vreg < vreg_count; vreg++) {
        if (def_count[vreg] > 1) ir_verify_error(&v, NULL, "v%d is defined %d times (vregs must be defined exactly once)", vreg, def_count[vreg]);
    }

    // Every use must refer to a defined vreg, and within a block the definition must come first
    if (v.errors == 0) {
        for (int b = 0; b < func->blocks.count; b++) {
            IrBlock *block = func->blocks.items[b];
            v.block = block;
            for (int i = 0; i < block->insts.count; i++) {
                IrInst *inst = &block->insts.items[i];
                for (int u = 0; u < ir_inst_use_count(inst); u++) {
                    int vreg = ir_inst_use(inst, u);
                    if (def_count[vreg] == 0) ir_verify_error(&v, inst, "v%d is used but never defined", vreg);
                }
            }
            for (int u = 0; u < ir_term_use_count(&block->term); u++) {
                int vreg = ir_term_use(&block->term, u);
                if (vreg < 0 || vreg >= vreg_count || def_count[vreg] == 0) ir_verify_error(&v, NULL, "terminator uses v%d which is never defined", vreg);
            }
        }
        v.block = NULL;
    }

    // A vreg that is live into the entry block is used on some path before being defined
    if (v.errors == 0) {
        IrLiveness live = ir_compute_liveness(func);
        for (int i = 0; i < func->params.count; i++) ir_bitset_clear(live.live_in[0], func->params.items[i]);
        for (int vreg = 0; vreg < vreg_count; vreg++) {
            if (ir_bitset_get(live.live_in[0], vreg)) {
                ir_verify_error(&v, NULL, "v%d may be used before it is defined", vreg);
            }
        }
    }

    free(def_count);
    return v.errors == 0;
}

bool ir_verify_module(IrModule *module) {
    bool ok = true;
    for (int i = 0; i < module->functions.count; i++) {
        if (!ir_verify_function(module, module->functions.items[i])) ok = false;
    }
    return ok;
}
