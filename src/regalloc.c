//////////////////////////////////////////////////////////////////////////////////////////
//
// Linear scan register allocation (Poletto & Sarkar)
//
// Target independent: the registers come from a TargetDesc. The IR is never rewritten;
// the result is a location (register or spill slot) for every vreg, which the backend
// uses while emitting code.
//
//   1. Number instructions in block layout order (positions step by 2)
//   2. Liveness (ir_compute_liveness) -> one conservative [start, end] interval per vreg
//   3. Intervals that span a call may only use callee-saved registers
//   4. Linear scan over the intervals sorted by start. When out of registers, spill the
//      interval that ends last.
//
// Invariant the backends rely on: an interval is only expired when its end is strictly
// before the start of the new interval. So the destination of an instruction never shares
// a register with any of that instruction's operands, which makes two-address code
// (`mov dst, a; sub dst, b`) safe.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "ir.h"

#define REG_CLASS_GPR 0
#define REG_CLASS_FPR 1
#define REG_CLASS_COUNT 2
#define TARGET_MAX_REGS 64

typedef struct TargetDesc {
    const char *name;
    const char *(*register_name)(int reg);   // For dumps
    int  allocatable[REG_CLASS_COUNT][TARGET_MAX_REGS]; // In order of preference
    int  allocatable_count[REG_CLASS_COUNT];
    bool is_callee_saved[TARGET_MAX_REGS];
} TargetDesc;

typedef enum LocationKind {
    LOCATION_NONE,
    LOCATION_REG,
    LOCATION_SPILL,
} LocationKind;

typedef struct Location {
    LocationKind kind;
    int          reg;         // LOCATION_REG
    int          spill_slot;  // LOCATION_SPILL. Spill slots are 8 bytes each
} Location;

typedef struct LiveInterval {
    int  vreg;
    int  start;
    int  end;
    int  reg_class;
    bool crosses_call;
} LiveInterval;

typedef struct RegAllocResult {
    int           vreg_count;
    Location     *locations;          // Indexed by vreg
    LiveInterval *intervals;          // Indexed by vreg (start > end means the vreg is never used)
    int           spill_slot_count;
    bool          used_callee_saved[TARGET_MAX_REGS];
    int          *inst_positions;     // Position of every instruction, in block layout order
} RegAllocResult;

static int reg_class_of(IrType type) {
    return ir_type_is_float(type) ? REG_CLASS_FPR : REG_CLASS_GPR;
}

static void interval_extend(LiveInterval *interval, int pos) {
    if (pos < interval->start) interval->start = pos;
    if (pos > interval->end)   interval->end   = pos;
}

static LiveInterval *sort_intervals_by;

static int compare_intervals_by_start(const void *a, const void *b) {
    const LiveInterval *x = &sort_intervals_by[*(const int *) a];
    const LiveInterval *y = &sort_intervals_by[*(const int *) b];
    if (x->start != y->start) return x->start - y->start;
    return x->vreg - y->vreg;
}

RegAllocResult regalloc_function(IrFunction *func, TargetDesc *target) {
    RegAllocResult result = {0};
    int vreg_count = func->vreg_types.count;
    result.vreg_count = vreg_count;
    result.locations  = calloc(vreg_count > 0 ? vreg_count : 1, sizeof(Location));
    result.intervals  = calloc(vreg_count > 0 ? vreg_count : 1, sizeof(LiveInterval));

    for (int v = 0; v < vreg_count; v++) {
        result.intervals[v].vreg      = v;
        result.intervals[v].start     = INT32_MAX;
        result.intervals[v].end       = -1;
        result.intervals[v].reg_class = reg_class_of(func->vreg_types.items[v]);
    }

    //
    // 1. Numbering. Position 0 is the function entry where the parameters are defined
    //
    int block_count = func->blocks.count;
    int *block_start = calloc(block_count > 0 ? block_count : 1, sizeof(int));
    int *block_end   = calloc(block_count > 0 ? block_count : 1, sizeof(int));

    int inst_count = 0;
    for (int b = 0; b < block_count; b++) inst_count += func->blocks.items[b]->insts.count;
    result.inst_positions = calloc(inst_count > 0 ? inst_count : 1, sizeof(int));

    IrArray(int) call_positions = {0};

    int pos = 2;
    int inst_index = 0;
    for (int b = 0; b < block_count; b++) {
        IrBlock *block = func->blocks.items[b];
        block_start[b] = pos;
        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            result.inst_positions[inst_index++] = pos;
            if (ir_inst_is_call_like(inst)) ir_array_push(call_positions, pos);
            pos += 2;
        }
        block_end[b] = pos; // The terminator
        pos += 2;
    }

    //
    // 2. Intervals
    //
    IrLiveness live = ir_compute_liveness(func);

    for (int i = 0; i < func->params.count; i++) {
        interval_extend(&result.intervals[func->params.items[i]], 0);
    }

    inst_index = 0;
    for (int b = 0; b < block_count; b++) {
        IrBlock *block = func->blocks.items[b];

        for (int v = 0; v < vreg_count; v++) {
            if (ir_bitset_get(live.live_in[b], v))  interval_extend(&result.intervals[v], block_start[b]);
            if (ir_bitset_get(live.live_out[b], v)) interval_extend(&result.intervals[v], block_end[b]);
        }

        for (int i = 0; i < block->insts.count; i++) {
            IrInst *inst = &block->insts.items[i];
            int inst_pos = result.inst_positions[inst_index++];
            for (int u = 0; u < ir_inst_use_count(inst); u++) {
                interval_extend(&result.intervals[ir_inst_use(inst, u)], inst_pos);
            }
            if (inst->dst != IR_NO_VREG) interval_extend(&result.intervals[inst->dst], inst_pos);
        }
        for (int u = 0; u < ir_term_use_count(&block->term); u++) {
            interval_extend(&result.intervals[ir_term_use(&block->term, u)], block_end[b]);
        }
    }

    //
    // 3. Intervals spanning a call (a value that is live both before and after it)
    //
    for (int v = 0; v < vreg_count; v++) {
        LiveInterval *interval = &result.intervals[v];
        for (int c = 0; c < call_positions.count; c++) {
            int call_pos = call_positions.items[c];
            if (interval->start < call_pos && call_pos < interval->end) {
                interval->crosses_call = true;
                break;
            }
        }
    }

    //
    // 4. Linear scan
    //
    int *order = calloc(vreg_count > 0 ? vreg_count : 1, sizeof(int));
    int order_count = 0;
    for (int v = 0; v < vreg_count; v++) {
        if (result.intervals[v].start <= result.intervals[v].end) order[order_count++] = v;
    }
    sort_intervals_by = result.intervals;
    qsort(order, order_count, sizeof(int), compare_intervals_by_start);

    bool reg_free[TARGET_MAX_REGS];
    for (int r = 0; r < TARGET_MAX_REGS; r++) reg_free[r] = true;

    IrArray(int) active = {0}; // vregs currently holding a register

    for (int k = 0; k < order_count; k++) {
        int v = order[k];
        LiveInterval *interval = &result.intervals[v];
        int cls = interval->reg_class;

        // Expire intervals that ended strictly before this one starts
        for (int a = 0; a < active.count; ) {
            int other = active.items[a];
            if (result.intervals[other].end < interval->start) {
                reg_free[result.locations[other].reg] = true;
                active.items[a] = active.items[--active.count];
            } else {
                a++;
            }
        }

        // Pick a free register from the allowed pool
        int chosen = -1;
        for (int i = 0; i < target->allocatable_count[cls]; i++) {
            int reg = target->allocatable[cls][i];
            if (!reg_free[reg]) continue;
            if (interval->crosses_call && !target->is_callee_saved[reg]) continue;
            chosen = reg;
            break;
        }

        if (chosen >= 0) {
            reg_free[chosen] = false;
            result.locations[v] = (Location){ LOCATION_REG, chosen, -1 };
            ir_array_push(active, v);
            continue;
        }

        // Out of registers: take the register of the active interval that ends last (if it ends after us)
        int victim_index = -1;
        for (int a = 0; a < active.count; a++) {
            int other = active.items[a];
            if (result.intervals[other].reg_class != cls) continue;
            int reg = result.locations[other].reg;
            if (interval->crosses_call && !target->is_callee_saved[reg]) continue;
            if (victim_index < 0 || result.intervals[other].end > result.intervals[active.items[victim_index]].end) {
                victim_index = a;
            }
        }

        if (victim_index >= 0 && result.intervals[active.items[victim_index]].end > interval->end) {
            int victim = active.items[victim_index];
            result.locations[v] = (Location){ LOCATION_REG, result.locations[victim].reg, -1 };
            result.locations[victim] = (Location){ LOCATION_SPILL, -1, result.spill_slot_count++ };
            active.items[victim_index] = v;
        } else {
            result.locations[v] = (Location){ LOCATION_SPILL, -1, result.spill_slot_count++ };
        }
    }

    for (int v = 0; v < vreg_count; v++) {
        if (result.locations[v].kind == LOCATION_REG && target->is_callee_saved[result.locations[v].reg]) {
            result.used_callee_saved[result.locations[v].reg] = true;
        }
    }

    free(order);
    free(active.items);
    free(call_positions.items);
    free(block_start);
    free(block_end);
    return result;
}

void regalloc_dump(IrFunction *func, RegAllocResult *ra, TargetDesc *target, StringBuilder *sb) {
    sb_append(sb, "; Register allocation for '%s' (%d spill slots)\n", func->name, ra->spill_slot_count);
    for (int v = 0; v < ra->vreg_count; v++) {
        LiveInterval *interval = &ra->intervals[v];
        if (interval->start > interval->end) {
            sb_append(sb, ";   v%-4d %-4s unused\n", v, ir_type_name(func->vreg_types.items[v]));
            continue;
        }
        sb_append(sb, ";   v%-4d %-4s [%4d, %4d]%s  ", v, ir_type_name(func->vreg_types.items[v]), interval->start, interval->end, interval->crosses_call ? " crosses call" : "             ");
        Location loc = ra->locations[v];
        if (loc.kind == LOCATION_REG)        sb_append(sb, "%s\n", target->register_name(loc.reg));
        else if (loc.kind == LOCATION_SPILL) sb_append(sb, "spill %d\n", loc.spill_slot);
        else                                 sb_append(sb, "-\n");
    }
    sb_append(sb, "\n");
}
