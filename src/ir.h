#ifndef IR_H
#define IR_H

//////////////////////////////////////////////////////////////////////////////////////////
//
// Saad intermediate representation (IR)
//
// A small, target independent, three-address IR. It is the output of the AST lowering
// (ir_lower.c) and the input to every backend (x64 today; later ARM64, LLVM and the
// compile-time interpreter).
//
// Rules of the IR (checked by ir_verify):
//
//   * Values live in virtual registers (vregs). Every vreg has an IrType and is defined
//     exactly once. Function parameters count as definitions at function entry.
//
//   * Vregs only ever hold scalars: i8 i16 i32 i64 f32 f64 ptr.
//     Booleans are i8 values that are always 0 or 1.
//
//   * Anything mutable or anything bigger than a scalar (structs, strings, arrays,
//     slices ...) lives in memory: a stack slot (like an LLVM alloca) or a global.
//     Aggregates are always handled by address. Copying an aggregate is a memcpy.
//
//   * Signedness is part of the operation, not the type: sdiv/udiv, icmp slt/ult,
//     sext/zext, ...
//
//   * Narrow integers (i8, i16, i32) only have their low bits defined in a register.
//     A backend is free to leave garbage in the upper bits, so every operation whose
//     result depends on the upper bits must extend explicitly.
//
//   * Every basic block ends in exactly one terminator (jmp, br or ret).
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdbool.h>
#include "lib/arena.h"
#include "lib/string_builder.h"

//
// A tiny typed growable array used throughout the IR and the backends
//
#define IrArray(T) struct { T *items; int count; int capacity; }

#define ir_array_push(arr, item) do {                                                       \
        if ((arr).count + 1 > (arr).capacity) {                                             \
            (arr).capacity = (arr).capacity ? (arr).capacity * 2 : 8;                       \
            (arr).items = realloc((arr).items, (size_t)(arr).capacity * sizeof(*(arr).items)); \
        }                                                                                   \
        (arr).items[(arr).count++] = (item);                                                \
    } while (0)

#define IR_NO_VREG (-1)

typedef enum IrType {
    IR_VOID,
    IR_I8,
    IR_I16,
    IR_I32,
    IR_I64,
    IR_F32,
    IR_F64,
    IR_PTR,
    IR_TYPE_COUNT,
} IrType;

int         ir_type_size(IrType type);
bool        ir_type_is_int(IrType type);      // i8 .. i64 (not ptr)
bool        ir_type_is_int_or_ptr(IrType type);
bool        ir_type_is_float(IrType type);
const char *ir_type_name(IrType type);

typedef enum IrOp {
    IR_NOP,

    // Values
    IR_CONST,          // dst = imm                          (int or ptr types)
    IR_CONST_FLOAT,    // dst = fimm                         (f32 / f64)
    IR_COPY,           // dst = a
    IR_SLOT_ADDR,      // dst:ptr = &slot[imm]
    IR_GLOBAL_ADDR,    // dst:ptr = &global[imm]             (data or function symbol)

    // Memory
    IR_LOAD,           // dst = load.type [addr]
    IR_STORE,          // store.type [addr], a
    IR_PTR_ADD,        // dst:ptr = a:ptr + b:i64
    IR_MEMCPY,         // memcpy(a:ptr, b:ptr, imm bytes)
    IR_MEMSET,         // memset(a:ptr, (u8) imm2, imm bytes)

    // Integer arithmetic (both operands have the instruction type)
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_SDIV,
    IR_UDIV,
    IR_SREM,
    IR_UREM,
    IR_AND,
    IR_OR,
    IR_XOR,
    IR_SHL,
    IR_LSHR,
    IR_ASHR,
    IR_NEG,            // dst = -a
    IR_NOT,            // dst = ~a

    // Float arithmetic
    IR_FADD,
    IR_FSUB,
    IR_FMUL,
    IR_FDIV,
    IR_FNEG,

    // Comparisons. dst is always an i8 (0 or 1). 'type' is the operand type
    IR_ICMP,
    IR_FCMP,

    // Conversions. 'type' is the destination type, the source type is the type of 'a'
    IR_SEXT,
    IR_ZEXT,
    IR_TRUNC,
    IR_SITOFP,
    IR_UITOFP,
    IR_FPTOSI,
    IR_FPTOUI,
    IR_FPEXT,
    IR_FPTRUNC,
    IR_BITCAST,        // Same size reinterpret, e.g ptr <-> i64

    // Calls
    IR_CALL,           // [dst =] call callee(args...)

    IR_OP_COUNT,
} IrOp;

typedef enum IrCmp {
    IR_CMP_NONE,

    // icmp
    IR_CMP_EQ,
    IR_CMP_NE,
    IR_CMP_SLT,
    IR_CMP_SLE,
    IR_CMP_SGT,
    IR_CMP_SGE,
    IR_CMP_ULT,
    IR_CMP_ULE,
    IR_CMP_UGT,
    IR_CMP_UGE,

    // fcmp. 'O' = ordered (false if any operand is NaN), 'U' = unordered (true if any operand is NaN)
    IR_CMP_OEQ,
    IR_CMP_UNE,
    IR_CMP_OLT,
    IR_CMP_OLE,
    IR_CMP_OGT,
    IR_CMP_OGE,
} IrCmp;

typedef enum IrAddrKind {
    IR_ADDR_NONE,
    IR_ADDR_VREG,      // [vreg + offset]
    IR_ADDR_SLOT,      // [slot + offset]
    IR_ADDR_GLOBAL,    // [global + offset]
} IrAddrKind;

typedef struct IrAddr {
    IrAddrKind kind;
    int        base;   // vreg, slot index or global index depending on 'kind'
    int        offset;
} IrAddr;

typedef enum IrCalleeKind {
    IR_CALLEE_DIRECT,  // Call a known symbol (a function global)
    IR_CALLEE_VREG,    // Call through a function pointer held in a vreg
} IrCalleeKind;

typedef struct IrCall {
    IrCalleeKind callee_kind;
    int          callee;          // global index or vreg
    IrType       return_type;     // IR_VOID if nothing is returned
    int          arg_count;
    int         *args;            // vregs
    bool        *arg_is_vararg;   // Set for arguments passed through a C '...' parameter
} IrCall;

typedef struct IrInst {
    IrOp     op;
    IrType   type;     // Result type. For STORE the stored type, for ICMP/FCMP the operand type
    IrCmp    cmp;      // ICMP / FCMP predicate
    int      dst;      // Defined vreg or IR_NO_VREG
    int      a;        // First operand vreg or IR_NO_VREG
    int      b;        // Second operand vreg or IR_NO_VREG
    IrAddr   addr;     // LOAD / STORE address
    int64_t  imm;      // CONST value, SLOT_ADDR slot, GLOBAL_ADDR global, MEMCPY/MEMSET byte count
    int64_t  imm2;     // MEMSET byte value
    double   fimm;     // CONST_FLOAT value
    IrCall  *call;     // CALL
    int      line;     // Source line, 0 if unknown (only used for comments in dumps)
} IrInst;

typedef enum IrTermKind {
    IR_TERM_NONE,
    IR_TERM_JMP,       // jmp target
    IR_TERM_BR,        // br cond, then_block, else_block
    IR_TERM_RET,       // ret [value]
} IrTermKind;

typedef struct IrBlock IrBlock;

typedef struct IrTerm {
    IrTermKind kind;
    int        cond;       // BR condition vreg (i8)
    int        value;      // RET value vreg or IR_NO_VREG
    IrBlock   *target;     // JMP target, BR true target
    IrBlock   *else_target;// BR false target
} IrTerm;

typedef struct IrBlock {
    int     id;        // Unique id within the function (stable, used for labels)
    int     index;     // Position in the function's block layout. Set by ir_function_finalize
    char   *name;      // Debug name, e.g "if.then"
    bool    is_placed; // Whether the block has been added to the function layout
    IrArray(IrInst)    insts;
    IrTerm             term;
    IrArray(IrBlock *) preds;
} IrBlock;

typedef struct IrSlot {
    int   size;
    int   align;
    char *name;        // Debug name (variable name or temporary kind)
} IrSlot;

typedef struct IrFunction {
    char   *name;
    int     global;            // Index of the function's symbol in the module globals
    bool    is_entry_point;    // 'main'. Exported and returns 0 to the C runtime
    IrType  return_type;       // Scalar return value (IR_VOID if none)

    IrArray(IrType)    vreg_types;
    IrArray(int)       params;     // Vreg per incoming parameter, in ABI order
    IrArray(IrSlot)    slots;
    IrArray(IrBlock *) blocks;     // Layout order. blocks.items[0] is the entry block
    int                next_block_id;
} IrFunction;

typedef struct IrReloc {
    int offset;        // Byte offset within the global's data where a pointer goes
    int target;        // Global the pointer points to
} IrReloc;

typedef enum IrGlobalKind {
    IR_GLOBAL_DATA,        // Bytes + relocations defined by this module
    IR_GLOBAL_FUNCTION,    // A function defined by this module
    IR_GLOBAL_EXTERN,      // A symbol defined elsewhere (C functions, runtime data)
} IrGlobalKind;

typedef struct IrGlobal {
    int           index;
    IrGlobalKind  kind;
    char         *name;      // Symbol name used by the backends
    bool          readonly;
    int           align;
    int           size;
    uint8_t      *data;      // 'size' bytes (DATA only)
    IrArray(IrReloc) relocs; // DATA only
    IrFunction   *function;  // FUNCTION only
} IrGlobal;

typedef struct IrModule {
    Arena                arena;
    IrArray(IrGlobal *)  globals;
    IrArray(IrFunction *) functions;
} IrModule;

//
// Module / function construction
//
IrModule   *ir_module_new(void);
void       *ir_allocate(IrModule *module, size_t size);
IrGlobal   *ir_global_new(IrModule *module, IrGlobalKind kind, const char *name);
IrGlobal   *ir_global_data(IrModule *module, const char *debug_name, const void *bytes, int size, int align, bool readonly);
int         ir_find_or_add_extern(IrModule *module, const char *name);
IrFunction *ir_function_new(IrModule *module, const char *name);
int         ir_new_vreg(IrFunction *func, IrType type);
IrType      ir_vreg_type(IrFunction *func, int vreg);
int         ir_new_slot(IrFunction *func, int size, int align, const char *name);
IrBlock    *ir_block_new(IrFunction *func, const char *name);
void        ir_function_finalize(IrFunction *func); // Drop unreachable blocks, compute preds and block indices

//
// Instruction introspection shared by every pass
//
int  ir_inst_use_count(IrInst *inst);
int  ir_inst_use(IrInst *inst, int i);
bool ir_inst_is_call_like(IrInst *inst); // CALL, MEMCPY, MEMSET: clobber caller-saved registers
int  ir_term_use_count(IrTerm *term);
int  ir_term_use(IrTerm *term, int i);
int  ir_block_successor_count(IrBlock *block);
IrBlock *ir_block_successor(IrBlock *block, int i);

const char *ir_op_name(IrOp op);
const char *ir_cmp_name(IrCmp cmp);

//
// Liveness (per block live-in / live-out sets of vregs)
//
typedef struct IrBitset {
    uint64_t *words;
    int       word_count;
} IrBitset;

IrBitset ir_bitset_new(int bit_count);
bool     ir_bitset_get(IrBitset set, int bit);
void     ir_bitset_set(IrBitset set, int bit);
void     ir_bitset_clear(IrBitset set, int bit);
bool     ir_bitset_union_into(IrBitset dst, IrBitset src); // Returns true if dst changed
void     ir_bitset_copy(IrBitset dst, IrBitset src);

typedef struct IrLiveness {
    int       block_count;
    IrBitset *live_in;   // Indexed by block->index
    IrBitset *live_out;
} IrLiveness;

IrLiveness ir_compute_liveness(IrFunction *func);

//
// Debugging
//
void  ir_dump_function(IrModule *module, IrFunction *func, StringBuilder *sb);
void  ir_dump_module(IrModule *module, StringBuilder *sb);
bool  ir_verify_module(IrModule *module);  // Prints every problem it finds. Returns false on failure

#endif
