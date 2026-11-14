//////////////////////////////////////////////////////////////////////////////////////////
//
// AST -> IR lowering
//
// Memory model
// ------------
//   * Every local variable and parameter lives in a stack slot.
//   * Scalars (ints, floats, bools, enums, pointers, function pointers) are loaded into
//     vregs when used as values.
//   * Aggregates (structs, strings, arrays, slices, any, ...) are never loaded into
//     vregs. An aggregate expression always evaluates to a *place*: the address of memory
//     holding the value. Copying an aggregate is a memcpy.
//
// Calling convention (used for both Saad and C functions, it is the Win64 rule)
// -----------------------------------------------------------------------------
//   * Scalars are passed by value.
//   * Aggregates of size 1, 2, 4 or 8 are passed by value as an integer of that size.
//   * Other aggregates are passed as a pointer to a copy made by the caller.
//   * Saad functions return every value through hidden out-pointer parameters
//     (func_defn->lowered_return_params, set up by the typer).
//   * C functions return scalars (and aggregates of size 1, 2, 4, 8) by value. Bigger
//     aggregates are returned through a hidden pointer (set up by the typer).
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <setjmp.h>
#include "ir.h"

typedef enum ValueKind {
    VALUE_NONE,     // void
    VALUE_SCALAR,   // A scalar held in 'vreg'
    VALUE_PLACE,    // Memory at 'addr' holding a value of 'type'
} ValueKind;

typedef struct Value {
    ValueKind kind;
    Type     *type;
    int       vreg;
    IrAddr    addr;
    bool      is_temporary;  // The place is a fresh temporary that nobody else refers to (no need to copy it)
} Value;

typedef struct LoopTargets {
    void    *loop;           // AstFor* or AstWhile*
    IrBlock *break_block;
    IrBlock *continue_block;
    int      defer_count;    // The defers that are active outside of the loop. break and continue run the ones above
} LoopTargets;

// A try of the function that is being lowered: a thrown error goes to its catch, after the defers inside of the try
typedef struct TryTarget {
    IrBlock *catch_block;
    int      defer_count;
} TryTarget;

typedef struct PtrMapEntry {
    void *key;
    int   value;
} PtrMapEntry;

typedef struct PtrMap {
    PtrMapEntry *entries;
    int          capacity;
    int          count;
} PtrMap;

typedef struct TypeInfoEntry {
    char *key;
    int   global;
} TypeInfoEntry;

typedef struct Lowerer {
    Parser          *parser;
    IrModule        *module;

    // Per function state
    IrFunction      *func;
    IrBlock         *block;
    AstFunctionDefn *ast_func;
    int              scope;     // The lexical scope of the instructions, an index of func->scopes
    PtrMap           locals;          // AstIdentifier* -> slot index
    IrArray(LoopTargets) loops;
    IrArray(Ast *)   defers;          // The deferred statements of the blocks that are being lowered, innermost last
    IrArray(TryTarget) tries;         // The try blocks that are being lowered, innermost last
    int              line;

    // Module state
    PtrMap           const_globals;   // AstIdentifier* -> global index of an aggregate constant
    PtrMap           c_texts;         // const char* that lives as long as the AST, e.g a file path -> global index of it as C text
    int              assert_filename_global; // Cached per file
    AstFile         *assert_filename_file;

    int              thread_block_slot; // Per function: the slot with the block of the @threadLocal globals of the thread, or -1
    PtrMap           thread_offsets;    // AstIdentifier* -> offset of a @threadLocal global in the block
    int              thread_template;   // The global with the start values of the @threadLocal globals
    int              thread_block_size;

    AstBlock        *runtime_scope;   // The declarations of runtime.sd, for the layouts of the type descriptors
    IrArray(TypeInfoEntry) type_infos; // The descriptor of each type, by the name of the type
    IrArray(TypeInfoEntry) interface_tables; // The table of the methods of a type for an interface, by "type|interface"

    jmp_buf          on_error;
} Lowerer;

//////////////////////////////////////////////////////////////////////////////////////////
// Pointer map
//////////////////////////////////////////////////////////////////////////////////////////

static int ptr_map_get(PtrMap *map, void *key) {
    if (map->capacity == 0) return -1;
    size_t h = ((size_t) key >> 3) & (map->capacity - 1);
    while (map->entries[h].key) {
        if (map->entries[h].key == key) return map->entries[h].value;
        h = (h + 1) & (map->capacity - 1);
    }
    return -1;
}

static void ptr_map_put(PtrMap *map, void *key, int value) {
    if ((map->count + 1) * 2 > map->capacity) {
        PtrMap bigger = {0};
        bigger.capacity = map->capacity ? map->capacity * 2 : 64;
        bigger.entries  = calloc(bigger.capacity, sizeof(PtrMapEntry));
        for (int i = 0; i < map->capacity; i++) {
            if (map->entries[i].key) ptr_map_put(&bigger, map->entries[i].key, map->entries[i].value);
        }
        free(map->entries);
        *map = bigger;
    }
    size_t h = ((size_t) key >> 3) & (map->capacity - 1);
    while (map->entries[h].key && map->entries[h].key != key) {
        h = (h + 1) & (map->capacity - 1);
    }
    if (!map->entries[h].key) map->count++;
    map->entries[h].key   = key;
    map->entries[h].value = value;
}

static void ptr_map_clear(PtrMap *map) {
    free(map->entries);
    *map = (PtrMap){0};
}

//////////////////////////////////////////////////////////////////////////////////////////
// Errors
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(__GNUC__) || defined(__clang__)
    #define LOWER_NORETURN __attribute__((noreturn))
#else
    #define LOWER_NORETURN
#endif

// Reports an error at 'site' and aborts lowering of the whole module
LOWER_NORETURN static void lower_fail(Lowerer *L, Ast *site, const char *format, ...) {
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    report_error_ast(L->parser, LABEL_ERROR, site, "%s", message);
    longjmp(L->on_error, 1);
}

#define LOWER_NOT_IMPLEMENTED(L, site, what) lower_fail(L, (Ast *)(site), "Not implemented in the IR backend yet: %s", what)

//////////////////////////////////////////////////////////////////////////////////////////
// Types
//////////////////////////////////////////////////////////////////////////////////////////

static bool type_is_aggregate(Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT:
    case TYPE_STRING:
    case TYPE_ARRAY:
    case TYPE_ANY:
    case TYPE_VARIADIC:
    case TYPE_TUPLE:
        return true;
    default:
        return false;
    }
}

static Type *enum_backing_type(Type *type) {
    if (type->kind == TYPE_ENUM) return ((TypeEnum *) type)->backing_type;
    return type;
}

static bool type_is_untyped_int(Type *type) {
    return type->kind == TYPE_INTEGER && ((TypePrimitive *) type)->kind == PRIMITIVE_UNTYPED_INT;
}

static bool type_is_untyped_float(Type *type) {
    return type->kind == TYPE_FLOAT && ((TypePrimitive *) type)->kind == PRIMITIVE_UNTYPED_FLOAT;
}

static bool type_is_signed(Type *type) {
    type = enum_backing_type(type);
    if (type->kind != TYPE_INTEGER) return false;
    if (type_is_untyped_int(type)) return true;
    return is_signed_integer(type);
}

static IrType int_ir_type_of_size(int size) {
    switch (size) {
    case 1: return IR_I8;
    case 2: return IR_I16;
    case 4: return IR_I32;
    case 8: return IR_I64;
    default: return IR_VOID;
    }
}

// Maps a scalar front-end type to its IR type. Aggregates map to IR_VOID
static IrType ir_type_of(Type *type) {
    type = enum_backing_type(type);
    switch (type->kind) {
    case TYPE_BOOL:     return IR_I8;
    case TYPE_INTEGER:
        if (type_is_untyped_int(type)) return IR_I64;
        return int_ir_type_of_size(type->size);
    case TYPE_FLOAT:
        if (type_is_untyped_float(type)) return type->size == 8 ? IR_F64 : IR_F32;
        return type->size == 8 ? IR_F64 : IR_F32;
    case TYPE_POINTER:
    case TYPE_FUNCTION: return IR_PTR;
    default:            return IR_VOID;
    }
}

static int type_alignment(Type *type) {
    switch (type->kind) {
    case TYPE_STRUCT: {
        int align = ((TypeStruct *) type)->alignment;
        return align > 0 ? align : 8;
    }
    case TYPE_STRING:
    case TYPE_ANY:
    case TYPE_VARIADIC:
    case TYPE_TUPLE:
        return 8;
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *) type;
        if (array->array_kind == ARRAY_FIXED) return type_alignment(array->elem_type);
        return 8;
    }
    default: {
        int size = type->size;
        if (size <= 0) return 1;
        if (size >= 8) return 8;
        return size;
    }
    }
}

// How a value of 'type' is passed to (and received by) a function. See the top of the file
typedef enum ArgClass {
    ARG_SCALAR,
    ARG_AGGREGATE_BY_VALUE,
    ARG_AGGREGATE_BY_REFERENCE,
} ArgClass;

static ArgClass classify_argument(Type *type) {
    if (!type_is_aggregate(type)) return ARG_SCALAR;
    int size = type->size;
    if (size == 1 || size == 2 || size == 4 || size == 8) return ARG_AGGREGATE_BY_VALUE;
    return ARG_AGGREGATE_BY_REFERENCE;
}

// The IR type a function parameter of 'type' is received as
static IrType ir_param_type(Type *type) {
    switch (classify_argument(type)) {
    case ARG_SCALAR:                 return ir_type_of(type);
    case ARG_AGGREGATE_BY_VALUE:     return int_ir_type_of_size(type->size);
    case ARG_AGGREGATE_BY_REFERENCE: return IR_PTR;
    }
    return IR_VOID;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Emission helpers
//////////////////////////////////////////////////////////////////////////////////////////

static IrInst make_inst(Lowerer *L, IrOp op, IrType type) {
    IrInst inst = {0};
    inst.op   = op;
    inst.type = type;
    inst.dst  = IR_NO_VREG;
    inst.a    = IR_NO_VREG;
    inst.b    = IR_NO_VREG;
    inst.line = L->line;
    inst.scope = L->scope;
    return inst;
}

static void push_inst(Lowerer *L, IrInst inst) {
    ir_array_push(L->block->insts, inst);
}

static int emit_with_dst(Lowerer *L, IrInst inst) {
    inst.dst = ir_new_vreg(L->func, (inst.op == IR_ICMP || inst.op == IR_FCMP) ? IR_I8 : inst.type);
    push_inst(L, inst);
    return inst.dst;
}

static int emit_const(Lowerer *L, IrType type, int64_t value) {
    IrInst inst = make_inst(L, IR_CONST, type);
    inst.imm = value;
    return emit_with_dst(L, inst);
}

static int emit_fconst(Lowerer *L, IrType type, double value) {
    IrInst inst = make_inst(L, IR_CONST_FLOAT, type);
    inst.fimm = value;
    return emit_with_dst(L, inst);
}

static int emit_binop(Lowerer *L, IrOp op, IrType type, int a, int b) {
    IrInst inst = make_inst(L, op, type);
    inst.a = a;
    inst.b = b;
    return emit_with_dst(L, inst);
}

static int emit_unop(Lowerer *L, IrOp op, IrType type, int a) {
    IrInst inst = make_inst(L, op, type);
    inst.a = a;
    return emit_with_dst(L, inst);
}

static int emit_cmp(Lowerer *L, IrOp op, IrCmp cmp, IrType operand_type, int a, int b) {
    IrInst inst = make_inst(L, op, operand_type);
    inst.cmp = cmp;
    inst.a   = a;
    inst.b   = b;
    return emit_with_dst(L, inst);
}

static int emit_load(Lowerer *L, IrType type, IrAddr addr) {
    IrInst inst = make_inst(L, IR_LOAD, type);
    inst.addr = addr;
    return emit_with_dst(L, inst);
}

static void emit_store(Lowerer *L, IrType type, IrAddr addr, int value) {
    IrInst inst = make_inst(L, IR_STORE, type);
    inst.addr = addr;
    inst.a    = value;
    push_inst(L, inst);
}

static int emit_slot_addr(Lowerer *L, int slot) {
    IrInst inst = make_inst(L, IR_SLOT_ADDR, IR_PTR);
    inst.imm = slot;
    return emit_with_dst(L, inst);
}

static int emit_global_addr(Lowerer *L, int global) {
    IrInst inst = make_inst(L, IR_GLOBAL_ADDR, IR_PTR);
    inst.imm = global;
    return emit_with_dst(L, inst);
}

static int emit_ptr_add(Lowerer *L, int base, int offset_i64) {
    return emit_binop(L, IR_PTR_ADD, IR_PTR, base, offset_i64);
}

static void emit_memcpy(Lowerer *L, int dst, int src, int size) {
    if (size <= 0) return;
    IrInst inst = make_inst(L, IR_MEMCPY, IR_VOID);
    inst.a   = dst;
    inst.b   = src;
    inst.imm = size;
    push_inst(L, inst);
}

static void emit_memset(Lowerer *L, int dst, int byte, int size) {
    if (size <= 0) return;
    IrInst inst = make_inst(L, IR_MEMSET, IR_VOID);
    inst.a    = dst;
    inst.imm  = size;
    inst.imm2 = byte;
    push_inst(L, inst);
}

static int emit_call(Lowerer *L, IrCall *call) {
    IrInst inst = make_inst(L, IR_CALL, call->return_type);
    inst.call = call;
    if (call->return_type != IR_VOID) {
        inst.dst = ir_new_vreg(L->func, call->return_type);
    }
    push_inst(L, inst);
    return inst.dst;
}

static IrCall *new_call(Lowerer *L, IrCalleeKind kind, int callee, IrType return_type, int arg_capacity) {
    IrCall *call = ir_allocate(L->module, sizeof(IrCall));
    call->callee_kind   = kind;
    call->callee        = callee;
    call->return_type   = return_type;
    call->args          = ir_allocate(L->module, sizeof(int)  * (arg_capacity > 0 ? arg_capacity : 1));
    call->arg_is_vararg = ir_allocate(L->module, sizeof(bool) * (arg_capacity > 0 ? arg_capacity : 1));
    return call;
}

static void call_add_arg(IrCall *call, int vreg, bool is_vararg) {
    call->args[call->arg_count] = vreg;
    call->arg_is_vararg[call->arg_count] = is_vararg;
    call->arg_count++;
}

// Calls a C function by name with scalar arguments
static int emit_c_call(Lowerer *L, const char *name, IrType return_type, int arg_count, int *args) {
    int callee = ir_find_or_add_extern(L->module, name);
    IrCall *call = new_call(L, IR_CALLEE_DIRECT, callee, return_type, arg_count);
    for (int i = 0; i < arg_count; i++) call_add_arg(call, args[i], false);
    return emit_call(L, call);
}

static bool block_is_terminated(Lowerer *L) {
    return L->block->term.kind != IR_TERM_NONE;
}

static void set_block(Lowerer *L, IrBlock *block) {
    if (!block->is_placed) {
        block->is_placed = true;
        ir_array_push(L->func->blocks, block);
    }
    L->block = block;
}

static IrBlock *create_block(Lowerer *L, const char *name) {
    return ir_block_new(L->func, name);
}

static void terminate_jmp(Lowerer *L, IrBlock *target) {
    if (block_is_terminated(L)) return;
    L->block->term.kind   = IR_TERM_JMP;
    L->block->term.target = target;
}

static void terminate_br(Lowerer *L, int cond, IrBlock *then_block, IrBlock *else_block) {
    if (block_is_terminated(L)) return;
    L->block->term.kind        = IR_TERM_BR;
    L->block->term.cond        = cond;
    L->block->term.target      = then_block;
    L->block->term.else_target = else_block;
}

static void terminate_ret(Lowerer *L, int value) {
    if (block_is_terminated(L)) return;
    L->block->term.kind  = IR_TERM_RET;
    L->block->term.value = value;
}

static int emit_const(Lowerer *L, IrType type, int64_t value);

// A Saad function returns its values through out-pointers, so eax says if an error left it: 0 when it returns, 1 when
// it was thrown out of it. The check after a call looks at eax, and not at thrownError, which is in the block of the
// thread (see emit_throw_check)
static void terminate_return(Lowerer *L, bool threw) {
    if (block_is_terminated(L)) return;
    terminate_ret(L, emit_const(L, IR_I32, threw ? 1 : 0));
}

// After a jump that ends control flow (return, break, continue) any following statements are
// unreachable. They still get lowered into a fresh block that ir_function_finalize removes.
static void start_unreachable_block(Lowerer *L) {
    set_block(L, create_block(L, "unreachable"));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Values and places
//////////////////////////////////////////////////////////////////////////////////////////

static Value value_none(void) {
    Value v = {0};
    v.kind = VALUE_NONE;
    v.vreg = IR_NO_VREG;
    return v;
}

static Value value_scalar(int vreg, Type *type) {
    Value v = {0};
    v.kind = VALUE_SCALAR;
    v.vreg = vreg;
    v.type = type;
    return v;
}

static Value value_place(IrAddr addr, Type *type) {
    Value v = {0};
    v.kind = VALUE_PLACE;
    v.vreg = IR_NO_VREG;
    v.addr = addr;
    v.type = type;
    return v;
}

static IrAddr addr_slot(int slot, int offset)     { return (IrAddr){ IR_ADDR_SLOT,   slot,   offset }; }
static IrAddr addr_vreg(int vreg, int offset)     { return (IrAddr){ IR_ADDR_VREG,   vreg,   offset }; }
static IrAddr addr_global(int global, int offset) { return (IrAddr){ IR_ADDR_GLOBAL, global, offset }; }

static IrAddr addr_add(IrAddr addr, int offset) {
    addr.offset += offset;
    return addr;
}

// Bytes of memory a value of 'type' needs. Untyped integers have size 4 in the AST but are
// carried as i64 in the IR, so their storage covers the IR type as well
static int type_storage_size(Type *type) {
    int size = type->size;
    if (!type_is_aggregate(type)) {
        int ir_size = ir_type_size(ir_type_of(type));
        if (ir_size > size) size = ir_size;
    }
    return size;
}

static int new_temp_slot(Lowerer *L, Type *type, const char *name) {
    return ir_new_slot(L->func, type_storage_size(type), type_alignment(type), name);
}

static Value temp_place(Lowerer *L, Type *type, const char *name) {
    Value v = value_place(addr_slot(new_temp_slot(L, type, name), 0), type);
    v.is_temporary = true;
    return v;
}

// Materializes the address of a place as a ptr vreg
static int addr_to_ptr(Lowerer *L, IrAddr addr) {
    int base;
    switch (addr.kind) {
    case IR_ADDR_SLOT:   base = emit_slot_addr(L, addr.base); break;
    case IR_ADDR_GLOBAL: base = emit_global_addr(L, addr.base); break;
    case IR_ADDR_VREG:   base = addr.base; break;
    default:             assert(false); return IR_NO_VREG;
    }
    if (addr.offset == 0) return base;
    return emit_ptr_add(L, base, emit_const(L, IR_I64, addr.offset));
}

// Loads a scalar value out of a place. Scalars are returned as is
static int value_to_scalar(Lowerer *L, Value value) {
    if (value.kind == VALUE_SCALAR) return value.vreg;
    assert(value.kind == VALUE_PLACE);
    IrType type = ir_type_of(value.type);
    assert(type != IR_VOID && "value_to_scalar on an aggregate");
    return emit_load(L, type, value.addr);
}

// Makes sure an aggregate value is in memory (e.g a small struct returned in a register by a C function)
static Value value_to_place(Lowerer *L, Value value) {
    if (value.kind == VALUE_PLACE) return value;
    assert(value.kind == VALUE_SCALAR);
    Value place = temp_place(L, value.type, "spilled value");
    emit_store(L, ir_vreg_type(L->func, value.vreg), place.addr, value.vreg);
    return place;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Conversions between scalar types
//////////////////////////////////////////////////////////////////////////////////////////

static int emit_to_bool(Lowerer *L, int vreg, Type *from) {
    IrType type = ir_vreg_type(L->func, vreg);
    if (from->kind == TYPE_BOOL) return vreg;
    if (ir_type_is_float(type)) {
        return emit_cmp(L, IR_FCMP, IR_CMP_UNE, type, vreg, emit_fconst(L, type, 0.0));
    }
    return emit_cmp(L, IR_ICMP, IR_CMP_NE, type, vreg, emit_const(L, type, 0));
}

// Converts a scalar vreg of front-end type 'from' to front-end type 'to'
int embedded_offset(TypeStruct *outer, TypeStruct *inner, int depth);

static int convert_scalar(Lowerer *L, int vreg, Type *from, Type *to) {
    IrType src = ir_vreg_type(L->func, vreg);
    IrType dst = ir_type_of(to);
    assert(dst != IR_VOID);

    // A *Outer as a *Inner that it embeds is the address of the embedded member
    if (from->kind == TYPE_POINTER && to->kind == TYPE_POINTER) {
        Type *outer = ((TypePointer *) from)->pointer_to, *inner = ((TypePointer *) to)->pointer_to;
        if (outer->kind == TYPE_STRUCT && inner->kind == TYPE_STRUCT && outer != inner) {
            int offset = embedded_offset((TypeStruct *) outer, (TypeStruct *) inner, 0);
            if (offset > 0) return emit_ptr_add(L, vreg, emit_const(L, IR_I64, offset));
        }
    }

    if (to->kind == TYPE_BOOL && from->kind != TYPE_BOOL) {
        return emit_to_bool(L, vreg, from);
    }

    if (src == dst) return vreg;

    bool from_signed = type_is_signed(from);
    bool to_signed   = type_is_signed(to);

    if (ir_type_is_int(src) && ir_type_is_int(dst)) {
        if (ir_type_size(src) < ir_type_size(dst)) return emit_unop(L, from_signed ? IR_SEXT : IR_ZEXT, dst, vreg);
        return emit_unop(L, IR_TRUNC, dst, vreg);
    }
    if (ir_type_is_int(src) && ir_type_is_float(dst)) return emit_unop(L, from_signed ? IR_SITOFP : IR_UITOFP, dst, vreg);
    if (ir_type_is_float(src) && ir_type_is_int(dst)) return emit_unop(L, to_signed ? IR_FPTOSI : IR_FPTOUI, dst, vreg);
    if (src == IR_F32 && dst == IR_F64) return emit_unop(L, IR_FPEXT, dst, vreg);
    if (src == IR_F64 && dst == IR_F32) return emit_unop(L, IR_FPTRUNC, dst, vreg);

    // Pointers <-> integers
    if (src == IR_PTR && ir_type_is_int(dst)) {
        int as_int = emit_unop(L, IR_BITCAST, IR_I64, vreg);
        if (dst == IR_I64) return as_int;
        return emit_unop(L, IR_TRUNC, dst, as_int);
    }
    if (ir_type_is_int(src) && dst == IR_PTR) {
        if (src != IR_I64) vreg = emit_unop(L, from_signed ? IR_SEXT : IR_ZEXT, IR_I64, vreg);
        return emit_unop(L, IR_BITCAST, IR_PTR, vreg);
    }

    printf("Compiler Error: Unhandled scalar conversion %s -> %s\n", ir_type_name(src), ir_type_name(dst));
    assert(false);
    return vreg;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Strings and constant data
//////////////////////////////////////////////////////////////////////////////////////////

// Decodes the escape sequences of a string literal. Returns the decoded length
static int decode_string_literal(const char *src, int src_len, char *out) {
    int n = 0;
    for (int i = 0; i < src_len; i++) {
        char c = src[i];
        if (c == '\\' && i + 1 < src_len) {
            char e = src[++i];
            switch (e) {
            case 'n':  out[n++] = '\n'; break;
            case 't':  out[n++] = '\t'; break;
            case 'r':  out[n++] = '\r'; break;
            case '0':  out[n++] = '\0'; break;
            case '\\': out[n++] = '\\'; break;
            case '"':  out[n++] = '"';  break;
            case '\'': out[n++] = '\''; break;
            default:   out[n++] = '\\'; out[n++] = e; break; // Unknown escape: keep it as written
            }
        } else {
            out[n++] = c;
        }
    }
    return n;
}

// Creates the character data global for a string literal (NUL terminated so it can be passed to C).
// The characters are writable: Saad programs may modify the contents of a string literal
static int string_data_global(Lowerer *L, const char *raw, int raw_len, int *out_len) {
    char *decoded = malloc(raw_len + 1);
    int len = decode_string_literal(raw, raw_len, decoded);
    decoded[len] = '\0';
    IrGlobal *data = ir_global_data(L->module, "str", decoded, len + 1, 1, false);
    free(decoded);
    *out_len = len;
    return data->index;
}

// Creates a global holding a Saad 'string' value ({ data: *u8, len: i64 }) for a literal
static int string_global(Lowerer *L, const char *raw, int raw_len) {
    int len = 0;
    int data = string_data_global(L, raw, raw_len, &len);

    int64_t header[2] = { 0, len };
    IrGlobal *str = ir_global_data(L->module, "string", header, 16, 8, true);
    IrReloc reloc = { 0, data };
    ir_array_push(str->relocs, reloc);
    return str->index;
}

static int function_global(Lowerer *L, AstFunctionDefn *func_defn);

static void write_int_bytes(uint8_t *dst, int64_t value, int size) {
    memcpy(dst, &value, size); // Little endian
}

// Writes the constant expression 'expr', interpreted as type 'type', into a global's data
static void write_default_constants(Lowerer *L, IrGlobal *global, int offset, Type *type);
bool is_optional_type(Type *type);
Type *optional_inner_type(Type *type);
int   optional_present_offset(Type *type);

static void write_constant(Lowerer *L, IrGlobal *global, int offset, AstExpr *expr, Type *type) {
    // A T for a ?T, e.g 'count: ?int = 5;': the value and present = true. null leaves it empty, all zeros
    if (is_optional_type(type) && expr->type && !is_optional_type(expr->type)) {
        if (is_null_type(expr->type)) return;
        write_constant(L, global, offset, expr, optional_inner_type(type));
        global->data[offset + optional_present_offset(type)] = 1;
        return;
    }

    Type *storage = enum_backing_type(type);

    switch (expr->head.kind) {
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *) expr;
        switch (lit->kind) {
        case LITERAL_BOOLEAN:
            global->data[offset] = lit->as.value.boolean ? 1 : 0;
            return;
        case LITERAL_INTEGER:
            if (storage->kind == TYPE_FLOAT) {
                double d = (double) lit->as.value.integer;
                if (storage->size == 4) { float f = (float) d; memcpy(&global->data[offset], &f, 4); }
                else                    { memcpy(&global->data[offset], &d, 8); }
                return;
            }
            write_int_bytes(&global->data[offset], lit->as.value.integer, storage->size);
            return;
        case LITERAL_FLOAT: {
            double d = lit->as.value.floating;
            if (storage->size == 4) { float f = (float) d; memcpy(&global->data[offset], &f, 4); }
            else                    { memcpy(&global->data[offset], &d, 8); }
            return;
        }
        case LITERAL_NULL:
            return; // Already zero
        case LITERAL_STRING: {
            if (storage->kind == TYPE_STRUCT) {
                // A c.string: the pointer to the bytes
                int len = 0;
                int data = string_data_global(L, lit->as.value.string.data, lit->as.value.string.length, &len);
                IrReloc reloc = { offset, data };
                ir_array_push(global->relocs, reloc);
                return;
            }
            if (storage->kind != TYPE_STRING) break;
            int len = 0;
            int data = string_data_global(L, lit->as.value.string.data, lit->as.value.string.length, &len);
            IrReloc reloc = { offset, data };
            ir_array_push(global->relocs, reloc);
            write_int_bytes(&global->data[offset + 8], len, 8);
            return;
        }
        case LITERAL_IDENTIFIER: {
            AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
            if (ident && (ident->flags & IDENTIFIER_IS_CONSTANT) && ident->value) {
                write_constant(L, global, offset, ident->value, type);
                return;
            }
            break;
        }
        }
        break;
    }
    case AST_ENUM_LITERAL: {
        AstEnumLiteral *lit = (AstEnumLiteral *) expr;
        write_int_bytes(&global->data[offset], lit->enum_member->value, storage->size);
        return;
    }
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma = (AstMemberAccess *) expr;
        if (ma->access_kind == MEMBER_ACCESS_ENUM_MEMBER && ma->enum_member->head.kind != AST_IDENTIFIER) {
            write_int_bytes(&global->data[offset], ma->enum_member->value, storage->size);
            return;
        }
        if (ma->access_kind == MEMBER_ACCESS_STATIC_STRUCT_MEMBER && ma->struct_member->value) {
            write_constant(L, global, offset, ma->struct_member->value, type);
            return;
        }
        break;
    }
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *) expr;
        if (unary->operator == OP_UNARY_MINUS && unary->expr->head.kind == AST_LITERAL) {
            AstLiteral *lit = (AstLiteral *) unary->expr;
            if (lit->kind == LITERAL_INTEGER && storage->kind != TYPE_FLOAT) {
                write_int_bytes(&global->data[offset], -lit->as.value.integer, storage->size);
                return;
            }
            double d = -(lit->kind == LITERAL_INTEGER ? (double) lit->as.value.integer : lit->as.value.floating);
            if (storage->size == 4) { float f = (float) d; memcpy(&global->data[offset], &f, 4); }
            else                    { memcpy(&global->data[offset], &d, 8); }
            return;
        }
        break;
    }
    case AST_CAST: {
        AstCast *cast = (AstCast *) expr;
        write_constant(L, global, offset, cast->expr, type);
        return;
    }
    case AST_STRUCT_LITERAL: {
        AstStructLiteral *lit = (AstStructLiteral *) expr;
        for (int i = 0; i < lit->initializers.count; i++) {
            AstStructInitializer *init = da_get_deref(lit->initializers, i);
            write_constant(L, global, offset + init->member->member_offset, init->value, init->member->type);
        }
        // The members that aren't given get their default value, e.g 'code: int = 7;', or of their type, e.g the first value
        // of an enum without 0
        if (storage->kind == TYPE_STRUCT && !((TypeStruct *) storage)->node->is_union) {
            DynamicArray members = ((TypeStruct *) storage)->node->members->identifiers;
            for (int m = 0; m < members.count; m++) {
                AstIdentifier *member = da_get_deref(members, m);
                if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
                bool given = false;
                for (int i = 0; i < lit->initializers.count; i++) {
                    if (((AstStructInitializer *) da_get_deref(lit->initializers, i))->member == member) given = true;
                }
                if (given) continue;
                AstExpr *fallback = member_default_value(member);
                if (fallback) write_constant(L, global, offset + member->member_offset, fallback, member->type);
                else          write_default_constants(L, global, offset + member->member_offset, member->type);
            }
        }
        return;
    }
    case AST_ARRAY_LITERAL: {
        AstArrayLiteral *lit = (AstArrayLiteral *) expr;
        if (storage->kind != TYPE_ARRAY || ((TypeArray *) storage)->array_kind != ARRAY_FIXED) break;
        Type *elem = ((TypeArray *) storage)->elem_type;
        for (int i = 0; i < lit->expressions.count; i++) {
            write_constant(L, global, offset + i * elem->size, da_get_deref(lit->expressions, i), elem);
        }
        return;
    }
    case AST_DEFAULT_VALUE:
        write_default_constants(L, global, offset, type);
        return;
    case AST_FUNCTION_DEFN: {
        AstFunctionDefn *func_defn = (AstFunctionDefn *) expr;
        IrReloc reloc = { offset, function_global(L, func_defn) }; // Also queues the function for lowering
        ir_array_push(global->relocs, reloc);
        return;
    }
    default:
        break;
    }

    lower_fail(L, (Ast *) expr, "Not implemented in the IR backend yet: constant expression of this kind (%s) as type %s", ast_to_str((Ast *) expr), type_to_str(type));
}

// Returns (and caches) the global holding an aggregate constant
static int aggregate_constant_global(Lowerer *L, AstIdentifier *ident) {
    int cached = ptr_map_get(&L->const_globals, ident);
    if (cached >= 0) return cached;

    Type *type = ident->type;
    AstExpr *value = ident->value;

    // A constant array literal of unknown size is laid out as a fixed array
    IrGlobal *global = ir_global_data(L->module, ident->name, NULL, type->size, type_alignment(type), true);
    ptr_map_put(&L->const_globals, ident, global->index);
    write_constant(L, global, 0, value, type);
    return global->index;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Storage of identifiers
//////////////////////////////////////////////////////////////////////////////////////////

static AstIdentifier *resolve_override(AstIdentifier *ident) {
    while (ident->ident_override) ident = ident->ident_override;
    return ident;
}

static int local_slot(Lowerer *L, AstIdentifier *ident) {
    return ptr_map_get(&L->locals, resolve_override(ident));
}

static int declare_local(Lowerer *L, AstIdentifier *ident) {
    ident = resolve_override(ident);
    int slot = ir_new_slot(L->func, type_storage_size(ident->type), type_alignment(ident->type), ident->name);
    ptr_map_put(&L->locals, ident, slot);

    // For the debug information. The names of the compiler, e.g $ret, are left out
    if (ident->name && ident->name[0] && ident->name[0] != '$' && strcmp(ident->name, "_") != 0) {
        bool same_file = !L->ast_func || ident->head.file == L->ast_func->head.head.file;
        IrLocal local = { ir_strdup(L->module, ident->name), ident->type, slot, L->scope, same_file ? ident->head.start.line : L->line, 0 };
        ir_array_push(L->func->locals, local);
    }
    return slot;
}

// A lexical scope inside the current one, for the variables declared in it. Returns the one to go back to
static int enter_scope(Lowerer *L, Ast *node) {
    int outer = L->scope;
    bool same_file = !L->ast_func || node->file == L->ast_func->head.head.file;
    IrScope scope = { outer, same_file && node->start.line > 0 ? node->start.line : L->line };
    ir_array_push(L->func->scopes, scope);
    L->scope = L->func->scopes.count - 1;
    return outer;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

static Value lower_expr(Lowerer *L, AstExpr *expr);
static Value lower_place(Lowerer *L, AstExpr *expr);
static void emit_errors_call(Lowerer *L, const char *name, int arg_count, int *args);
bool bounds_checks = true; // Indexes of arrays, slices, lists and strings, and pointers that are dereferenced, are checked, unless built with --unchecked
static void emit_null_check(Lowerer *L, int ptr, AstExpr *expr);
static void emit_memory_check(Lowerer *L, int ptr, AstExpr *expr);
static void emit_string_memory_check(Lowerer *L, int string_ptr, AstExpr *expr);
bool memory_checks = false; // saad build --check-memory: pointers into the memory of arenas that were cleared or freed are found
static bool is_receiver_of_method(Lowerer *L, AstExpr *expr);
// The place whose address & takes, e.g p[i] of &p[i]. Its last step only computes an address, so it isn't checked for null
static AstExpr *address_taken_of = NULL;
static void emit_trace_frame(Lowerer *L, int line);
static void emit_unwind(Lowerer *L);
static void emit_throw_check(Lowerer *L, int threw, bool confirm);
static void emit_throw(Lowerer *L, int err);
static void  lower_declaration(Lowerer *L, AstDeclaration *decl);
static int   lower_scalar_as(Lowerer *L, AstExpr *expr, Type *target);
static void  lower_condition(Lowerer *L, AstExpr *expr, IrBlock *on_true, IrBlock *on_false);
static void  lower_block(Lowerer *L, AstBlock *block);
static void  lower_statement(Lowerer *L, Ast *stmt);
static int   string_global_raw(Lowerer *L, const char *text);
static void  store_expr(Lowerer *L, IrAddr dst, Type *dst_type, AstExpr *expr, bool dst_is_fresh);
static Value lower_call(Lowerer *L, AstFunctionCall *call, int *ret_slots, int max_ret_slots);
static int   function_global(Lowerer *L, AstFunctionDefn *func_defn);
static Value box_to_any(Lowerer *L, Value value, Type *any_type, Ast *site);
static Value unbox_any(Lowerer *L, Value any, Type *to);

//////////////////////////////////////////////////////////////////////////////////////////
// Copies and zeroing
//////////////////////////////////////////////////////////////////////////////////////////

static void emit_zero(Lowerer *L, IrAddr addr, int size) {
    if (size > 64) {
        emit_memset(L, addr_to_ptr(L, addr), 0, size);
        return;
    }

    int zero[IR_TYPE_COUNT];
    for (int i = 0; i < IR_TYPE_COUNT; i++) zero[i] = IR_NO_VREG;

    int offset = 0;
    while (offset < size) {
        int remaining = size - offset;
        IrType type = remaining >= 8 ? IR_I64 : remaining >= 4 ? IR_I32 : remaining >= 2 ? IR_I16 : IR_I8;
        if (zero[type] == IR_NO_VREG) zero[type] = emit_const(L, type, 0);
        emit_store(L, type, addr_add(addr, offset), zero[type]);
        offset += ir_type_size(type);
    }
}

static int64_t fixed_array_count(TypeArray *array) {
    if (array->count > 0) return array->count;
    return array->capacity;
}

static void store_expr(Lowerer *L, IrAddr dst, Type *dst_type, AstExpr *expr, bool dst_is_fresh);

// The value an enum starts with when 0 isn't one of its values, e.g .ok of 'Status :: enum { ok = 200, notFound = 404 }':
// its first value, so an enum always holds one of its values. False for an enum that has 0, and an enum flags, whose 0 is
// no flags
static bool enum_default_value(Type *type, int64_t *value) {
    if (type->kind != TYPE_ENUM) return false;
    AstEnum *node = ((TypeEnum *) type)->node;
    if (node == NULL || node->is_flags || node->enumerators.count == 0) return false;
    for (int i = 0; i < node->enumerators.count; i++) {
        if (((AstEnumerator **) node->enumerators.items)[i]->value == 0) return false;
    }
    *value = ((AstEnumerator **) node->enumerators.items)[0]->value;
    return true;
}

// Returns true if zeroed memory is not the default value of the type, because it has struct members with default values
// e.g 'age: int = 30;', or is an enum without the value 0 (enum_default_value)
static bool has_default_values(Type *type) {
    int64_t unused;
    if (enum_default_value(type, &unused)) return true;
    if (type->kind == TYPE_STRUCT) {
        DynamicArray members = ((TypeStruct *) type)->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
            if (member_default_value(member) || has_default_values(member->type)) return true;
        }
        return false;
    }
    if (type->kind == TYPE_ARRAY && ((TypeArray *) type)->array_kind == ARRAY_FIXED) {
        return has_default_values(((TypeArray *) type)->elem_type);
    }
    return false;
}

// Writes the default values of the struct members in 'type' into zeroed memory
static void emit_default_values(Lowerer *L, IrAddr addr, Type *type) {
    int64_t first;
    if (enum_default_value(type, &first)) {
        IrType ir_type = ir_type_of(type);
        emit_store(L, ir_type, addr, emit_const(L, ir_type, first));
        return;
    }
    if (type->kind == TYPE_STRUCT) {
        DynamicArray members = ((TypeStruct *) type)->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;

            AstExpr *value = member_default_value(member);
            IrAddr member_addr = addr_add(addr, member->member_offset);
            if (value) {
                store_expr(L, member_addr, member->type, value, true);
            } else if (has_default_values(member->type)) {
                emit_default_values(L, member_addr, member->type);
            }
        }
        return;
    }

    if (type->kind == TYPE_ARRAY && ((TypeArray *) type)->array_kind == ARRAY_FIXED) {
        Type   *elem  = ((TypeArray *) type)->elem_type;
        int64_t count = fixed_array_count((TypeArray *) type);
        if (!has_default_values(elem) || count <= 0) return;

        if (count <= 4) {
            for (int64_t i = 0; i < count; i++) emit_default_values(L, addr_add(addr, (int)(i * elem->size)), elem);
            return;
        }

        // for i := 0; i < count; i += 1 { defaults of element i }
        int base    = addr_to_ptr(L, addr);
        int counter = ir_new_slot(L->func, 8, 8, "default init index");
        emit_store(L, IR_I64, addr_slot(counter, 0), emit_const(L, IR_I64, 0));

        IrBlock *cond = create_block(L, "default.cond");
        IrBlock *body = create_block(L, "default.body");
        IrBlock *done = create_block(L, "default.done");
        terminate_jmp(L, cond);

        set_block(L, cond);
        int index = emit_load(L, IR_I64, addr_slot(counter, 0));
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_SLT, IR_I64, index, emit_const(L, IR_I64, count)), body, done);

        set_block(L, body);
        int offset  = emit_binop(L, IR_MUL, IR_I64, index, emit_const(L, IR_I64, elem->size));
        int element = emit_ptr_add(L, base, offset);
        emit_default_values(L, addr_vreg(element, 0), elem);
        emit_store(L, IR_I64, addr_slot(counter, 0), emit_binop(L, IR_ADD, IR_I64, index, emit_const(L, IR_I64, 1)));
        terminate_jmp(L, cond);

        set_block(L, done);
    }
}

// The value of a variable declared without an initializer
static void emit_default_init(Lowerer *L, IrAddr addr, Type *type) {
    emit_zero(L, addr, type_storage_size(type));
    if (has_default_values(type)) emit_default_values(L, addr, type);
}

static void copy_aggregate(Lowerer *L, IrAddr dst, Value src, int size) {
    src = value_to_place(L, src);
    if (size <= 0) return;

    IrType small = int_ir_type_of_size(size);
    if (small != IR_VOID) {
        emit_store(L, small, dst, emit_load(L, small, src.addr));
        return;
    }
    emit_memcpy(L, addr_to_ptr(L, dst), addr_to_ptr(L, src.addr), size);
}

bool  is_optional_type(Type *type);
Type *optional_inner_type(Type *type);
int   optional_present_offset(Type *type);
static void store_value(Lowerer *L, IrAddr dst, Type *dst_type, Value src, Ast *site);

// A T (or null) as a ?T: the value and present = true, or all zeros for null
static Value wrap_in_optional(Lowerer *L, Value value, Type *optional_type, Ast *site) {
    Value optional = temp_place(L, optional_type, "optional");
    emit_zero(L, optional.addr, type_storage_size(optional_type));
    if (!is_null_type(value.type)) {
        store_value(L, optional.addr, optional_inner_type(optional_type), value, site);
        emit_store(L, IR_I8, addr_add(optional.addr, optional_present_offset(optional_type)), emit_const(L, IR_I8, 1));
    }
    return optional;
}

// Converts an aggregate value to another aggregate type with a compatible layout, e.g a fixed array to a slice
static Value make_interface_value(Lowerer *L, Value value, TypeStruct *interface, Ast *site);
static void write_pointer_field(IrGlobal *global, int offset, int target);
static int type_info_global(Lowerer *L, Type *type);
static int emit_c_text(Lowerer *L, const char *text);

static Value convert_aggregate(Lowerer *L, Value value, Type *to, Ast *site) {
    Type *from = value.type;
    if (from == to) return value;

    if (is_interface_type(to) && !is_interface_type(from)) {
        return make_interface_value(L, value, (TypeStruct *) to, site);
    }

    if (is_optional_type(to) && !is_optional_type(from)) {
        return wrap_in_optional(L, value, to, site);
    }

    value = value_to_place(L, value);

    bool to_slice   = (to->kind == TYPE_ARRAY && ((TypeArray *) to)->array_kind == ARRAY_SLICE) || to->kind == TYPE_VARIADIC;
    bool from_fixed = from->kind == TYPE_ARRAY && ((TypeArray *) from)->array_kind == ARRAY_FIXED;

    if (to_slice && from_fixed) {
        Value slice = temp_place(L, to, "slice");
        emit_store(L, IR_PTR, slice.addr, addr_to_ptr(L, value.addr));
        emit_store(L, IR_I64, addr_add(slice.addr, 8), emit_const(L, IR_I64, fixed_array_count((TypeArray *) from)));
        return slice;
    }

    if (to->kind == TYPE_ANY && from->kind != TYPE_ANY) {
        return box_to_any(L, value, to, site);
    }
    if (from->kind == TYPE_ANY && to->kind != TYPE_ANY) {
        return unbox_any(L, value, to);
    }

    // Everything else that the typer allows shares the same memory layout for the part being
    // used (slice <-> dynamic array data/count, struct downcasts, variadic <-> slice ...)
    value.type = to;
    return value;
}

// The table of the methods of a type for an interface, in read-only data: the descriptor of the type, for 'as' and
// 'switch', and then the methods in the order of the interface. One for each type and interface
static int interface_table_global(Lowerer *L, Type *type, TypeStruct *interface, Ast *site) {
    char key[600];
    snprintf(key, sizeof(key), "%s|%s", type_identity(type), type_identity((Type *) interface));
    for (int i = 0; i < L->interface_tables.count; i++) {
        if (strcmp(L->interface_tables.items[i].key, key) == 0) return L->interface_tables.items[i].global;
    }
    DynamicArray methods = interface_methods(interface);
    IrGlobal *table = ir_global_data(L->module, "interface_table", NULL, 8 * (1 + methods.count), 8, true);
    int index = table->index;
    TypeInfoEntry entry = { ir_strdup(L->module, key), index };
    ir_array_push(L->interface_tables, entry);   // Before the methods, which can make tables too

    write_pointer_field(table, 0, type_info_global(L, type));
    for (int i = 0; i < methods.count; i++) {
        AstFunctionDefn *wanted = ((AstFunctionDefn **) methods.items)[i];
        AstFunctionDefn *method = method_for_interface(active_typer, type, wanted->identifier->name);
        if (method == NULL) lower_fail(L, site, "Compiler Error: %s has no method '%s' for %s", type_to_str(type), wanted->identifier->name, interface->head.name);
        if (is_generic_function(method)) lower_fail(L, site, "Compiler Error: the method '%s' of %s for %s is generic", wanted->identifier->name, type_to_str(type), interface->head.name);
        write_pointer_field(L->module->globals.items[index], 8 * (1 + i), function_global(L, method));
    }
    da_free(&methods);
    return index;
}

// A pointer, or null, as a value of an interface: the pointer and the table of the methods of its type. A null pointer
// is a null interface, with no table, so 'x == null' tells for both
static Value make_interface_value(Lowerer *L, Value value, TypeStruct *interface, Ast *site) {
    Value result = temp_place(L, (Type *) interface, "interface");
    Type *from = value.type;
    if (from == NULL || from->kind != TYPE_POINTER || ((TypePointer *) from)->pointer_to->kind == TYPE_VOID) {
        emit_store(L, IR_PTR, result.addr, emit_const(L, IR_PTR, 0));
        emit_store(L, IR_PTR, addr_add(result.addr, 8), emit_const(L, IR_PTR, 0));
        return result;
    }
    int ptr   = value_to_scalar(L, value);
    int table = emit_global_addr(L, interface_table_global(L, ((TypePointer *) from)->pointer_to, interface, site));
    emit_store(L, IR_PTR, result.addr, ptr);
    // The table only when there is a value
    IrBlock *has_value = create_block(L, "interface.value");
    IrBlock *no_value  = create_block(L, "interface.null");
    IrBlock *done      = create_block(L, "interface.made");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, ptr, emit_const(L, IR_PTR, 0)), has_value, no_value);
    set_block(L, has_value);
    emit_store(L, IR_PTR, addr_add(result.addr, 8), table);
    terminate_jmp(L, done);
    set_block(L, no_value);
    emit_store(L, IR_PTR, addr_add(result.addr, 8), emit_const(L, IR_PTR, 0));
    terminate_jmp(L, done);
    set_block(L, done);
    return result;
}

// Stores a value that has already been computed into memory of type 'dst_type'
static void store_value(Lowerer *L, IrAddr dst, Type *dst_type, Value src, Ast *site) {
    if (!type_is_aggregate(dst_type)) {
        int vreg = value_to_scalar(L, src);
        vreg = convert_scalar(L, vreg, src.type, dst_type);
        emit_store(L, ir_type_of(dst_type), dst, vreg);
        return;
    }
    Value converted = convert_aggregate(L, src, dst_type, site);
    copy_aggregate(L, dst, converted, dst_type->size);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Literals
//////////////////////////////////////////////////////////////////////////////////////////

static void lower_struct_literal_into(Lowerer *L, AstStructLiteral *lit, IrAddr dst) {
    emit_zero(L, dst, lit->head.type->size);

    // Members that are not mentioned get their default value
    DynamicArray members = ((TypeStruct *) lit->head.type)->node->members->identifiers;
    for (int m = 0; m < members.count; m++) {
        AstIdentifier *member = da_get_deref(members, m);
        if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
        bool initialized = false;
        for (int i = 0; i < lit->initializers.count; i++) {
            AstStructInitializer *init = da_get_deref(lit->initializers, i);
            if (init->member == member) { initialized = true; break; }
        }
        if (initialized) continue;

        AstExpr *value = member_default_value(member);
        IrAddr member_addr = addr_add(dst, member->member_offset);
        if (value) {
            store_expr(L, member_addr, member->type, value, true);
        } else if (has_default_values(member->type)) {
            emit_default_values(L, member_addr, member->type);
        }
    }

    for (int i = 0; i < lit->initializers.count; i++) {
        AstStructInitializer *init = da_get_deref(lit->initializers, i);
        store_expr(L, addr_add(dst, init->member->member_offset), init->member->type, init->value, true);
    }
}

// Writes the elements of an array literal to 'data' (a ptr vreg or a place address)
static void lower_array_elements_into(Lowerer *L, AstArrayLiteral *lit, IrAddr data, Type *elem_type) {
    for (int i = 0; i < lit->expressions.count; i++) {
        AstExpr *elem = da_get_deref(lit->expressions, i);
        store_expr(L, addr_add(data, i * elem_type->size), elem_type, elem, true);
    }
}

static void lower_array_literal_into(Lowerer *L, AstArrayLiteral *lit, IrAddr dst) {
    TypeArray *array_type = (TypeArray *) lit->head.type;
    Type      *elem_type  = array_type->elem_type;
    int        count      = lit->expressions.count;

    switch (array_type->array_kind) {
    case ARRAY_FIXED: {
        emit_zero(L, dst, lit->head.type->size);
        lower_array_elements_into(L, lit, dst, elem_type);
        return;
    }
    case ARRAY_SLICE: {
        // The elements live in a temporary array in the current function. The slice points to it
        int data_ptr;
        if (count == 0) {
            data_ptr = emit_const(L, IR_PTR, 0);
        } else {
            int backing = ir_new_slot(L->func, count * elem_type->size, type_alignment(elem_type), "slice data");
            lower_array_elements_into(L, lit, addr_slot(backing, 0), elem_type);
            data_ptr = emit_slot_addr(L, backing);
        }
        emit_store(L, IR_PTR, dst, data_ptr);
        emit_store(L, IR_I64, addr_add(dst, 8), emit_const(L, IR_I64, count));
        return;
    }
    case ARRAY_DYNAMIC:
        lower_fail(L, (Ast *) lit, "Compiler Error: Dynamic arrays are List<T> from the standard collections");
        return;
    }
}

// Lowers aggregate literals directly into their destination. Returns false if 'expr' is not such a literal.
// Only safe when the destination is fresh memory that the expression can not refer to.
static bool lower_into(Lowerer *L, AstExpr *expr, IrAddr dst, Type *dst_type) {
    if (expr->replacement) return false;
    if (expr->type != dst_type) return false;
    if (expr->head.kind == AST_ARRAY_LITERAL && ((AstArrayLiteral *) expr)->collection_decl) return false;
    if (expr->head.kind == AST_STRUCT_LITERAL) {
        lower_struct_literal_into(L, (AstStructLiteral *) expr, dst);
        return true;
    }
    if (expr->head.kind == AST_ARRAY_LITERAL) {
        lower_array_literal_into(L, (AstArrayLiteral *) expr, dst);
        return true;
    }
    return false;
}

// Evaluates 'expr' and stores it into memory of type 'dst_type'
static void store_expr(Lowerer *L, IrAddr dst, Type *dst_type, AstExpr *expr, bool dst_is_fresh) {
    if (!type_is_aggregate(dst_type)) {
        emit_store(L, ir_type_of(dst_type), dst, lower_scalar_as(L, expr, dst_type));
        return;
    }
    if (dst_is_fresh && lower_into(L, expr, dst, dst_type)) return;

    Value src = lower_expr(L, expr);
    store_value(L, dst, dst_type, src, (Ast *) expr);
}

static Value lower_literal(Lowerer *L, AstLiteral *lit) {
    Type *type = lit->head.type;
    switch (lit->kind) {
    case LITERAL_INTEGER: {
        IrType ir_type = ir_type_of(type);
        if (ir_type_is_float(ir_type)) return value_scalar(emit_fconst(L, ir_type, (double) lit->as.value.integer), type);
        return value_scalar(emit_const(L, ir_type, lit->as.value.integer), type);
    }
    case LITERAL_FLOAT:
        return value_scalar(emit_fconst(L, ir_type_of(type), lit->as.value.floating), type);
    case LITERAL_BOOLEAN:
        return value_scalar(emit_const(L, IR_I8, lit->as.value.boolean ? 1 : 0), type);
    case LITERAL_NULL:
        return value_scalar(emit_const(L, IR_PTR, 0), type);
    case LITERAL_STRING: {
        if (type->kind == TYPE_STRING) {
            int global = string_global(L, lit->as.value.string.data, lit->as.value.string.length);
            return value_place(addr_global(global, 0), type);
        }
        // A string literal typed as a pointer (e.g passed to a *u8 parameter) is just the character data, which ends with
        // a 0. A c.string is a struct of that pointer
        int len = 0;
        int data = string_data_global(L, lit->as.value.string.data, lit->as.value.string.length, &len);
        if (type->kind == TYPE_STRUCT) {
            Value place = temp_place(L, type, "c string");
            emit_store(L, IR_PTR, place.addr, emit_global_addr(L, data));
            return place;
        }
        return value_scalar(emit_global_addr(L, data), type);
    }
    case LITERAL_IDENTIFIER:
        break;
    }
    assert(false);
    return value_none();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Identifiers and constants
//////////////////////////////////////////////////////////////////////////////////////////

static Value function_address(Lowerer *L, AstFunctionDefn *func_defn, Type *type) {
    return value_scalar(emit_global_addr(L, function_global(L, func_defn)), type);
}

// The global holding a slice of the names or values of an enum (e.g `Fruit.names`)
static int enum_static_slice_global(Lowerer *L, AstIdentifier *member, AstEnum *ast_enum, bool names) {
    int cached = ptr_map_get(&L->const_globals, member);
    if (cached >= 0) return cached;

    int count = ast_enum->enumerators.count;
    TypeEnum *enum_type = (TypeEnum *) ast_enum->identifier->type;
    int elem_size = names ? 16 : enum_type->backing_type->size;

    IrGlobal *data = ir_global_data(L->module, names ? "enum_names" : "enum_values", NULL, count * elem_size, 8, true);
    for (int i = 0; i < count; i++) {
        AstEnumerator *etor = da_get_deref(ast_enum->enumerators, i);
        if (names) {
            int len = 0;
            int str = string_data_global(L, etor->name, (int) strlen(etor->name), &len);
            IrReloc reloc = { i * 16, str };
            ir_array_push(data->relocs, reloc);
            write_int_bytes(&data->data[i * 16 + 8], len, 8);
        } else {
            write_int_bytes(&data->data[i * elem_size], etor->value, elem_size);
        }
    }

    int64_t header[2] = { 0, count };
    IrGlobal *slice = ir_global_data(L->module, names ? "enum_names_slice" : "enum_values_slice", header, 16, 8, true);
    IrReloc reloc = { 0, data->index };
    ir_array_push(slice->relocs, reloc);

    ptr_map_put(&L->const_globals, member, slice->index);
    return slice->index;
}

static Value lower_constant(Lowerer *L, AstIdentifier *ident, Ast *site) {
    AstExpr *value = ident->value;
    Type    *type  = ident->type;

    if (type && type->kind == TYPE_FUNCTION) {
        AstFunctionDefn *func_defn = ((TypeFunction *) type)->node;
        return function_address(L, func_defn, type);
    }
    if (value == NULL) {
        lower_fail(L, site, "Compiler Error: Constant '%s' has no value", ident->name);
    }
    if (value->head.kind == AST_FUNCTION_DEFN) {
        return function_address(L, (AstFunctionDefn *) value, value->type);
    }

    if (type_is_aggregate(type)) {
        return value_place(addr_global(aggregate_constant_global(L, ident), 0), type);
    }

    // Scalar constants are materialized at the point of use
    return value_scalar(lower_scalar_as(L, value, value->type), value->type);
}

static bool is_fixed_array(Type *type);

// Writes the default values of the struct members in 'type' into the data of a global
static void write_default_constants(Lowerer *L, IrGlobal *global, int offset, Type *type) {
    int64_t first;
    if (enum_default_value(type, &first)) {
        write_int_bytes(&global->data[offset], first, type->size);
        return;
    }
    if (type->kind == TYPE_STRUCT && !((TypeStruct *) type)->node->is_union) {
        DynamicArray members = ((TypeStruct *) type)->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
            AstExpr *value = member_default_value(member);
            if (value) write_constant(L, global, offset + member->member_offset, value, member->type);
            else       write_default_constants(L, global, offset + member->member_offset, member->type);
        }
    }
    else if (is_fixed_array(type) && has_default_values(((TypeArray *) type)->elem_type)) {
        Type *elem = ((TypeArray *) type)->elem_type;
        for (int64_t i = 0; i < fixed_array_count((TypeArray *) type); i++) {
            write_default_constants(L, global, offset + (int)(i * elem->size), elem);
        }
    }
}

// The global holding a global variable e.g 'count := 0;', with its start value
static void write_attributed_functions(Lowerer *L, IrGlobal *list);

static int global_variable(Lowerer *L, AstIdentifier *ident) {
    int cached = ptr_map_get(&L->const_globals, ident);
    if (cached >= 0) return cached;

    // attributedFunctionList of runtime.sd: the functions with attribute values
    if (L->runtime_scope && strcmp(ident->name, "attributedFunctionList") == 0 && find_identifier_in_scope(L->runtime_scope, ident->name) == ident) {
        IrGlobal *list = ir_global_data(L->module, ident->name, NULL, 16, 8, false);
        ptr_map_put(&L->const_globals, ident, list->index);
        write_attributed_functions(L, list);
        return list->index;
    }

    Type *type = ident->type;
    int size = type_storage_size(type);
    IrGlobal *global = ir_global_data(L->module, ident->name, NULL, size > 0 ? size : 1, type_alignment(type), false);
    ptr_map_put(&L->const_globals, ident, global->index);

    AstExpr *value = NULL;
    AstDeclaration *decl = ident->decl;
    for (int i = 0; decl && i < decl->idents.count && i < decl->values.count; i++) {
        if (da_get_deref(decl->idents, i) == ident) value = da_get_deref(decl->values, i);
    }
    if (value) write_constant(L, global, 0, value, type);
    else       write_default_constants(L, global, 0, type);
    return global->index;
}

//
// @threadLocal globals: each thread has a block of them, a copy of their start values (L->thread_template). A function
// that uses one loads the block of its thread when it starts, into a slot, in a block of its own before the others
// (lower_function). The block is made the first time a thread needs it, by newThreadBlock() of std/prelude/thread_blocks.sd,
// and main sets up the key of the blocks with startThreadBlocks() first. thrownError is one of them
//

static void layout_thread_locals(Lowerer *L) {
    DynamicArray locals = L->parser->thread_locals;
    int size = 0;
    for (int i = 0; i < locals.count; i++) {
        AstIdentifier *ident = da_get_deref(locals, i);
        if (ident->type == NULL || (ident->flags & IDENTIFIER_IS_POISONED)) continue;
        int align = type_alignment(ident->type);
        if (align < 1) align = 1;
        size = (size + align - 1) / align * align;
        ptr_map_put(&L->thread_offsets, ident, size);
        size += type_storage_size(ident->type);
    }
    L->thread_block_size = size;
    IrGlobal *template = ir_global_data(L->module, "thread_locals", NULL, size > 0 ? size : 8, 16, false);
    L->thread_template = template->index;
    for (int i = 0; i < locals.count; i++) {
        AstIdentifier *ident = da_get_deref(locals, i);
        int offset = ptr_map_get(&L->thread_offsets, ident);
        if (offset < 0) continue;
        AstExpr *value = NULL;
        AstDeclaration *decl = ident->decl;
        for (int k = 0; decl && k < decl->idents.count && k < decl->values.count; k++) {
            if (da_get_deref(decl->idents, k) == ident) value = da_get_deref(decl->values, k);
        }
        if (value) write_constant(L, template, offset, value, ident->type);
        else       write_default_constants(L, template, offset, ident->type);
    }
}

static AstIdentifier *errors_declaration(Lowerer *L, const char *name);
// The files of the prelude that the compiler calls into before a function is set up, e.g to throw or for the block of
// the @threadLocal globals. Their pointers aren't checked (null, --check-memory), as the checks call them
static bool is_runtime_file(Lowerer *L, AstFile *file) {
    return file != NULL && (file == L->parser->errors_file || file == L->parser->allocator_file || file == L->parser->thread_blocks_file);
}
static void emit_errors_call(Lowerer *L, const char *name, int arg_count, int *args);

// Looks up the block of the thread, and makes it the first time: FlsGetValue or pthread_getspecific with the key of
// startThreadBlocks(), and newThreadBlock() when there is none yet
static int emit_lookup_thread_block(Lowerer *L) {
    bool windows = strcmp(target_os, "windows") == 0;
    AstIdentifier *get = errors_declaration(L, windows ? "FlsGetValue" : "pthread_getspecific");
    int get_global = function_global(L, ((TypeFunction *)get->type)->node);
    AstIdentifier *key_ident = errors_declaration(L, "threadBlockKey");
    int slot = ir_new_slot(L->func, 8, 8, "thread block");

    int key = emit_load(L, IR_I32, addr_global(global_variable(L, key_ident), 0));
    IrCall *call = new_call(L, IR_CALLEE_DIRECT, get_global, IR_PTR, 1);
    call_add_arg(call, key, false);
    int block = emit_call(L, call);
    emit_store(L, IR_PTR, addr_slot(slot, 0), block);
    IrBlock *make = create_block(L, "new thread block");
    IrBlock *done = create_block(L, "thread block");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_PTR, block, emit_const(L, IR_PTR, 0)), make, done);

    // The first function of a thread that uses the block
    set_block(L, make);
    emit_errors_call(L, "newThreadBlock", 0, NULL);
    key = emit_load(L, IR_I32, addr_global(global_variable(L, key_ident), 0));
    call = new_call(L, IR_CALLEE_DIRECT, get_global, IR_PTR, 1);
    call_add_arg(call, key, false);
    emit_store(L, IR_PTR, addr_slot(slot, 0), emit_call(L, call));
    terminate_jmp(L, done);

    set_block(L, done);
    return emit_load(L, IR_PTR, addr_slot(slot, 0));
}

// The block of the @threadLocal globals of the thread. A function that uses a @threadLocal global of the program looks
// it up once, when it starts (the prologue of lower_function). Where an error is thrown or caught, it is looked up
// there, as that is rare
static int thread_block(Lowerer *L, bool rare) {
    if (rare) return emit_lookup_thread_block(L);
    if (L->thread_block_slot < 0) L->thread_block_slot = ir_new_slot(L->func, 8, 8, "thread block");
    return emit_load(L, IR_PTR, addr_slot(L->thread_block_slot, 0));
}

// Where a global variable is: in the data of the program, or in the block of the thread
static IrAddr global_variable_addr_of(Lowerer *L, AstIdentifier *ident, bool rare) {
    int offset = (ident->flags & IDENTIFIER_IS_THREAD_LOCAL) ? ptr_map_get(&L->thread_offsets, ident) : -1;
    if (offset >= 0) return addr_vreg(thread_block(L, rare), offset);
    return addr_global(global_variable(L, ident), 0);
}

static IrAddr global_variable_addr(Lowerer *L, AstIdentifier *ident) {
    return global_variable_addr_of(L, ident, false);
}

static Value lower_identifier(Lowerer *L, AstLiteral *lit) {
    AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
    if (ident == NULL) lower_fail(L, (Ast *) lit, "Compiler Error: Unresolved identifier");
    ident = resolve_override(ident);

    if (ident->flags & IDENTIFIER_IS_CONSTANT) {
        return lower_constant(L, ident, (Ast *) lit);
    }

    AstIdentifier *declared = ident->imported_from ? ident->imported_from : ident;
    if (declared->flags & IDENTIFIER_IS_GLOBAL) {
        return value_place(global_variable_addr(L, declared), declared->type);
    }

    int slot = local_slot(L, ident);
    if (slot < 0) {
        if (ident->type && ident->type->kind == TYPE_FUNCTION) {
            return function_address(L, ((TypeFunction *) ident->type)->node, ident->type);
        }
        lower_fail(L, (Ast *) lit, "Compiler Error: No storage for identifier '%s' (globals are not supported yet)", ident->name);
    }
    return value_place(addr_slot(slot, 0), ident->type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Member and array access
//////////////////////////////////////////////////////////////////////////////////////////

static bool is_fixed_array(Type *type) {
    return type->kind == TYPE_ARRAY && ((TypeArray *) type)->array_kind == ARRAY_FIXED;
}

// The address of the struct that a member access reads from. Pointers are dereferenced automatically
static IrAddr member_base(Lowerer *L, AstExpr *left, AstExpr *access) {
    if (left->type->kind == TYPE_POINTER) {
        int ptr = value_to_scalar(L, lower_expr(L, left));
        if (access != address_taken_of) emit_null_check(L, ptr, left);
        return addr_vreg(ptr, 0);
    }
    return lower_place(L, left).addr;
}

static Value lower_member_access(Lowerer *L, AstMemberAccess *ma) {
    Type *type = ma->head.type;

    switch (ma->access_kind) {
    case MEMBER_ACCESS_STRUCT_MEMBER: {
        AstIdentifier *member = ma->struct_member;

        // Fixed arrays only have pseudo members
        if (is_fixed_array(ma->left->type)) {
            TypeArray *array = (TypeArray *) ma->left->type;
            if (member->member_index == 0) {
                return value_scalar(addr_to_ptr(L, lower_place(L, ma->left).addr), type);
            }
            if (member->member_index == 1) {
                return value_scalar(emit_const(L, ir_type_of(type), fixed_array_count(array)), type);
            }
            LOWER_NOT_IMPLEMENTED(L, ma, "this member of a fixed array");
        }

        IrAddr base = member_base(L, ma->left, (AstExpr *) ma);
        return value_place(addr_add(base, member->member_offset), type);
    }
    case MEMBER_ACCESS_STATIC_STRUCT_MEMBER:
        return lower_constant(L, ma->struct_member, (Ast *) ma);
    case MEMBER_ACCESS_ENUM_MEMBER: {
        if (ma->enum_member->head.kind == AST_IDENTIFIER) {
            // count, names or values
            AstIdentifier *member = (AstIdentifier *) ma->enum_member;
            AstEnum *ast_enum = (AstEnum *) member->decl;
            if (strcmp(member->name, "count") == 0) {
                return value_scalar(emit_const(L, ir_type_of(type), ast_enum->enumerators.count), type);
            }
            bool names = strcmp(member->name, "names") == 0;
            if (!names && strcmp(member->name, "values") != 0) LOWER_NOT_IMPLEMENTED(L, ma, "this static enum member");
            int global = enum_static_slice_global(L, member, ast_enum, names);
            return value_place(addr_global(global, 0), type);
        }
        return value_scalar(emit_const(L, ir_type_of(type), ma->enum_member->value), type);
    }
    }
    assert(false);
    return value_none();
}

static IrAddr array_access_addr(Lowerer *L, AstArrayAccess *aa) {
    Type *left_type = aa->left->type;
    Type *elem_type = aa->head.type;

    // Constant index into a fixed array: fold the offset into the address
    if (is_fixed_array(left_type) && aa->index_expr->head.kind == AST_LITERAL && ((AstLiteral *) aa->index_expr)->kind == LITERAL_INTEGER) {
        int64_t index = ((AstLiteral *) aa->index_expr)->as.value.integer;
        return addr_add(lower_place(L, aa->left).addr, (int)(index * elem_type->size));
    }

    int data;
    int count = -1; // The number of elements, for the bounds check. A pointer has none
    if (left_type->kind == TYPE_POINTER) {
        data = value_to_scalar(L, lower_expr(L, aa->left));
        if ((AstExpr *) aa != address_taken_of) emit_null_check(L, data, aa->left);
    } else if (is_fixed_array(left_type)) {
        data = addr_to_ptr(L, lower_place(L, aa->left).addr);
        count = emit_const(L, IR_I64, fixed_array_count((TypeArray *) left_type));
    } else {
        // slices, dynamic arrays, variadics and strings all start with a data pointer, and the count after it
        IrAddr place = lower_place(L, aa->left).addr;
        data  = emit_load(L, IR_PTR, place);
        if (aa->head.head.flags & AST_FLAG_IS_ARRAY_INDEX_INTO_POINTER) {
            // A *T that the typer gave the type of a slice, e.g p[i]
            if ((AstExpr *) aa != address_taken_of) emit_null_check(L, data, aa->left);
        } else {
            count = emit_load(L, IR_I64, addr_add(place, 8));
        }
    }

    int index  = lower_scalar_as(L, aa->index_expr, primitive_type(PRIMITIVE_I64));

    // An index outside of the elements throws a Panic. The compare is unsigned, so a negative index is outside too
    bool into_pointer = (aa->head.head.flags & AST_FLAG_IS_ARRAY_INDEX_INTO_POINTER) != 0; // e.g self.data[i] of a *T
    if (count >= 0 && bounds_checks && !into_pointer) {
        IrBlock *outside = create_block(L, "index.out.of.bounds");
        IrBlock *inside  = create_block(L, "index");
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_ULT, IR_I64, index, count), inside, outside);
        set_block(L, outside);
        int args[2] = { index, count };
        emit_errors_call(L, "panicIndexOutOfBounds", 2, args);
        emit_trace_frame(L, aa->head.head.start.line);
        emit_unwind(L);
        set_block(L, inside);
    }
    // The bytes of a string, slice or list, which can be in an arena that was cleared (--check-memory)
    if (count >= 0 && !is_fixed_array(left_type) && (AstExpr *) aa != address_taken_of) emit_memory_check(L, data, aa->left);
    int offset = emit_binop(L, IR_MUL, IR_I64, index, emit_const(L, IR_I64, elem_type->size));
    return addr_vreg(emit_ptr_add(L, data, offset), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Operators
//////////////////////////////////////////////////////////////////////////////////////////

// If the integer arithmetic of the function being lowered is checked for overflow: unless built with --unchecked, or the
// function is marked @noOverflowCheck, e.g a hash that wraps around on purpose, or it is a file of the runtime
static bool overflow_checks_on(Lowerer *L) {
    if (!bounds_checks || !L->ast_func) return false;
    AstFunctionDefn *f = L->ast_func;
    if (f->no_overflow_check || (f->generic_function && f->generic_function->no_overflow_check)) return false;
    AstFile *file = f->head.head.file;
    return file != NULL && !is_runtime_file(L, file);
}

// The smallest and the biggest value of an integer type of fewer than 64 bits
static void integer_range(Type *type, int64_t *low, int64_t *high) {
    int bits = type->size * 8;
    if (type_is_signed(type)) { *low = -((int64_t)1 << (bits - 1)); *high = ((int64_t)1 << (bits - 1)) - 1; }
    else                      { *low = 0;                          *high = ((int64_t)1 << bits) - 1; }
}

// Throws a Panic that says which operation didn't fit, e.g '2147483647 + 1 doesn't fit in i32', when 'fits' (an i8) is 0
static void emit_overflow_branch(Lowerer *L, int fits, int op, Type *type, int a64, int b64, bool unary, Ast *site) {
    IrBlock *overflow = create_block(L, "integer.overflow");
    IrBlock *go_on    = create_block(L, "integer.fits");
    terminate_br(L, fits, go_on, overflow);
    set_block(L, overflow);
    int args[6] = {
        a64, b64 == IR_NO_VREG ? emit_const(L, IR_I64, 0) : b64,
        emit_const(L, IR_I64, op), emit_const(L, IR_I8, unary ? 1 : 0),
        emit_const(L, IR_I8, type_is_signed(type) ? 0 : 1), emit_c_text(L, type_to_str(type)),
    };
    emit_errors_call(L, "panicOverflow", 6, args);
    emit_trace_frame(L, site ? site->start.line : L->line);
    emit_unwind(L);
    set_block(L, go_on);
}

// a op b of + - * with a check that the result fits the type
static int emit_checked_arithmetic(Lowerer *L, int op, Type *type, int a, int b, Ast *site) {
    IrType ir_type   = ir_type_of(type);
    bool   is_signed = type_is_signed(type);
    IrOp   ir_op     = op == '+' ? IR_ADD : op == '-' ? IR_SUB : IR_MUL;
    Type  *i64_type  = primitive_type(PRIMITIVE_I64);

    if (type->size < 8) {
        // In 64 bits, where it can't overflow, and then in the range of the type
        int wide_a = convert_scalar(L, a, type, i64_type);
        int wide_b = convert_scalar(L, b, type, i64_type);
        int wide   = emit_binop(L, ir_op, IR_I64, wide_a, wide_b);
        int64_t low, high;
        integer_range(type, &low, &high);
        int above = emit_cmp(L, IR_ICMP, IR_CMP_SGE, IR_I64, wide, emit_const(L, IR_I64, low));
        int below = emit_cmp(L, IR_ICMP, IR_CMP_SLE, IR_I64, wide, emit_const(L, IR_I64, high));
        emit_overflow_branch(L, emit_binop(L, IR_AND, IR_I8, above, below), op, type, wide_a, wide_b, false, site);
        return convert_scalar(L, wide, i64_type, type);
    }

    int result = emit_binop(L, ir_op, ir_type, a, b);
    int fits;
    if (op == '+' || op == '-') {
        if (is_signed) {
            // Signs: a + b overflows when both have a sign the result doesn't, a - b when a and b differ and a and the result do
            int x = emit_binop(L, IR_XOR, IR_I64, a, result);
            int y = op == '+' ? emit_binop(L, IR_XOR, IR_I64, b, result) : emit_binop(L, IR_XOR, IR_I64, a, b);
            int both = emit_binop(L, IR_AND, IR_I64, x, y);
            fits = emit_cmp(L, IR_ICMP, IR_CMP_SGE, IR_I64, both, emit_const(L, IR_I64, 0));
        } else if (op == '+') {
            fits = emit_cmp(L, IR_ICMP, IR_CMP_UGE, IR_I64, result, a);
        } else {
            fits = emit_cmp(L, IR_ICMP, IR_CMP_UGE, IR_I64, a, b);
        }
        emit_overflow_branch(L, fits, op, type, a, b, false, site);
        return result;
    }

    // a * b: fits when a is 0, or result / a is b. For a signed -1, when b isn't the smallest value, as that division traps
    IrBlock *divide = create_block(L, "multiply.check");
    IrBlock *done   = create_block(L, "multiply.fits");
    int fits_slot = ir_new_slot(L->func, 1, 1, "multiply fits");
    emit_store(L, IR_I8, addr_slot(fits_slot, 0), emit_const(L, IR_I8, 1));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, a, emit_const(L, IR_I64, 0)), done, divide);
    set_block(L, divide);
    if (is_signed) {
        IrBlock *minus_one = create_block(L, "multiply.minus.one");
        IrBlock *other     = create_block(L, "multiply.other");
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, a, emit_const(L, IR_I64, -1)), minus_one, other);
        set_block(L, minus_one);
        emit_store(L, IR_I8, addr_slot(fits_slot, 0), emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_I64, b, emit_const(L, IR_I64, INT64_MIN)));
        terminate_jmp(L, done);
        set_block(L, other);
        int back = emit_binop(L, IR_SDIV, IR_I64, result, a);
        emit_store(L, IR_I8, addr_slot(fits_slot, 0), emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, back, b));
        terminate_jmp(L, done);
    } else {
        int back = emit_binop(L, IR_UDIV, IR_I64, result, a);
        emit_store(L, IR_I8, addr_slot(fits_slot, 0), emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, back, b));
        terminate_jmp(L, done);
    }
    set_block(L, done);
    fits = emit_load(L, IR_I8, addr_slot(fits_slot, 0));
    emit_overflow_branch(L, fits, op, type, a, b, false, site);
    return result;
}

static int emit_arithmetic(Lowerer *L, int op, Type *type, int a, int b, Ast *site) {
    IrType ir_type = ir_type_of(type);
    if (ir_type_is_float(ir_type)) {
        switch (op) {
        case '+': return emit_binop(L, IR_FADD, ir_type, a, b);
        case '-': return emit_binop(L, IR_FSUB, ir_type, a, b);
        case '*': return emit_binop(L, IR_FMUL, ir_type, a, b);
        case '/': return emit_binop(L, IR_FDIV, ir_type, a, b);
        }
        lower_fail(L, site, "Operator '%s' is not supported on floats", token_type_to_str(op));
    }
    if (!ir_type_is_int(ir_type)) {
        lower_fail(L, site, "Not implemented in the IR backend yet: arithmetic on type %s", type_to_str(type));
    }

    bool is_signed = type_is_signed(type);

    // + - * of integers throw a Panic when the result doesn't fit the type
    // A constant without a type, e.g -9223372036854775807 - 1, is checked when its value is fitted to a type
    if ((op == '+' || op == '-' || op == '*') && type->kind == TYPE_INTEGER && !type_is_untyped_int(type) && overflow_checks_on(L)) {
        return emit_checked_arithmetic(L, op, type, a, b, site);
    }

    // A division by zero throws a Panic
    if (op == '/' || op == '%') {
        IrBlock *zero  = create_block(L, "division.by.zero");
        IrBlock *go_on = create_block(L, "division");
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, b, emit_const(L, ir_type, 0)), zero, go_on);
        set_block(L, zero);
        emit_errors_call(L, "panicDivisionByZero", 0, NULL);
        emit_trace_frame(L, site ? site->start.line : L->line);
        emit_unwind(L);
        set_block(L, go_on);

        // The smallest value divided by -1 doesn't fit, and the processor traps on it, for % too. x % -1 is 0, which x % 1 gives
        if (is_signed && type->kind == TYPE_INTEGER) {
            int64_t low = INT64_MIN, high;
            if (type->size < 8) integer_range(type, &low, &high);
            int minus_one = emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, b, emit_const(L, ir_type, -1));
            if (op == '%') {
                b = emit_binop(L, IR_ADD, ir_type, b, emit_binop(L, IR_MUL, ir_type, convert_scalar(L, minus_one, primitive_type(PRIMITIVE_BOOL), type), emit_const(L, ir_type, 2)));
            } else {
                int smallest = emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, a, emit_const(L, ir_type, low));
                int traps    = emit_binop(L, IR_AND, IR_I8, minus_one, smallest);
                int fits     = emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I8, traps, emit_const(L, IR_I8, 0));
                Type *i64_type = primitive_type(PRIMITIVE_I64);
                emit_overflow_branch(L, fits, op, type, convert_scalar(L, a, type, i64_type), convert_scalar(L, b, type, i64_type), false, site);
            }
        }
    }

    switch (op) {
    case '+': return emit_binop(L, IR_ADD, ir_type, a, b);
    case '-': return emit_binop(L, IR_SUB, ir_type, a, b);
    case '*': return emit_binop(L, IR_MUL, ir_type, a, b);
    case '/': return emit_binop(L, is_signed ? IR_SDIV : IR_UDIV, ir_type, a, b);
    case '%': return emit_binop(L, is_signed ? IR_SREM : IR_UREM, ir_type, a, b);
    case '&': return emit_binop(L, IR_AND, ir_type, a, b);
    case '|': return emit_binop(L, IR_OR,  ir_type, a, b);
    case '^': return emit_binop(L, IR_XOR, ir_type, a, b);
    case TOKEN_BITWISE_SHIFT_LEFT:  return emit_binop(L, IR_SHL, ir_type, a, b);
    case TOKEN_BITWISE_SHIFT_RIGHT: return emit_binop(L, is_signed ? IR_ASHR : IR_LSHR, ir_type, a, b);
    }
    lower_fail(L, site, "Not implemented in the IR backend yet: operator '%s'", token_type_to_str(op));
    return IR_NO_VREG;
}

// The type both sides of a comparison are converted to before comparing
static Type *comparison_type(Type *left, Type *right) {
    Type *l = enum_backing_type(left);
    Type *r = enum_backing_type(right);

    if (l->kind == TYPE_FLOAT || r->kind == TYPE_FLOAT) {
        if (l->kind != TYPE_FLOAT) return right;
        if (r->kind != TYPE_FLOAT) return left;
        if (type_is_untyped_float(l)) return right;
        if (type_is_untyped_float(r)) return left;
        return l->size >= r->size ? left : right;
    }
    if (l->kind == TYPE_INTEGER && r->kind == TYPE_INTEGER) {
        if (type_is_untyped_int(l)) return right;
        if (type_is_untyped_int(r)) return left;
        return l->size >= r->size ? left : right;
    }
    return left;
}

static IrCmp comparison_predicate(int op, bool is_float, bool is_signed) {
    if (is_float) {
        switch (op) {
        case TOKEN_DOUBLE_EQUAL:  return IR_CMP_OEQ;
        case TOKEN_NOT_EQUAL:     return IR_CMP_UNE;
        case '<':                 return IR_CMP_OLT;
        case TOKEN_LESS_EQUAL:    return IR_CMP_OLE;
        case '>':                 return IR_CMP_OGT;
        case TOKEN_GREATER_EQUAL: return IR_CMP_OGE;
        }
        return IR_CMP_NONE;
    }
    switch (op) {
    case TOKEN_DOUBLE_EQUAL:  return IR_CMP_EQ;
    case TOKEN_NOT_EQUAL:     return IR_CMP_NE;
    case '<':                 return is_signed ? IR_CMP_SLT : IR_CMP_ULT;
    case TOKEN_LESS_EQUAL:    return is_signed ? IR_CMP_SLE : IR_CMP_ULE;
    case '>':                 return is_signed ? IR_CMP_SGT : IR_CMP_UGT;
    case TOKEN_GREATER_EQUAL: return is_signed ? IR_CMP_SGE : IR_CMP_UGE;
    }
    return IR_CMP_NONE;
}

// Compares two values of the same type in memory. Returns an i8 that is 1 if they are equal.
// Structs are compared member by member (the typer only allows members that can be compared)
static int emit_values_equal(Lowerer *L, IrAddr a, IrAddr b, Type *type) {
    if (type->kind == TYPE_STRUCT) {
        int result = emit_const(L, IR_I8, 1);
        DynamicArray members = ((TypeStruct *) type)->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;

            int equal = emit_values_equal(L, addr_add(a, member->member_offset), addr_add(b, member->member_offset), member->type);
            result = emit_binop(L, IR_AND, IR_I8, result, equal);
        }
        return result;
    }
    if (type->kind == TYPE_STRING) {
        int args[2] = { addr_to_ptr(L, a), addr_to_ptr(L, b) };
        int result  = emit_c_call(L, "runtime_compare_strings", IR_I32, 2, args);
        return emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I32, result, emit_const(L, IR_I32, 0));
    }

    IrType ir_type = ir_type_of(type);
    if (ir_type == IR_VOID) lower_fail(L, NULL, "Compiler Error: Cannot compare values of type %s", type_to_str(type));

    int value_a = emit_load(L, ir_type, a);
    int value_b = emit_load(L, ir_type, b);
    if (ir_type_is_float(ir_type)) return emit_cmp(L, IR_FCMP, IR_CMP_OEQ, ir_type, value_a, value_b);
    return emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, value_a, value_b);
}

static int lower_comparison(Lowerer *L, AstBinary *bin) {
    Type *left  = bin->left->type;
    Type *right = bin->right->type;

    // An optional compared with null, a value or an optional. An empty optional is all zeros, so optionals are equal when their memory is
    if (is_optional_type(left) || is_optional_type(right)) {
        AstExpr *optional = is_optional_type(left) ? bin->left : bin->right;
        AstExpr *other    = optional == bin->left ? bin->right : bin->left;
        Type    *type     = optional->type;

        int equal;
        IrAddr a = value_to_place(L, lower_expr(L, optional)).addr;
        if (is_null_type(other->type)) {
            int present = emit_load(L, IR_I8, addr_add(a, optional_present_offset(type)));
            equal = emit_binop(L, IR_XOR, IR_I8, present, emit_const(L, IR_I8, 1));
        } else {
            IrAddr b = value_to_place(L, convert_aggregate(L, lower_expr(L, other), type, (Ast *) bin)).addr;
            equal = emit_values_equal(L, a, b, type);
        }
        if (bin->operator == TOKEN_NOT_EQUAL) return emit_binop(L, IR_XOR, IR_I8, equal, emit_const(L, IR_I8, 1));
        return equal;
    }

    // An interface with null: if it has a value
    if (is_interface_type(left) != is_interface_type(right) && (is_interface_type(left) || is_interface_type(right))) {
        AstExpr *interface = is_interface_type(left) ? bin->left : bin->right;
        IrAddr at = value_to_place(L, lower_expr(L, interface)).addr;
        int value = emit_load(L, IR_PTR, at);
        IrCmp cmp = bin->operator == TOKEN_NOT_EQUAL ? IR_CMP_NE : IR_CMP_EQ;
        return emit_cmp(L, IR_ICMP, cmp, IR_PTR, value, emit_const(L, IR_PTR, 0));
    }

    if (left->kind == TYPE_STRUCT && right->kind == TYPE_STRUCT) {
        IrAddr a = value_to_place(L, lower_expr(L, bin->left)).addr;
        IrAddr b = value_to_place(L, lower_expr(L, bin->right)).addr;
        int equal = emit_values_equal(L, a, b, left);
        if (bin->operator == TOKEN_NOT_EQUAL) return emit_binop(L, IR_XOR, IR_I8, equal, emit_const(L, IR_I8, 1));
        return equal;
    }

    if (left->kind == TYPE_STRING && right->kind == TYPE_STRING) {
        // runtime_compare_strings(*string, *string) -> int  (<0, 0, >0)
        int args[2];
        args[0] = addr_to_ptr(L, value_to_place(L, lower_expr(L, bin->left)).addr);
        args[1] = addr_to_ptr(L, value_to_place(L, lower_expr(L, bin->right)).addr);
        emit_string_memory_check(L, args[0], bin->left);    // C compares them, so they are checked before (--check-memory)
        emit_string_memory_check(L, args[1], bin->right);
        int result = emit_c_call(L, "runtime_compare_strings", IR_I32, 2, args);
        IrCmp cmp = comparison_predicate(bin->operator, false, true);
        return emit_cmp(L, IR_ICMP, cmp, IR_I32, result, emit_const(L, IR_I32, 0));
    }

    Type *type = comparison_type(left, right);
    int a = lower_scalar_as(L, bin->left,  type);
    int b = lower_scalar_as(L, bin->right, type);

    IrType ir_type = ir_type_of(type);
    if (ir_type == IR_VOID) {
        lower_fail(L, (Ast *) bin, "Not implemented in the IR backend yet: comparing values of type %s", type_to_str(type));
    }

    bool is_float = ir_type_is_float(ir_type);
    IrCmp cmp = comparison_predicate(bin->operator, is_float, type_is_signed(type));
    return emit_cmp(L, is_float ? IR_FCMP : IR_ICMP, cmp, ir_type, a, b);
}

// && and || used as a value: branch on the condition and store 0 or 1
static int lower_logical_value(Lowerer *L, AstExpr *expr) {
    int slot = ir_new_slot(L->func, 1, 1, "logical result");
    IrBlock *on_true  = create_block(L, "logical.true");
    IrBlock *on_false = create_block(L, "logical.false");
    IrBlock *done     = create_block(L, "logical.end");

    lower_condition(L, expr, on_true, on_false);

    set_block(L, on_true);
    emit_store(L, IR_I8, addr_slot(slot, 0), emit_const(L, IR_I8, 1));
    terminate_jmp(L, done);

    set_block(L, on_false);
    emit_store(L, IR_I8, addr_slot(slot, 0), emit_const(L, IR_I8, 0));
    terminate_jmp(L, done);

    set_block(L, done);
    return emit_load(L, IR_I8, addr_slot(slot, 0));
}

// a ?? b: the value of a, or b when a is empty. b is only computed then
static Value lower_optional_default(Lowerer *L, AstBinary *bin) {
    Type *type      = bin->head.type;
    Type *left_type = bin->left->type;

    Value left   = value_to_place(L, lower_expr(L, bin->left));
    Value result = temp_place(L, type, "?? result");

    IrBlock *has      = create_block(L, "optional.has");
    IrBlock *fallback = create_block(L, "optional.fallback");
    IrBlock *done     = create_block(L, "optional.end");

    int present = emit_load(L, IR_I8, addr_add(left.addr, optional_present_offset(left_type)));
    terminate_br(L, present, has, fallback);

    set_block(L, has);
    if (is_optional_type(type)) store_value(L, result.addr, type, left, (Ast *) bin);
    else                        store_value(L, result.addr, type, value_place(left.addr, optional_inner_type(left_type)), (Ast *) bin);
    terminate_jmp(L, done);

    set_block(L, fallback);
    store_value(L, result.addr, type, lower_expr(L, bin->right), (Ast *) bin);
    terminate_jmp(L, done);

    set_block(L, done);
    return result;
}

// a?.b or a?.m(x): the access on the value inside a, wrapped in an optional, or an empty optional when a is empty (or a null pointer)
static Value lower_optional_chain(Lowerer *L, AstOptionalChain *chain) {
    Type *type       = chain->head.type;
    bool  has_result = type->kind != TYPE_VOID;
    Value result     = has_result ? temp_place(L, type, "?. result") : value_none();

    Value left = lower_expr(L, chain->left);
    Value inside;
    int present;
    if (chain->through_pointer) {
        int ptr = value_to_scalar(L, left);
        present = emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, ptr, emit_const(L, IR_PTR, 0));
        inside  = value_scalar(ptr, chain->value->head.type);
    } else {
        left    = value_to_place(L, left);
        present = emit_load(L, IR_I8, addr_add(left.addr, optional_present_offset(chain->left->type)));
        inside  = value_place(left.addr, chain->value->head.type);
    }

    IrBlock *has   = create_block(L, "chain.has");
    IrBlock *empty = create_block(L, "chain.empty");
    IrBlock *done  = create_block(L, "chain.end");
    terminate_br(L, present, has, empty);

    set_block(L, has);
    void *saved = chain->value->lowered;
    chain->value->lowered = &inside;
    Value value = lower_expr(L, chain->inner);
    chain->value->lowered = saved;
    if (has_result) store_value(L, result.addr, type, value, (Ast *) chain);
    terminate_jmp(L, done);

    set_block(L, empty);
    if (has_result) emit_zero(L, result.addr, type_storage_size(type));
    terminate_jmp(L, done);

    set_block(L, done);
    return result;
}

static Value lower_binary(Lowerer *L, AstBinary *bin) {
    int op = bin->operator;
    Type *type = bin->head.type;

    if (op == TOKEN_DOUBLE_QUESTION) return lower_optional_default(L, bin);

    if (op == TOKEN_LOGICAL_AND || op == TOKEN_LOGICAL_OR) {
        return value_scalar(lower_logical_value(L, (AstExpr *) bin), type);
    }
    if (is_comparison_operator(op)) {
        return value_scalar(lower_comparison(L, bin), type);
    }

    int a = lower_scalar_as(L, bin->left,  type);
    int b = lower_scalar_as(L, bin->right, type);
    return value_scalar(emit_arithmetic(L, op, type, a, b, (Ast *) bin), type);
}

static Value lower_unary(Lowerer *L, AstUnary *unary) {
    Type *type = unary->head.type;

    switch (unary->operator) {
    case OP_UNARY_MINUS: {
        int value = lower_scalar_as(L, unary->expr, type);
        IrType ir_type = ir_type_of(type);
        // -x of the smallest signed value doesn't fit. A number written in the program, e.g -2147483648, is the number
        bool literal = unary->expr->head.kind == AST_LITERAL && ((AstLiteral *) unary->expr)->kind == LITERAL_INTEGER;
        if (type->kind == TYPE_INTEGER && type_is_signed(type) && !literal && !type_is_untyped_int(type) && overflow_checks_on(L)) {
            int64_t low = INT64_MIN, high;
            if (type->size < 8) integer_range(type, &low, &high);
            int fits = emit_cmp(L, IR_ICMP, IR_CMP_NE, ir_type, value, emit_const(L, ir_type, low));
            emit_overflow_branch(L, fits, '-', type, convert_scalar(L, value, type, primitive_type(PRIMITIVE_I64)), IR_NO_VREG, true, (Ast *) unary);
        }
        return value_scalar(emit_unop(L, ir_type_is_float(ir_type) ? IR_FNEG : IR_NEG, ir_type, value), type);
    }
    case OP_NOT: {
        Type *operand_type = unary->expr->type;
        int value = value_to_scalar(L, lower_expr(L, unary->expr));
        if (operand_type->kind == TYPE_BOOL) {
            return value_scalar(emit_binop(L, IR_XOR, IR_I8, value, emit_const(L, IR_I8, 1)), type);
        }
        // !pointer
        IrType ir_type = ir_vreg_type(L->func, value);
        return value_scalar(emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, value, emit_const(L, ir_type, 0)), type);
    }
    case OP_BITWISE_NOT: {
        int value = lower_scalar_as(L, unary->expr, type);
        return value_scalar(emit_unop(L, IR_NOT, ir_type_of(type), value), type);
    }
    case OP_ADDRESS_OF: {
        AstExpr *outer = address_taken_of;
        address_taken_of = unary->expr;
        Value place = lower_place(L, unary->expr);
        address_taken_of = outer;
        return value_scalar(addr_to_ptr(L, place.addr), type);
    }
    case OP_POINTER_DEREFERENCE: {
        int ptr = value_to_scalar(L, lower_expr(L, unary->expr));
        if ((AstExpr *) unary != address_taken_of) emit_null_check(L, ptr, unary->expr);
        return value_place(addr_vreg(ptr, 0), type);
    }
    case OP_SPREAD:
        return lower_expr(L, unary->expr);
    default:
        break;
    }
    LOWER_NOT_IMPLEMENTED(L, unary, "this unary operator");
    return value_none();
}

// The pointer of an interface value when its value is of the type 'to' points to, else null, and if it is: the type is
// the first entry of the table, the descriptor of the type
static int interface_as(Lowerer *L, AstExpr *expr, Type *to, int *is_type) {
    IrAddr at = value_to_place(L, lower_expr(L, expr)).addr;
    int value = emit_load(L, IR_PTR, at);
    int table = emit_load(L, IR_PTR, addr_add(at, 8));
    int wanted = emit_global_addr(L, type_info_global(L, ((TypePointer *) to)->pointer_to));
    Value type_slot = temp_place(L, primitive_type(PRIMITIVE_U64), "interface type");
    emit_store(L, IR_PTR, type_slot.addr, emit_const(L, IR_PTR, 0));
    IrBlock *has_table = create_block(L, "interface.has.table");
    IrBlock *done      = create_block(L, "interface.typed");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, table, emit_const(L, IR_PTR, 0)), has_table, done);
    set_block(L, has_table);
    emit_store(L, IR_PTR, type_slot.addr, emit_load(L, IR_PTR, addr_vreg(table, 0)));
    terminate_jmp(L, done);
    set_block(L, done);
    int type = emit_load(L, IR_PTR, type_slot.addr);
    *is_type = emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_PTR, type, wanted);
    return value;
}

// The offset of a member of Error, e.g "cause"
static int error_member_offset(TypeStruct *error, const char *name) {
    DynamicArray members = error->node->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)members.items)[i];
        if (strcmp(member->name, name) == 0) return member->member_offset;
    }
    assert(!"Compiler Error: a member of Error isn't there");
    return 0;
}

// err as *QueryError: the error of the type 'to' points to, in the error or in its causes, or null, and if there is one.
// errors.typed sets the type of an error to the descriptor of its type, and its value to the whole error
static int error_as(Lowerer *L, AstExpr *expr, Type *to, int *found) {
    TypeStruct *error = (TypeStruct *) ((TypePointer *) expr->type)->pointer_to;
    int type_at = error_member_offset(error, "type"), value_at = error_member_offset(error, "value"), cause_at = error_member_offset(error, "cause");
    int wanted = emit_global_addr(L, type_info_global(L, ((TypePointer *) to)->pointer_to));
    int current = ir_new_slot(L->func, 8, 8, "error as");
    int result  = ir_new_slot(L->func, 8, 8, "error as result");
    emit_store(L, IR_PTR, addr_slot(current, 0), lower_scalar_as(L, expr, expr->type));
    emit_store(L, IR_PTR, addr_slot(result, 0), emit_const(L, IR_PTR, 0));
    IrBlock *loop = create_block(L, "error.as.loop");
    IrBlock *body = create_block(L, "error.as.body");
    IrBlock *hit  = create_block(L, "error.as.hit");
    IrBlock *next = create_block(L, "error.as.cause");
    IrBlock *done = create_block(L, "error.as.done");
    terminate_jmp(L, loop);
    set_block(L, loop);
    int e = emit_load(L, IR_PTR, addr_slot(current, 0));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, e, emit_const(L, IR_PTR, 0)), body, done);
    set_block(L, body);
    int type = emit_load(L, IR_PTR, addr_vreg(e, type_at));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_PTR, type, wanted), hit, next);
    set_block(L, hit);
    emit_store(L, IR_PTR, addr_slot(result, 0), emit_load(L, IR_PTR, addr_vreg(e, value_at)));
    terminate_jmp(L, done);
    set_block(L, next);
    emit_store(L, IR_PTR, addr_slot(current, 0), emit_load(L, IR_PTR, addr_vreg(e, cause_at)));
    terminate_jmp(L, loop);
    set_block(L, done);
    int value = emit_load(L, IR_PTR, addr_slot(result, 0));
    *found = emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, value, emit_const(L, IR_PTR, 0));
    return value;
}

static int compare_i64(const void *a, const void *b) {
    int64_t x = *(const int64_t *) a, y = *(const int64_t *) b;
    return (x > y) - (x < y);
}

// Throws a Panic when the number (an i64) isn't a value of the enum. A plain enum has a value of each enumerator: its values
// are in a range when they follow each other, which is one compare, or else each is compared. An enum flags has any
// combination of its flags, so the number has no other bits
static void emit_enum_value_check(Lowerer *L, int value, TypeEnum *type, Ast *at) {
    AstEnum *node = type->node;
    IrBlock *valid   = create_block(L, "enum.value");
    IrBlock *invalid = create_block(L, "enum.not.a.value");

    int      count  = node->enumerators.count;
    int64_t *values = malloc(sizeof(int64_t) * (count > 0 ? count : 1));
    int64_t  flags  = 0;
    for (int i = 0; i < count; i++) {
        AstEnumerator *e = ((AstEnumerator **) node->enumerators.items)[i];
        values[i] = e->value;
        flags    |= e->value;
    }

    if (count == 0) {
        terminate_jmp(L, invalid);
    } else if (node->is_flags) {
        int others = emit_binop(L, IR_AND, IR_I64, value, emit_const(L, IR_I64, ~flags));
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, others, emit_const(L, IR_I64, 0)), valid, invalid);
    } else {
        qsort(values, count, sizeof(int64_t), compare_i64);
        int distinct = 1;
        for (int i = 1; i < count; i++) if (values[i] != values[i - 1]) distinct++;
        int64_t low = values[0], high = values[count - 1];
        if ((int64_t) distinct == high - low + 1) {
            // e.g 0..3: value - low < 4, compared unsigned so a value below low is outside too
            int offset = emit_binop(L, IR_SUB, IR_I64, value, emit_const(L, IR_I64, low));
            terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_ULT, IR_I64, offset, emit_const(L, IR_I64, high - low + 1)), valid, invalid);
        } else {
            for (int i = 0; i < count; i++) {
                if (i > 0 && values[i] == values[i - 1]) continue;
                IrBlock *next = create_block(L, "enum.next.value");
                terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, IR_I64, value, emit_const(L, IR_I64, values[i])), valid, next);
                set_block(L, next);
            }
            terminate_jmp(L, invalid);
        }
    }
    free(values);

    set_block(L, invalid);
    int args[2] = { value, emit_c_text(L, type_to_str((Type *) type)) };
    emit_errors_call(L, "panicEnumCast", 2, args);
    emit_trace_frame(L, at->start.line);
    emit_unwind(L);
    set_block(L, valid);
}

static Value lower_cast(Lowerer *L, AstCast *cast) {
    Type *from = cast->expr->type;
    Type *to   = cast->cast_to;

    // If an interface is of a type, for a case of a switch on it
    if (cast->is_type_test) {
        int is_type;
        if (cast->is_error_cast) error_as(L, cast->expr, to, &is_type);
        else                     interface_as(L, cast->expr, to, &is_type);
        return value_scalar(is_type, cast->head.type);
    }

    // err as *QueryError: a Panic when neither the error nor its causes are one
    if (cast->is_error_cast) {
        int found;
        int value = error_as(L, cast->expr, to, &found);
        IrBlock *none = create_block(L, "error.as.none");
        IrBlock *some = create_block(L, "error.as.some");
        terminate_br(L, found, some, none);
        set_block(L, none);
        char wanted[200];
        snprintf(wanted, sizeof(wanted), "%s", type_to_str(((TypePointer *) to)->pointer_to));
        int args[2] = { lower_scalar_as(L, cast->expr, from), emit_c_text(L, wanted) };
        emit_errors_call(L, "panicErrorCast", 2, args);
        emit_trace_frame(L, cast->head.head.start.line);
        emit_unwind(L);
        set_block(L, some);
        return value_scalar(value, to);
    }

    // s as *Circle of an interface: a Panic when its value is of another type
    if (is_interface_type(from) && to->kind == TYPE_POINTER) {
        int is_type;
        int value = interface_as(L, cast->expr, to, &is_type);
        IrBlock *other = create_block(L, "interface.other.type");
        IrBlock *same  = create_block(L, "interface.same.type");
        terminate_br(L, is_type, same, other);
        set_block(L, other);
        IrAddr at = value_to_place(L, lower_expr(L, cast->expr)).addr;
        int table = emit_load(L, IR_PTR, addr_add(at, 8));
        char wanted[200];
        snprintf(wanted, sizeof(wanted), "%s", type_to_str(to));
        int args[3] = { table, emit_c_text(L, ((TypeStruct *) from)->head.name), emit_c_text(L, wanted) };
        emit_errors_call(L, "panicInterfaceCast", 3, args);
        emit_trace_frame(L, cast->head.head.start.line);
        emit_unwind(L);
        set_block(L, same);
        return value_scalar(value, to);
    }

    if (to->kind == TYPE_ANY && from->kind != TYPE_ANY) {
        return box_to_any(L, lower_expr(L, cast->expr), to, (Ast *) cast);
    }
    if (from->kind == TYPE_ANY && to->kind != TYPE_ANY) {
        return unbox_any(L, value_to_place(L, lower_expr(L, cast->expr)), to);
    }

    if (from->kind == TYPE_STRING && to->kind == TYPE_POINTER) {
        // string -> *u8 is the string's data pointer
        if (cast->expr->head.kind == AST_LITERAL && ((AstLiteral *) cast->expr)->kind == LITERAL_STRING) {
            AstLiteral *lit = (AstLiteral *) cast->expr;
            int len = 0;
            int data = string_data_global(L, lit->as.value.string.data, lit->as.value.string.length, &len);
            return value_scalar(emit_global_addr(L, data), to);
        }
        Value str = value_to_place(L, lower_expr(L, cast->expr));
        return value_scalar(emit_load(L, IR_PTR, str.addr), to);
    }

    if (type_is_aggregate(from) && type_is_aggregate(to)) {
        return convert_aggregate(L, lower_expr(L, cast->expr), to, (Ast *) cast);
    }

    if (type_is_aggregate(from) && to->kind == TYPE_POINTER) {
        Value value = value_to_place(L, lower_expr(L, cast->expr));
        if (is_fixed_array(from)) return value_scalar(addr_to_ptr(L, value.addr), to);
        if (from->kind == TYPE_ARRAY) return value_scalar(emit_load(L, IR_PTR, value.addr), to);
    }

    if (type_is_aggregate(from) || type_is_aggregate(to)) {
        lower_fail(L, (Ast *) cast, "Not implemented in the IR backend yet: cast from %s to %s", type_to_str(from), type_to_str(to));
    }

    // A number as an enum, e.g n as Color: a Panic when it isn't one of its values, so an enum always holds one of them
    if (to->kind == TYPE_ENUM && from->kind != TYPE_ENUM && bounds_checks) {
        Type *i64_type = primitive_type(PRIMITIVE_I64);
        int value = lower_scalar_as(L, cast->expr, i64_type);
        emit_enum_value_check(L, value, (TypeEnum *) to, (Ast *) cast);
        return value_scalar(convert_scalar(L, value, i64_type, to), to);
    }

    return value_scalar(lower_scalar_as(L, cast->expr, to), to);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Runtime type information, any and print
//////////////////////////////////////////////////////////////////////////////////////////

// A string global for text that is used as is (no escape sequences)
static int string_global_raw(Lowerer *L, const char *text) {
    int len = (int) strlen(text);
    IrGlobal *data = ir_global_data(L->module, "str", text, len + 1, 1, true);
    int64_t header[2] = { 0, len };
    IrGlobal *str = ir_global_data(L->module, "string", header, 16, 8, true);
    IrReloc reloc = { 0, data->index };
    ir_array_push(str->relocs, reloc);
    return str->index;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Runtime type information
//
// Every type that is converted to an 'any' gets one constant descriptor (a Type, TypePrimitive, TypeStruct, ... of runtime.sd).
// The layouts are read from the structs in runtime.sd, so they can't get out of sync with the compiler
//////////////////////////////////////////////////////////////////////////////////////////

static AstStruct *runtime_struct(Lowerer *L, const char *name) {
    AstIdentifier *ident = L->runtime_scope ? find_identifier_in_scope(L->runtime_scope, (char *) name) : NULL;
    if (ident == NULL || ident->type == NULL || ident->type->kind != TYPE_STRUCT) {
        lower_fail(L, NULL, "Compiler Error: Failed to find the struct '%s' of runtime.sd", name);
    }
    return ((TypeStruct *) ident->type)->node;
}

static int runtime_size(Lowerer *L, const char *struct_name) {
    return runtime_struct(L, struct_name)->identifier->type->size;
}

static int runtime_offset(Lowerer *L, const char *struct_name, const char *member_name) {
    AstIdentifier *member = get_struct_member(runtime_struct(L, struct_name), (char *) member_name);
    if (member == NULL) lower_fail(L, NULL, "Compiler Error: The struct '%s' of runtime.sd has no member '%s'", struct_name, member_name);
    return member->member_offset;
}

static void write_pointer_field(IrGlobal *global, int offset, int target) {
    IrReloc reloc = { offset, target };
    ir_array_push(global->relocs, reloc);
}

// A string { data, len } in a global
static void write_string_field(Lowerer *L, IrGlobal *global, int offset, const char *text) {
    int len = (int) strlen(text);
    IrGlobal *data = ir_global_data(L->module, "str", text, len + 1, 1, true);
    write_pointer_field(global, offset, data->index);
    write_int_bytes(&global->data[offset + 8], len, 8);
}

// A slice { data, count } in a global
static void write_slice_field(IrGlobal *global, int offset, int data, int64_t count) {
    if (count > 0) write_pointer_field(global, offset, data);
    write_int_bytes(&global->data[offset + 8], count, 8);
}

static int type_info_global(Lowerer *L, Type *type);

// The value of an attribute e.g @json.Field(name: "id"): the default values of its struct, and then its arguments
static int attribute_value_global(Lowerer *L, AstStructLiteral *value) {
    Type *type = value->head.type;
    IrGlobal *global = ir_global_data(L->module, "attribute", NULL, type->size > 0 ? type->size : 1, type_alignment(type), true);

    // write_constant writes the given members, and the default of each member that isn't given. Not both, as a pointer, e.g
    // of a string, is a relocation, and the first one written at an offset would win
    write_constant(L, global, 0, (AstExpr *) value, type);
    return global->index;
}

static int function_global(Lowerer *L, AstFunctionDefn *func_defn);
static int thunk_global(Lowerer *L, AstFunctionDefn *target, int *argument_offsets, int *result_offsets);

// Lays out values of the types like the members of a struct: their offsets, and the size of the block
static int layout_values(Type **types, int count, int *offsets) {
    int size = 0;
    for (int i = 0; i < count; i++) {
        int align = type_alignment(types[i]);
        if (align < 1) align = 1;
        size = (size + align - 1) / align * align;
        offsets[i] = size;
        size += type_storage_size(types[i]);
    }
    return (size + 7) / 8 * 8;
}

// The parameters, results, sizes and invoke() of an AttributedFunction
static void write_function_values(Lowerer *L, IrGlobal *array, int at, AstFunctionDefn *func) {
    int param_count  = func->parameters.count;
    int result_count = func->lowered_return_params.count;   // Not a void result
    Type **param_types  = calloc(param_count + 1, sizeof(Type *));
    Type **result_types = calloc(result_count + 1, sizeof(Type *));
    int *argument_offsets = calloc(param_count + 1, sizeof(int));
    int *result_offsets   = calloc(result_count + 1, sizeof(int));
    for (int i = 0; i < param_count; i++)  param_types[i]  = ((AstIdentifier *)da_get_deref(func->parameters, i))->type;
    for (int i = 0; i < result_count; i++) result_types[i] = da_get_deref(func->return_types, i);
    int arguments_size = layout_values(param_types, param_count, argument_offsets);
    int results_size   = layout_values(result_types, result_count, result_offsets);

    int value_size = runtime_size(L, "FunctionValue");
    for (int kind = 0; kind < 2; kind++) {
        int count = kind == 0 ? param_count : result_count;
        if (count == 0) continue;
        IrGlobal *values = ir_global_data(L->module, kind == 0 ? "parameters" : "results", NULL, count * value_size, 8, true);
        for (int i = 0; i < count; i++) {
            int v = i * value_size;
            const char *name = kind == 0 ? ((AstIdentifier *)da_get_deref(func->parameters, i))->name : "";
            write_string_field(L, values, v + runtime_offset(L, "FunctionValue", "name"), name);
            write_pointer_field(values, v + runtime_offset(L, "FunctionValue", "type"), type_info_global(L, kind == 0 ? param_types[i] : result_types[i]));
            write_int_bytes(&values->data[v + runtime_offset(L, "FunctionValue", "offset")], kind == 0 ? argument_offsets[i] : result_offsets[i], 8);

            // The default of a parameter, written in the data when it is a constant
            AstExpr *fallback = kind == 0 ? ((AstIdentifier *)da_get_deref(func->parameters, i))->value : NULL;
            if (fallback) {
                values->data[v + runtime_offset(L, "FunctionValue", "hasDefault")] = 1;
                if (is_constant_value(fallback)) {
                    Type *type = param_types[i];
                    IrGlobal *value = ir_global_data(L->module, "default_value", NULL, type->size > 0 ? type->size : 1, type_alignment(type), true);
                    write_constant(L, value, 0, fallback, type);
                    write_pointer_field(values, v + runtime_offset(L, "FunctionValue", "defaultValue"), value->index);
                }
            }
        }
        write_slice_field(array, at + runtime_offset(L, "AttributedFunction", kind == 0 ? "parameters" : "results"), values->index, count);
    }
    write_int_bytes(&array->data[at + runtime_offset(L, "AttributedFunction", "argumentsSize")], arguments_size, 8);
    write_int_bytes(&array->data[at + runtime_offset(L, "AttributedFunction", "resultsSize")], results_size, 8);
    write_pointer_field(array, at + runtime_offset(L, "AttributedFunction", "invoke"), thunk_global(L, func, argument_offsets, result_offsets));
    free(param_types);
    free(result_types);
}

// The slice of attributedFunctionList: an AttributedFunction for each function with attribute values. The functions are
// compiled, as the program calls them through the list
static void write_attributed_functions(Lowerer *L, IrGlobal *list) {
    DynamicArray functions = L->parser->attributed_functions;
    int entry_size = runtime_size(L, "AttributedFunction");
    int count = 0;
    for (int i = 0; i < functions.count; i++) {
        AstIdentifier *ident = da_get_deref(functions, i);
        if (ident->type && ident->type->kind == TYPE_FUNCTION && !(ident->flags & IDENTIFIER_IS_POISONED)) count++;
    }
    if (count == 0) return;

    IrGlobal *array = ir_global_data(L->module, "attributed_functions", NULL, count * entry_size, 8, true);
    int entry = 0;
    for (int i = 0; i < functions.count; i++) {
        AstIdentifier *ident = da_get_deref(functions, i);
        if (!ident->type || ident->type->kind != TYPE_FUNCTION || (ident->flags & IDENTIFIER_IS_POISONED)) continue;
        AstFunctionDefn *func = ((TypeFunction *)ident->type)->node;

        int at = entry * entry_size;
        write_string_field(L, array, at + runtime_offset(L, "AttributedFunction", "name"), ident->name);
        AstFile *file = ident->head.file;
        write_string_field(L, array, at + runtime_offset(L, "AttributedFunction", "namespaceName"), file && file->ns ? file->ns->name : "");
        write_pointer_field(array, at + runtime_offset(L, "AttributedFunction", "function"), function_global(L, func));

        write_function_values(L, array, at, func);

        int attribute_count = ident->attributes.count;
        IrGlobal *attributes = ir_global_data(L->module, "attributes", NULL, attribute_count * 16, 8, true);
        for (int j = 0; j < attribute_count; j++) {
            AstStructLiteral *value = da_get_deref(ident->attributes, j);
            write_pointer_field(attributes, j * 16,     attribute_value_global(L, value));
            write_pointer_field(attributes, j * 16 + 8, type_info_global(L, value->head.type));
        }
        write_slice_field(array, at + runtime_offset(L, "AttributedFunction", "attributes"), attributes->index, attribute_count);
        entry++;
    }
    write_slice_field(list, 0, array->index, count);
}

static int struct_members_global(Lowerer *L, TypeStruct *struct_type, int *out_count) {
    DynamicArray members = struct_type->node->members->identifiers;
    int entry_size = runtime_size(L, "StructMember");

    int count = 0;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = da_get_deref(members, i);
        if (!(member->flags & IDENTIFIER_IS_CONSTANT)) count++;
    }
    *out_count = count;
    if (count == 0) return -1;

    IrGlobal *array = ir_global_data(L->module, "struct_members", NULL, count * entry_size, 8, true);
    int entry = 0;
    for (int i = 0; i < members.count; i++) {
        AstIdentifier *member = da_get_deref(members, i);
        if (member->flags & IDENTIFIER_IS_CONSTANT) continue;

        int at = entry * entry_size;
        write_string_field(L, array, at + runtime_offset(L, "StructMember", "name"), member->name);
        write_pointer_field(array, at + runtime_offset(L, "StructMember", "type"), type_info_global(L, member->type));
        write_int_bytes(&array->data[at + runtime_offset(L, "StructMember", "offset")], member->member_offset, 8);
        write_int_bytes(&array->data[at + runtime_offset(L, "StructMember", "index")], member->member_index, 8);

        int attribute_count = member->attributes.count;
        if (attribute_count > 0) {
            // An array of 'any' { data, type }
            IrGlobal *attributes = ir_global_data(L->module, "attributes", NULL, attribute_count * 16, 8, true);
            for (int j = 0; j < attribute_count; j++) {
                AstStructLiteral *value = da_get_deref(member->attributes, j);
                write_pointer_field(attributes, j * 16,     attribute_value_global(L, value));
                write_pointer_field(attributes, j * 16 + 8, type_info_global(L, value->head.type));
            }
            write_slice_field(array, at + runtime_offset(L, "StructMember", "attributes"), attributes->index, attribute_count);
        }
        entry++;
    }
    return array->index;
}

static int enum_values_global(Lowerer *L, TypeEnum *enum_type, int *out_count) {
    DynamicArray enumerators = enum_type->node->enumerators;
    int entry_size = runtime_size(L, "EnumValue");
    *out_count = enumerators.count;
    if (enumerators.count == 0) return -1;

    IrGlobal *array = ir_global_data(L->module, "enum_values", NULL, enumerators.count * entry_size, 8, true);
    for (int i = 0; i < enumerators.count; i++) {
        AstEnumerator *etor = da_get_deref(enumerators, i);
        int at = i * entry_size;
        write_string_field(L, array, at + runtime_offset(L, "EnumValue", "name"), etor->name);
        write_int_bytes(&array->data[at + runtime_offset(L, "EnumValue", "value")], etor->value, 8);
    }
    return array->index;
}

// The global holding the descriptor of the type. Each type gets one, so descriptors can be compared by address
static int type_info_global(Lowerer *L, Type *type) {
    if (type_is_untyped_int(type))   type = primitive_type(PRIMITIVE_INT);
    if (type_is_untyped_float(type)) type = primitive_type(PRIMITIVE_FLOAT);

    char *key = type_identity(type); // Tells apart types with the same name, e.g a struct P of two functions
    for (int i = 0; i < L->type_infos.count; i++) {
        if (strcmp(L->type_infos.items[i].key, key) == 0) return L->type_infos.items[i].global;
    }

    const char *layout = "Type";
    switch (type->kind) {
    case TYPE_VOID: case TYPE_BOOL: case TYPE_INTEGER: case TYPE_FLOAT: layout = "TypePrimitive"; break;
    case TYPE_POINTER: layout = "TypePointer"; break;
    case TYPE_ARRAY:   layout = "TypeArray";   break;
    case TYPE_ENUM:    layout = "TypeEnum";    break;
    case TYPE_STRUCT:  layout = "TypeStruct";  break;
    case TYPE_ANY:     layout = "TypeAny";     break;
    default: break;
    }

    IrGlobal *global = ir_global_data(L->module, "type_info", NULL, runtime_size(L, layout), 8, true);

    // Cached before the parts are made, so that a type that refers to itself e.g Node :: struct { next: *Node; } ends
    TypeInfoEntry entry = { key, global->index };
    ir_array_push(L->type_infos, entry);

    // The head, a Type :: struct { name: string; kind: TypeKind; size: int; flags: int; }
    write_string_field(L, global, runtime_offset(L, "Type", "name"), type_to_str(type)); // As it is written, e.g P of a local P
    write_int_bytes(&global->data[runtime_offset(L, "Type", "kind")], type->kind - TYPE_VOID, 4);
    write_int_bytes(&global->data[runtime_offset(L, "Type", "size")], type->size, 4);

    switch (type->kind) {
    case TYPE_VOID: case TYPE_BOOL: case TYPE_INTEGER: case TYPE_FLOAT: {
        write_int_bytes(&global->data[runtime_offset(L, "TypePrimitive", "kind")], ((TypePrimitive *) type)->kind - 1, 4);
        break;
    }
    case TYPE_POINTER: {
        Type *pointee = ((TypePointer *) type)->pointer_to;
        write_pointer_field(global, runtime_offset(L, "TypePointer", "pointerTo"), type_info_global(L, pointee));
        break;
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *) type;
        write_int_bytes(&global->data[runtime_offset(L, "TypeArray", "kind")], array->array_kind, 4);
        write_pointer_field(global, runtime_offset(L, "TypeArray", "elemType"), type_info_global(L, array->elem_type));
        int64_t count = array->array_kind == ARRAY_FIXED ? fixed_array_count(array) : 0;
        write_int_bytes(&global->data[runtime_offset(L, "TypeArray", "count")], count, 8);
        break;
    }
    case TYPE_ENUM: {
        TypeEnum *enum_type = (TypeEnum *) type;
        write_pointer_field(global, runtime_offset(L, "TypeEnum", "backingType"), type_info_global(L, enum_type->backing_type));
        if (enum_type->node && enum_type->node->is_flags) {
            write_int_bytes(&global->data[runtime_offset(L, "Type", "flags")], 4, 4); // TypeFlags.isFlags in runtime.sd
        }
        int count = 0;
        int values = enum_values_global(L, enum_type, &count);
        write_slice_field(global, runtime_offset(L, "TypeEnum", "values"), values, count);
        break;
    }
    case TYPE_STRUCT: {
        TypeStruct *struct_type = (TypeStruct *) type;
        int count = 0;
        int members = struct_members_global(L, struct_type, &count);
        write_slice_field(global, runtime_offset(L, "TypeStruct", "members"), members, count);
        write_int_bytes(&global->data[runtime_offset(L, "TypeStruct", "alignment")], struct_type->alignment, 4);
        if (struct_type->node->is_union) {
            write_int_bytes(&global->data[runtime_offset(L, "Type", "flags")], 1, 4); // TypeFlags.isUnion in runtime.sd
        }
        if (is_optional_type(type)) {
            write_int_bytes(&global->data[runtime_offset(L, "Type", "flags")], 2, 4); // TypeFlags.isOptional in runtime.sd
        }
        break;
    }
    default:
        break;
    }

    return global->index;
}

// Boxes a value into an 'any': { data: *void (pointer to a copy of the value), type: *Type }
static Value box_to_any(Lowerer *L, Value value, Type *any_type, Ast *site) {
    Type *from = value.type;
    Value boxed;

    // An interface is its value, of the type of its value, e.g for print. A null one is a null pointer
    if (is_interface_type(from)) {
        IrAddr at = value_to_place(L, value).addr;
        int table = emit_load(L, IR_PTR, addr_add(at, 8));
        Value any = temp_place(L, any_type, "any");
        Value null_pointer = temp_place(L, primitive_type(PRIMITIVE_U64), "null");
        emit_store(L, IR_PTR, null_pointer.addr, emit_const(L, IR_PTR, 0));
        IrBlock *has_value = create_block(L, "any.of.interface");
        IrBlock *no_value  = create_block(L, "any.of.null.interface");
        IrBlock *done      = create_block(L, "any.made");
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, table, emit_const(L, IR_PTR, 0)), has_value, no_value);
        set_block(L, has_value);
        emit_store(L, IR_PTR, any.addr, emit_load(L, IR_PTR, at));
        emit_store(L, IR_PTR, addr_add(any.addr, 8), emit_load(L, IR_PTR, addr_vreg(table, 0)));
        terminate_jmp(L, done);
        set_block(L, no_value);
        Type *void_pointer = (Type *) type_defn_null_ptr;
        emit_store(L, IR_PTR, any.addr, addr_to_ptr(L, null_pointer.addr));
        emit_store(L, IR_PTR, addr_add(any.addr, 8), emit_global_addr(L, type_info_global(L, void_pointer)));
        terminate_jmp(L, done);
        set_block(L, done);
        return any;
    }

    boxed = temp_place(L, from, "boxed value");
    store_value(L, boxed.addr, from, value, site);

    int data = addr_to_ptr(L, boxed.addr);
    int type = emit_global_addr(L, type_info_global(L, from));

    Value any = temp_place(L, any_type, "any");
    emit_store(L, IR_PTR, any.addr, data);
    emit_store(L, IR_PTR, addr_add(any.addr, 8), type);
    return any;
}

// Reads the value out of an 'any' as type 'to'
static Value unbox_any(Lowerer *L, Value any, Type *to) {
    any = value_to_place(L, any);
    int data = emit_load(L, IR_PTR, any.addr);
    return value_place(addr_vreg(data, 0), to);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Calls
//////////////////////////////////////////////////////////////////////////////////////////

static bool call_is_direct(AstFunctionCall *call) {
    if (call->is_lambda_call) return false;
    if (call->is_method_call) return true;

    AstExpr *expr = call->expression;
    if (expr->head.kind == AST_FUNCTION_DEFN) return true;

    AstIdentifier *ident = NULL;
    if (expr->head.kind == AST_LITERAL && ((AstLiteral *) expr)->kind == LITERAL_IDENTIFIER) {
        ident = ((AstLiteral *) expr)->as.value.identifier.resolved_identifier;
    } else if (expr->head.kind == AST_MEMBER_ACCESS && ((AstMemberAccess *) expr)->access_kind == MEMBER_ACCESS_STATIC_STRUCT_MEMBER) {
        ident = ((AstMemberAccess *) expr)->struct_member;
    }
    if (ident == NULL) return false;
    ident = resolve_override(ident);

    if (ident->flags & (IDENTIFIER_IS_CONSTANT_FUNCTION_DEFN | IDENTIFIER_IS_NAME_OF_FUNCTION)) return true;
    if ((ident->flags & IDENTIFIER_IS_CONSTANT) && ident->value && ident->value->head.kind == AST_FUNCTION_DEFN) return true;
    return false;
}

// Lowers one argument according to the calling convention
static int lower_argument(Lowerer *L, AstExpr *arg, Type *param_type, bool is_c_vararg) {
    Type *type = is_c_vararg ? arg->type : param_type;

    if (!type_is_aggregate(type)) {
        int value = lower_scalar_as(L, arg, type);
        if (is_c_vararg) {
            // C default argument promotions
            IrType ir_type = ir_vreg_type(L->func, value);
            if (ir_type == IR_F32) {
                value = emit_unop(L, IR_FPEXT, IR_F64, value);
            } else if (ir_type == IR_I8 || ir_type == IR_I16) {
                value = emit_unop(L, type_is_signed(type) ? IR_SEXT : IR_ZEXT, IR_I32, value);
            }
        }
        return value;
    }

    Value value = convert_aggregate(L, lower_expr(L, arg), type, (Ast *) arg);
    value = value_to_place(L, value);

    switch (classify_argument(type)) {
    case ARG_AGGREGATE_BY_VALUE:
        return emit_load(L, int_ir_type_of_size(type->size), value.addr);
    case ARG_AGGREGATE_BY_REFERENCE:
        if (!value.is_temporary) {
            // The callee owns the memory it is given, so pass a copy
            Value copy = temp_place(L, type, "argument copy");
            copy_aggregate(L, copy.addr, value, type->size);
            value = copy;
        }
        return addr_to_ptr(L, value.addr);
    default:
        assert(false);
        return IR_NO_VREG;
    }
}

static Value lower_call(Lowerer *L, AstFunctionCall *call, int *ret_slots, int max_ret_slots) {
    AstFunctionDefn *func_defn = call->func_defn;
    if (func_defn == NULL) lower_fail(L, (Ast *) call, "Compiler Error: Call without a resolved function");

    bool is_c = func_defn->is_extern || func_defn->calling_convention == CALLING_CONV_MSVC;

    // Callee (evaluated before the arguments)
    IrCalleeKind callee_kind;
    int callee;
    int interface_value = IR_NO_VREG;   // The value of the interface a method of an interface is called on: its self
    if (func_defn->is_interface_method) {
        // Through the table of the interface value: its receiver is a pointer to the interface value
        AstExpr *receiver = da_get_deref(call->lowered_arguments, 0);
        int at = lower_scalar_as(L, receiver, receiver->type);
        Type *interface = receiver->type->kind == TYPE_POINTER ? ((TypePointer *) receiver->type)->pointer_to : receiver->type;
        if (!is_interface_type(interface)) lower_fail(L, (Ast *) call, "Compiler Error: A method of an interface called on a %s", type_to_str(receiver->type));
        interface_value = emit_load(L, IR_PTR, addr_vreg(at, 0));
        AstExpr *named = receiver;
        if (named->head.kind == AST_UNARY && ((AstUnary *) named)->operator == OP_ADDRESS_OF) named = ((AstUnary *) named)->expr;
        emit_null_check(L, interface_value, named);   // A null interface has no table either
        int table = emit_load(L, IR_PTR, addr_vreg(at, 8));
        DynamicArray methods = interface_methods((TypeStruct *) interface);
        int index = -1;
        for (int i = 0; i < methods.count; i++) {
            if (strcmp(((AstFunctionDefn **) methods.items)[i]->identifier->name, func_defn->identifier->name) == 0) index = i;
        }
        da_free(&methods);
        if (index < 0) lower_fail(L, (Ast *) call, "Compiler Error: '%s' isn't a method of %s", func_defn->identifier->name, type_to_str(interface));
        callee_kind = IR_CALLEE_VREG;
        callee = emit_load(L, IR_PTR, addr_vreg(table, 8 * (1 + index)));
    } else if (call_is_direct(call)) {
        callee_kind = IR_CALLEE_DIRECT;
        callee = function_global(L, func_defn);
    } else {
        callee_kind = IR_CALLEE_VREG;
        callee = value_to_scalar(L, lower_expr(L, call->expression));
    }

    int ret_count = func_defn->lowered_return_params.count;
    int arg_count = call->lowered_arguments.count;

    // Return type for C functions returning in a register
    Type  *c_return_type = func_defn->return_types.count > 0 ? da_get_deref(func_defn->return_types, 0) : NULL;
    IrType ir_return_type = is_c ? IR_VOID : IR_I32; // A Saad function says in eax if an error was thrown out of it
    if (is_c && ret_count == 0 && c_return_type && c_return_type->kind != TYPE_VOID) {
        if (type_is_aggregate(c_return_type)) {
            ir_return_type = int_ir_type_of_size(c_return_type->size);
            if (ir_return_type == IR_VOID) LOWER_NOT_IMPLEMENTED(L, call, "C functions returning a struct of this size");
        } else {
            ir_return_type = ir_type_of(c_return_type);
        }
    }

    IrCall *ir_call = new_call(L, callee_kind, callee, ir_return_type, ret_count + arg_count);

    // Hidden out-pointers for the return values
    int local_ret_slots[16];
    if (ret_count > 16) LOWER_NOT_IMPLEMENTED(L, call, "more than 16 return values");
    for (int i = 0; i < ret_count; i++) {
        AstIdentifier *ret_param = da_get_deref(func_defn->lowered_return_params, i);
        Type *ret_type = ((TypePointer *) ret_param->type)->pointer_to;
        local_ret_slots[i] = new_temp_slot(L, ret_type, "return value");
        call_add_arg(ir_call, emit_slot_addr(L, local_ret_slots[i]), false);
        if (ret_slots && i < max_ret_slots) ret_slots[i] = local_ret_slots[i];
    }

    // User arguments
    for (int i = 0; i < arg_count; i++) {
        AstExpr *arg = da_get_deref(call->lowered_arguments, i);
        bool is_vararg = (arg->head.flags & AST_FLAG_IS_C_VARARG) != 0;

        Type *param_type = arg->type;
        int param_index = ret_count + i;
        if (!is_vararg && param_index < func_defn->lowered_params.count) {
            AstIdentifier *param = da_get_deref(func_defn->lowered_params, param_index);
            param_type = resolve_override(param)->type;
        }
        if (param_type->kind == TYPE_VARIADIC) param_type = (Type *) ((TypeVariadic *) param_type)->slice;

        int value = (i == 0 && interface_value != IR_NO_VREG) ? interface_value : lower_argument(L, arg, param_type, is_vararg);
        // The receiver of a method, when it is a pointer, so that the method doesn't check self. &x, e.g of x.m() of a
        // struct x, isn't null
        bool is_receiver = func_defn->is_method && i == 0 && param_type->kind == TYPE_POINTER;
        if (is_receiver && interface_value == IR_NO_VREG && !(arg->head.kind == AST_UNARY && ((AstUnary *) arg)->operator == OP_ADDRESS_OF)) {
            emit_null_check(L, value, arg);
        }
        call_add_arg(ir_call, value, is_vararg);
    }

    int result = emit_call(L, ir_call);

    // An error that the callee threw goes on, and so does the error result that isn't taken, e.g of 'x := f();'
    if (!is_c) {
        if (call->head.head.start.line > 0 && (!L->ast_func || call->head.head.file == L->ast_func->head.head.file)) L->line = call->head.head.start.line;
        // A function value can be a function of C, whose eax isn't the flag, so thrownError says it then
        emit_throw_check(L, result, callee_kind == IR_CALLEE_VREG);
        if ((call->head.head.flags & AST_FLAG_THROWS_ON_ERROR) && ret_count > 0) {
            AstIdentifier *err_param = da_get_deref(func_defn->lowered_return_params, ret_count - 1);
            (void) err_param;
            emit_throw(L, emit_load(L, IR_PTR, addr_slot(local_ret_slots[ret_count - 1], 0)));
        }
    }

    // Result
    if (ret_count > 0) {
        AstIdentifier *ret_param = da_get_deref(func_defn->lowered_return_params, 0);
        Type *ret_type = ((TypePointer *) ret_param->type)->pointer_to;
        Value v = value_place(addr_slot(local_ret_slots[0], 0), ret_type);
        v.is_temporary = true;
        return v;
    }
    if (ir_return_type == IR_VOID) return value_none();
    if (type_is_aggregate(c_return_type)) {
        Value place = temp_place(L, c_return_type, "returned struct");
        emit_store(L, ir_return_type, place.addr, result);
        return place;
    }
    return value_scalar(result, c_return_type);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Expressions
//////////////////////////////////////////////////////////////////////////////////////////

static Value lower_expr(Lowerer *L, AstExpr *expr) {
    if (expr->replacement) return lower_expr(L, expr->replacement); // e.g xs[i] on a collection means *xs.indexRef(i)

    switch (expr->head.kind) {
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *) expr;
        if (lit->kind == LITERAL_IDENTIFIER) return lower_identifier(L, lit);
        return lower_literal(L, lit);
    }
    case AST_BINARY:
        return lower_binary(L, (AstBinary *) expr);
    case AST_UNARY:
        return lower_unary(L, (AstUnary *) expr);
    case AST_CAST:
        return lower_cast(L, (AstCast *) expr);
    case AST_FUNCTION_CALL:
        return lower_call(L, (AstFunctionCall *) expr, NULL, 0);
    case AST_FUNCTION_DEFN:
        return function_address(L, (AstFunctionDefn *) expr, expr->type);
    case AST_MEMBER_ACCESS:
        return lower_member_access(L, (AstMemberAccess *) expr);
    case AST_OPTIONAL_CHAIN:
        return lower_optional_chain(L, (AstOptionalChain *) expr);
    case AST_OPTIONAL_VALUE: {
        AstOptionalValue *value = (AstOptionalValue *) expr;
        if (value->lowered == NULL) lower_fail(L, (Ast *) expr, "Compiler Error: The value of an optional is used outside its ?.");
        return *(Value *) value->lowered;
    }
    case AST_ARRAY_ACCESS:
        return value_place(array_access_addr(L, (AstArrayAccess *) expr), expr->type);
    case AST_ENUM_LITERAL: {
        AstEnumLiteral *lit = (AstEnumLiteral *) expr;
        return value_scalar(emit_const(L, ir_type_of(expr->type), lit->enum_member->value), expr->type);
    }
    case AST_STRUCT_LITERAL: {
        Value place = temp_place(L, expr->type, "struct literal");
        lower_struct_literal_into(L, (AstStructLiteral *) expr, place.addr);
        return place;
    }
    case AST_ARRAY_LITERAL: {
        AstArrayLiteral *lit = (AstArrayLiteral *) expr;
        if (lit->collection_decl) {
            // A collection literal is built in its hidden local by calling add() or set() for each element
            lower_declaration(L, lit->collection_decl);
            for (int i = 0; i < lit->collection_calls.count; i++) {
                lower_call(L, da_get_deref(lit->collection_calls, i), NULL, 0);
            }
            return lower_expr(L, lit->collection_value);
        }

        Value place = temp_place(L, expr->type, "array literal");
        lower_array_literal_into(L, lit, place.addr);
        return place;
    }
    case AST_SIZEOF: {
        AstSizeof *ast_sizeof = (AstSizeof *) expr;
        return value_scalar(emit_const(L, ir_type_of(expr->type), ast_sizeof->type->size), expr->type);
    }
    case AST_SEMICOLON_EXPR:
        return value_none();
    case AST_DEFAULT_VALUE: {
        // The value a variable of the type starts with (emit_default_init): zero, or the defaults of members and enums
        Type *type = expr->type;
        Value place = temp_place(L, type, "default");
        emit_default_init(L, place.addr, type);
        if (type_is_aggregate(type)) return place;
        return value_scalar(emit_load(L, ir_type_of(type), place.addr), type);
    }
    case AST_SWITCH: {
        // A switch expression: the cases store their value in a slot, which is the value of the switch
        AstSwitch *sw = (AstSwitch *) expr;
        Type *type = sw->result_type;
        int size = type_storage_size(type);
        int slot = ir_new_slot(L->func, size > 0 ? size : 1, type_alignment(type), "switch value");
        emit_zero(L, addr_slot(slot, 0), size);
        sw->result_slot = slot;
        lower_block(L, sw->lowered);
        return value_place(addr_slot(slot, 0), type);
    }
    case AST_TYPEOF: {
        // typeof gives the name of the static type as a string
        char *name = type_to_str(((AstTypeof *) expr)->expr->type);
        return value_place(addr_global(string_global_raw(L, name), 0), expr->type);
    }
    default:
        break;
    }

    lower_fail(L, (Ast *) expr, "Not implemented in the IR backend yet: expression '%s'", ast_to_str((Ast *) expr));
    return value_none();
}

static Value lower_place(Lowerer *L, AstExpr *expr) {
    Value value = lower_expr(L, expr);
    if (value.kind == VALUE_PLACE) return value;
    if (value.kind == VALUE_NONE) lower_fail(L, (Ast *) expr, "Expression has no value");
    // An rvalue (e.g a function call result) used where memory is needed
    return value_to_place(L, value);
}

static int lower_scalar_as(Lowerer *L, AstExpr *expr, Type *target) {
    IrType target_ir = ir_type_of(target);
    if (target_ir == IR_VOID) {
        lower_fail(L, (Ast *) expr, "Compiler Error: Expected a scalar type, got %s", type_to_str(target));
    }

    // Numeric literals are materialized directly in the wanted type
    if (expr->head.kind == AST_LITERAL && target->kind != TYPE_BOOL) {
        AstLiteral *lit = (AstLiteral *) expr;
        if (lit->kind == LITERAL_INTEGER) {
            if (ir_type_is_float(target_ir)) return emit_fconst(L, target_ir, (double) lit->as.value.integer);
            if (ir_type_is_int(target_ir))   return emit_const(L, target_ir, lit->as.value.integer);
        }
        if (lit->kind == LITERAL_FLOAT && ir_type_is_float(target_ir)) return emit_fconst(L, target_ir, lit->as.value.floating);
        if (lit->kind == LITERAL_NULL && target_ir == IR_PTR) return emit_const(L, IR_PTR, 0);
    }

    Value value = lower_expr(L, expr);
    if (value.kind == VALUE_NONE) lower_fail(L, (Ast *) expr, "Expression has no value");
    if (type_is_aggregate(value.type)) {
        lower_fail(L, (Ast *) expr, "Compiler Error: Expected a scalar value, got %s", type_to_str(value.type));
    }
    return convert_scalar(L, value_to_scalar(L, value), value.type, target);
}

static void lower_condition(Lowerer *L, AstExpr *expr, IrBlock *on_true, IrBlock *on_false) {
    if (expr->head.kind == AST_BINARY) {
        AstBinary *bin = (AstBinary *) expr;
        if (bin->operator == TOKEN_LOGICAL_AND) {
            IrBlock *rhs = create_block(L, "and.rhs");
            lower_condition(L, bin->left, rhs, on_false);
            set_block(L, rhs);
            lower_condition(L, bin->right, on_true, on_false);
            return;
        }
        if (bin->operator == TOKEN_LOGICAL_OR) {
            IrBlock *rhs = create_block(L, "or.rhs");
            lower_condition(L, bin->left, on_true, rhs);
            set_block(L, rhs);
            lower_condition(L, bin->right, on_true, on_false);
            return;
        }
    }
    if (expr->head.kind == AST_UNARY && ((AstUnary *) expr)->operator == OP_NOT && ((AstUnary *) expr)->expr->type->kind == TYPE_BOOL) {
        lower_condition(L, ((AstUnary *) expr)->expr, on_false, on_true);
        return;
    }

    Value value = lower_expr(L, expr);
    int cond = emit_to_bool(L, value_to_scalar(L, value), value.type);
    terminate_br(L, cond, on_true, on_false);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Statements
//////////////////////////////////////////////////////////////////////////////////////////

static void lower_declaration(Lowerer *L, AstDeclaration *decl) {
    if (decl->flags & DECLARATION_CONSTANT) {
        return; // Constants are materialized where they are used
    }

    // `c, ok := s as *Circle` of an interface: the pointer, or null, and if it is of that type
    if (decl->values.count == 1 && decl->idents.count == 2 && ((AstExpr *) da_get_deref(decl->values, 0))->head.kind == AST_CAST &&
        ((AstCast *) da_get_deref(decl->values, 0))->with_ok) {
        AstCast *cast = da_get_deref(decl->values, 0);
        int is_type;
        int value = cast->is_error_cast ? error_as(L, cast->expr, cast->cast_to, &is_type) : interface_as(L, cast->expr, cast->cast_to, &is_type);
        int pointer = ir_new_slot(L->func, 8, 8, "as");
        emit_store(L, IR_PTR, addr_slot(pointer, 0), emit_const(L, IR_PTR, 0));
        IrBlock *same = create_block(L, "as.same");
        IrBlock *done = create_block(L, "as.done");
        terminate_br(L, is_type, same, done);
        set_block(L, same);
        emit_store(L, IR_PTR, addr_slot(pointer, 0), value);
        terminate_jmp(L, done);
        set_block(L, done);
        AstIdentifier *first = da_get_deref(decl->idents, 0), *second = da_get_deref(decl->idents, 1);
        emit_store(L, IR_PTR, addr_slot(declare_local(L, first), 0), emit_load(L, IR_PTR, addr_slot(pointer, 0)));
        emit_store(L, IR_I8, addr_slot(declare_local(L, second), 0), is_type);
        return;
    }

    // `a, b := foo()` with a function returning multiple values
    if (decl->values.count == 1 && decl->idents.count > 1) {
        AstExpr *value = da_get_deref(decl->values, 0);
        if (value->type->kind != TYPE_TUPLE || value->head.kind != AST_FUNCTION_CALL) {
            LOWER_NOT_IMPLEMENTED(L, decl, "this kind of multi-value declaration");
        }
        TypeTuple *tuple = (TypeTuple *) value->type;
        int ret_slots[16];
        lower_call(L, (AstFunctionCall *) value, ret_slots, 16);
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = da_get_deref(decl->idents, i);
            Type *ret_type = da_get_deref(tuple->types, i);
            int slot = declare_local(L, ident);
            store_value(L, addr_slot(slot, 0), resolve_override(ident)->type, value_place(addr_slot(ret_slots[i], 0), ret_type), (Ast *) decl);
        }
        return;
    }

    for (int i = 0; i < decl->idents.count; i++) {
        AstIdentifier *ident = da_get_deref(decl->idents, i);
        int slot = declare_local(L, ident);
        Type *type = resolve_override(ident)->type;

        if (i < decl->values.count) {
            AstExpr *value = da_get_deref(decl->values, i);
            store_expr(L, addr_slot(slot, 0), type, value, true);
        } else {
            emit_default_init(L, addr_slot(slot, 0), type);
        }
    }
}

static void lower_assignment(Lowerer *L, AstAssignment *assign) {
    Type *type = assign->lhs->type;
    Value lhs = lower_place(L, assign->lhs);

    // a += b with an operator + of the program: a = operator +(a, b), whose first operand is the value at the place
    if (assign->operator_call) {
        assign->operator_place->lowered = &lhs;
        store_expr(L, lhs.addr, type, assign->operator_call, false);
        assign->operator_place->lowered = NULL;
        return;
    }

    if (assign->op == ASSIGN_EQUAL) {
        // Not 'fresh': the right hand side may read the destination (e.g `a = A{x = a.y}`)
        store_expr(L, lhs.addr, type, assign->expr, false);
        return;
    }

    int op = 0;
    switch (assign->op) {
    case ASSIGN_PLUS_EQUAL:   op = '+'; break;
    case ASSIGN_MINUS_EQUAL:  op = '-'; break;
    case ASSIGN_TIMES_EQUAL:  op = '*'; break;
    case ASSIGN_DIVIDE_EQUAL: op = '/'; break;
    case ASSIGN_MODULO_EQUAL: op = '%'; break;
    case ASSIGN_XOR_EQUAL:    op = '^'; break;
    case ASSIGN_OR_EQUAL:     op = '|'; break;
    case ASSIGN_AND_EQUAL:    op = '&'; break;
    case ASSIGN_SHIFT_LEFT_EQUAL:  op = TOKEN_BITWISE_SHIFT_LEFT;  break;
    case ASSIGN_SHIFT_RIGHT_EQUAL: op = TOKEN_BITWISE_SHIFT_RIGHT; break;
    default: LOWER_NOT_IMPLEMENTED(L, assign, "this assignment operator");
    }

    // The place is evaluated once, e.g `a[f()] += 1` only calls f once
    int current = value_to_scalar(L, lhs);
    int rhs     = lower_scalar_as(L, assign->expr, type);
    int result  = emit_arithmetic(L, op, type, current, rhs, (Ast *) assign);
    emit_store(L, ir_type_of(type), lhs.addr, result);
}

static void lower_assert(Lowerer *L, AstAssert *assertion) {
    // A failed assert throws a Panic, with the place of the assert in its trace
    IrBlock *failed = create_block(L, "assert.failed");
    IrBlock *ok     = create_block(L, "assert.ok");
    lower_condition(L, assertion->expr, ok, failed);
    set_block(L, failed);
    emit_errors_call(L, "panicAssertion", 0, NULL);
    emit_trace_frame(L, assertion->head.start.line);
    emit_unwind(L);
    set_block(L, ok);
}

// The assert of the C runtime, which ends the program. Not used: an assert throws
__attribute__((unused)) static void lower_assert_with_c(Lowerer *L, AstAssert *assertion) {
    // runtime_builtin_assert(condition: bool, filename: string, line: u64)
    int args[3];
    args[0] = lower_scalar_as(L, assertion->expr, primitive_type(PRIMITIVE_BOOL));

    AstFile *file = assertion->head.file;
    if (L->assert_filename_file != file) {
        // The path is used as is (no escape sequences: Windows paths contain backslashes)
        char *path = file->absolute_path;
        int len = (int) strlen(path);
        IrGlobal *data = ir_global_data(L->module, "filename", path, len + 1, 1, true);
        int64_t header[2] = { 0, len };
        IrGlobal *str = ir_global_data(L->module, "filename_string", header, 16, 8, true);
        IrReloc reloc = { 0, data->index };
        ir_array_push(str->relocs, reloc);
        L->assert_filename_global = str->index;
        L->assert_filename_file = file;
    }
    args[1] = emit_global_addr(L, L->assert_filename_global);
    args[2] = emit_const(L, IR_I64, assertion->head.start.line);

    emit_c_call(L, "runtime_builtin_assert", IR_VOID, 3, args);
}

static void lower_if(Lowerer *L, AstIf *ast_if) {
    IrBlock *done    = create_block(L, "if.end");
    IrBlock *then_bb = create_block(L, "if.then");
    IrBlock *next_bb = create_block(L, "if.else");

    lower_condition(L, ast_if->condition, then_bb, next_bb);

    set_block(L, then_bb);
    lower_block(L, ast_if->then_block);
    terminate_jmp(L, done);

    for (int i = 0; i < ast_if->else_ifs.count; i++) {
        AstIf *else_if = da_get_deref(ast_if->else_ifs, i);

        set_block(L, next_bb);
        then_bb = create_block(L, "elseif.then");
        next_bb = create_block(L, "elseif.else");
        lower_condition(L, else_if->condition, then_bb, next_bb);

        set_block(L, then_bb);
        lower_block(L, else_if->then_block);
        terminate_jmp(L, done);
    }

    set_block(L, next_bb);
    if (ast_if->else_block) lower_block(L, ast_if->else_block);
    terminate_jmp(L, done);

    set_block(L, done);
}

static void push_loop(Lowerer *L, void *loop, IrBlock *break_block, IrBlock *continue_block) {
    LoopTargets targets = { loop, break_block, continue_block, L->defers.count };
    ir_array_push(L->loops, targets);
}

static void lower_statement(Lowerer *L, Ast *stmt);

// Runs the deferred statements above 'down_to', the last one first, e.g on the way out of a block or for a return.
// They stay active, as another way out of the block runs them too
static void emit_defers(Lowerer *L, int down_to) {
    int count = L->defers.count;
    if (count <= down_to) return;
    // A deferred statement runs with only the defers below it active, e.g an error it throws doesn't run it again.
    // What it pushes itself goes on top of those, so the defers above are kept aside and put back
    Ast **kept = malloc(sizeof(Ast *) * count);
    memcpy(kept, L->defers.items, sizeof(Ast *) * count);
    for (int i = count - 1; i >= down_to; i--) {
        if (block_is_terminated(L)) break;
        L->defers.count = i;
        lower_statement(L, kept[i]);
    }
    L->defers.count = 0;
    for (int i = 0; i < count; i++) ir_array_push(L->defers, kept[i]);
    free(kept);
}

//
// Throwing (std/prelude/errors.sd). The error that is being thrown is in the global thrownError. After a call of a Saad
// function, a thrown error goes to the catch of the try that the call is in, or the function returns, after running
// the defers on the way. Each place it leaves adds a frame to its trace. main reports an error that nothing caught
//

// A declaration of the prelude that the compiler uses, e.g panic or checkedBase: of errors.sd, allocator.sd or
// thread_blocks.sd
static AstIdentifier *errors_declaration(Lowerer *L, const char *name) {
    AstFile *files[3] = { L->parser->errors_file, L->parser->allocator_file, L->parser->thread_blocks_file };
    AstIdentifier *ident = NULL;
    for (int i = 0; i < 3 && ident == NULL; i++) {
        if (files[i]) ident = find_identifier_in_scope(files[i]->scope, (char *)name);
    }
    if (ident == NULL) lower_fail(L, NULL, "Compiler Error: '%s' of std/prelude/errors.sd isn't loaded", name);
    return ident;
}

static IrAddr thrown_error_addr(Lowerer *L) {
    return global_variable_addr_of(L, errors_declaration(L, "thrownError"), true);
}

// The address of a NUL terminated copy of the text, as written (no escapes, e.g a Windows path)
static int emit_c_text(Lowerer *L, const char *text) {
    int len = (int) strlen(text);
    IrGlobal *data = ir_global_data(L->module, "text", text, len + 1, 1, true);
    return emit_global_addr(L, data->index);
}

// Like emit_c_text, for a text that lives as long as the AST, e.g the name of a function or the path of a file. It is
// one global however many times it is used, e.g by the frames of the traces of a file
static int emit_shared_c_text(Lowerer *L, const char *text) {
    int global = ptr_map_get(&L->c_texts, (void *) text);
    if (global < 0) {
        global = ir_global_data(L->module, "text", text, (int) strlen(text) + 1, 1, true)->index;
        ptr_map_put(&L->c_texts, (void *) text, global);
    }
    return emit_global_addr(L, global);
}

// The text of the expression in its file, e.g user.manager, for a message. Empty when it isn't one short line
static void expr_source_text(AstExpr *expr, char *out, int size) {
    out[0] = 0;
    AstFile *file = expr->head.file;
    if (file == NULL || file->text == NULL) return;
    int start = expr->head.start.input_idx;
    int end   = expr->head.end.input_idx;
    int len   = end - start;
    if (start < 0 || len <= 0 || len >= size || len > 60 || start + len > (int) strlen(file->text)) return;
    for (int i = 0; i < len; i++) {
        char c = file->text[start + i];
        if (c == '\n' || c == '\r') { out[0] = 0; return; }
        out[i] = c;
    }
    out[len] = 0;
}

// If the expression is the receiver of the method that is being lowered, e.g self. A call of a method checks a receiver
// that is a pointer (see lower_call), so self isn't checked again each time it is used
static bool is_receiver_of_method(Lowerer *L, AstExpr *expr) {
    if (L->ast_func == NULL || L->ast_func->receiver == NULL) return false;
    if (expr->head.kind != AST_LITERAL || ((AstLiteral *) expr)->kind != LITERAL_IDENTIFIER) return false;
    AstIdentifier *ident = ((AstLiteral *) expr)->as.value.identifier.resolved_identifier;
    return ident != NULL && resolve_override(ident) == resolve_override(L->ast_func->receiver);
}

// A pointer that is null when it is dereferenced (p.x, *p and p[i]) throws a Panic, with the place in the trace,
// instead of crashing the program. Taking an address, e.g &p[i] or &p.x, only computes it, so its last step isn't
// checked, but the steps before it are, e.g p.next of &p.next.value. --unchecked leaves the check out. The functions of errors.sd, allocator.sd and thread_blocks.sd aren't checked, as the
// compiler calls some of them before a Panic can be thrown
static void emit_null_check(Lowerer *L, int ptr, AstExpr *expr) {
    AstFile *file = L->ast_func ? L->ast_func->head.head.file : NULL;
    if (file == NULL || is_runtime_file(L, file)) return;
    if (is_receiver_of_method(L, expr)) return;   // self: the call checked it
    if (!bounds_checks) {
        emit_memory_check(L, ptr, expr);
        return;
    }

    IrType ir_type = ir_vreg_type(L->func, ptr);
    IrBlock *is_null  = create_block(L, "null.pointer");
    IrBlock *not_null = create_block(L, "not.null");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_EQ, ir_type, ptr, emit_const(L, ir_type, 0)), is_null, not_null);
    set_block(L, is_null);
    char text[64];
    expr_source_text(expr, text, sizeof(text));
    int args[1] = { emit_c_text(L, text) };
    emit_errors_call(L, "panicNullPointer", 1, args);
    emit_trace_frame(L, expr->head.start.line);
    emit_unwind(L);
    set_block(L, not_null);
    emit_memory_check(L, ptr, expr);
}

static int emit_errors_global_load(Lowerer *L, IrType type, const char *name) {
    return emit_load(L, type, addr_global(global_variable(L, errors_declaration(L, name)), 0));
}

// With --check-memory: a pointer into the memory of an arena that was cleared or freed throws a Panic. The chunks of
// arenas are in one range of addresses, with a byte for each page that is 1 when the page was freed (see
// checkedAllocate in std/prelude/allocator.sd), so the check is a subtraction, a compare and, for a pointer in the range,
// the load of that byte. Pointers outside the range, e.g into the heap or the stack, aren't checked
static void emit_memory_check(Lowerer *L, int ptr, AstExpr *expr) {
    if (!memory_checks) return;
    AstFile *file = L->ast_func ? L->ast_func->head.head.file : NULL;
    if (file == NULL || is_runtime_file(L, file)) return;

    int address = ir_vreg_type(L->func, ptr) == IR_I64 ? ptr : emit_unop(L, IR_BITCAST, IR_I64, ptr);
    int offset  = emit_binop(L, IR_SUB, IR_I64, address, emit_errors_global_load(L, IR_I64, "checkedBase"));
    IrBlock *in_range = create_block(L, "checked.memory");
    IrBlock *freed    = create_block(L, "freed.memory");
    IrBlock *done     = create_block(L, "memory.checked");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_ULT, IR_I64, offset, emit_errors_global_load(L, IR_I64, "checkedSize")), in_range, done);

    set_block(L, in_range);
    int page  = emit_binop(L, IR_LSHR, IR_I64, offset, emit_const(L, IR_I64, 12));   // CheckedPageSize is 4096
    int state = emit_load(L, IR_I8, addr_vreg(emit_ptr_add(L, emit_errors_global_load(L, IR_PTR, "checkedPages"), page), 0));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_I8, state, emit_const(L, IR_I8, 0)), freed, done);

    set_block(L, freed);
    char text[64];
    expr_source_text(expr, text, sizeof(text));
    int args[1] = { emit_c_text(L, text) };
    emit_errors_call(L, "panicFreedMemory", 1, args);
    emit_trace_frame(L, expr->head.start.line);
    emit_unwind(L);
    set_block(L, done);
}

// The memory check of the bytes of a string, slice or list, at string_ptr, when it has any
static void emit_string_memory_check(Lowerer *L, int string_ptr, AstExpr *expr) {
    if (!memory_checks) return;
    AstFile *file = L->ast_func ? L->ast_func->head.head.file : NULL;
    if (file == NULL || is_runtime_file(L, file)) return;
    IrBlock *has_bytes = create_block(L, "string.has.bytes");
    IrBlock *done      = create_block(L, "string.checked");
    int count = emit_load(L, IR_I64, addr_vreg(string_ptr, 8));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_I64, count, emit_const(L, IR_I64, 0)), has_bytes, done);
    set_block(L, has_bytes);
    emit_memory_check(L, emit_load(L, IR_PTR, addr_vreg(string_ptr, 0)), expr);
    terminate_jmp(L, done);
    set_block(L, done);
}

// Calls a function of errors.sd, allocator.sd or thread_blocks.sd that takes no results, without the check for a thrown error after it
static void emit_errors_call(Lowerer *L, const char *name, int arg_count, int *args) {
    AstIdentifier *ident = errors_declaration(L, name);
    AstFunctionDefn *func = ((TypeFunction *) ident->type)->node;
    IrCall *call = new_call(L, IR_CALLEE_DIRECT, function_global(L, func), IR_VOID, arg_count);
    for (int i = 0; i < arg_count; i++) call_add_arg(call, args[i], false);
    emit_call(L, call);
}

// Adds the function that is being lowered, at the line, to the trace of the error that is being thrown. The functions of
// errors.sd, e.g panic, are left out
static void emit_trace_frame(Lowerer *L, int line) {
    AstFile *file = L->ast_func ? L->ast_func->head.head.file : NULL;
    if (file == NULL || is_runtime_file(L, file)) return;
    const char *function = L->ast_func->identifier && L->ast_func->identifier->name ? L->ast_func->identifier->name : "lambda";
    int args[3] = { emit_shared_c_text(L, function), emit_shared_c_text(L, file->absolute_path), emit_const(L, IR_I64, line) };
    emit_errors_call(L, "traceThrow", 3, args);
}

// If the type is error, a *Error of std/prelude/errors.sd
static bool is_error_pointer(Lowerer *L, Type *type) {
    if (type == NULL || type->kind != TYPE_POINTER) return false;
    Type *to = ((TypePointer *)type)->pointer_to;
    if (to == NULL || to->kind != TYPE_STRUCT) return false;
    TypeStruct *error_struct = (TypeStruct *)to;
    return error_struct->node && error_struct->node->head.file == L->parser->errors_file && strcmp(to->name, "Error") == 0;
}

// An error that the function returns as a value gets the function and the line in its trace (traceReturn of errors.sd): where
// it was made, e.g 'return 0, errors.make("empty text");', and each function that passes it on, e.g 'return 0, err;'. A
// caller that throws it adds its own frame after them. The functions that make errors, of the prelude and the namespace
// errors, are left out, as their caller made it
static void emit_trace_of_returned_error(Lowerer *L, int err, int line) {
    AstFile *file = L->ast_func ? L->ast_func->head.head.file : NULL;
    if (file == NULL || is_runtime_file(L, file)) return;
    if (file->ns && strcmp(file->ns->name, "errors") == 0) return;
    // The prelude, e.g allocateNew of new(Error) in collections.sd
    const char *runtime = L->parser->runtime_file_path;
    const char *slash   = runtime ? strrchr(runtime, '/') : NULL;
    const char *back    = runtime ? strrchr(runtime, '\\') : NULL;
    if (back > slash) slash = back;
    if (slash && file->absolute_path && strncmp(file->absolute_path, runtime, slash - runtime + 1) == 0) return;
    const char *function = L->ast_func->identifier && L->ast_func->identifier->name ? L->ast_func->identifier->name : "lambda";
    IrBlock *has_error = create_block(L, "returned.error");
    IrBlock *done      = create_block(L, "returned.error.traced");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, err, emit_const(L, IR_PTR, 0)), has_error, done);
    set_block(L, has_error);
    int args[4] = { err, emit_shared_c_text(L, function), emit_shared_c_text(L, file->absolute_path), emit_const(L, IR_I64, line) };
    emit_errors_call(L, "traceReturn", 4, args);
    terminate_jmp(L, done);
    set_block(L, done);
}

static bool is_main_function(Lowerer *L) {
    AstFunctionDefn *f = L->ast_func;
    return f && f->identifier && strcmp(f->identifier->name, "main") == 0 && f->head.head.file && f->head.head.file->ns == NULL && !f->is_method;
}

// Runs the defers above down_to while an error is thrown. The error is put aside meanwhile, so the calls of the defers
// run as usual instead of seeing it and leaving
static void emit_defers_while_throwing(Lowerer *L, int down_to) {
    if (L->defers.count <= down_to) return;
    int parked = ir_new_slot(L->func, 8, 8, "thrown error");
    emit_store(L, IR_PTR, addr_slot(parked, 0), emit_load(L, IR_PTR, thrown_error_addr(L)));
    emit_store(L, IR_PTR, thrown_error_addr(L), emit_const(L, IR_PTR, 0));
    emit_defers(L, down_to);
    if (!block_is_terminated(L)) emit_store(L, IR_PTR, thrown_error_addr(L), emit_load(L, IR_PTR, addr_slot(parked, 0)));
}

// Leaves for the catch of the innermost try, or returns from the function, running the defers on the way
static void emit_unwind(Lowerer *L) {
    if (L->tries.count > 0) {
        TryTarget target = L->tries.items[L->tries.count - 1];
        emit_defers_while_throwing(L, target.defer_count);
        terminate_jmp(L, target.catch_block);
        return;
    }
    emit_defers_while_throwing(L, 0);
    if (is_main_function(L)) emit_errors_call(L, "reportUncaught", 0, NULL); // Ends the program
    terminate_return(L, true);
}

// After a call: if it threw, the error goes on from here. 'threw' is the flag the callee returned in eax (see
// terminate_return). With 'confirm', a flag that isn't 0 is checked with thrownError, e.g for a function value that
// can be a function of C
static void emit_throw_check(Lowerer *L, int threw, bool confirm) {
    if (block_is_terminated(L)) return;
    IrBlock *throwing = create_block(L, "throw");
    IrBlock *go_on    = create_block(L, "no.throw");
    int flag_set = emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_I32, threw, emit_const(L, IR_I32, 0));
    if (confirm) {
        IrBlock *check = create_block(L, "check.throw");
        terminate_br(L, flag_set, check, go_on);
        set_block(L, check);
        int thrown = emit_load(L, IR_PTR, thrown_error_addr(L));
        terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, thrown, emit_const(L, IR_PTR, 0)), throwing, go_on);
    } else {
        terminate_br(L, flag_set, throwing, go_on);
    }
    set_block(L, throwing);
    emit_trace_frame(L, L->line);
    emit_unwind(L);
    set_block(L, go_on);
}

// Throws the error, a *Error in the vreg, when it isn't null
static void emit_throw(Lowerer *L, int err) {
    IrBlock *throwing = create_block(L, "throw");
    IrBlock *go_on    = create_block(L, "no.error");
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_NE, IR_PTR, err, emit_const(L, IR_PTR, 0)), throwing, go_on);
    set_block(L, throwing);
    emit_store(L, IR_PTR, thrown_error_addr(L), err);
    emit_trace_frame(L, L->line);
    emit_unwind(L);
    set_block(L, go_on);
}

static void lower_while(Lowerer *L, AstWhile *ast_while) {
    IrBlock *cond = create_block(L, "while.cond");
    IrBlock *body = create_block(L, "while.body");
    IrBlock *done = create_block(L, "while.end");

    terminate_jmp(L, cond);
    set_block(L, cond);
    lower_condition(L, ast_while->condition, body, done);

    set_block(L, body);
    push_loop(L, ast_while, done, cond);
    lower_block(L, ast_while->body);
    L->loops.count--;
    terminate_jmp(L, cond);

    set_block(L, done);
}

static void lower_for(Lowerer *L, AstFor *ast_for) {
    IrBlock *cond = create_block(L, "for.cond");
    IrBlock *body = create_block(L, "for.body");
    IrBlock *post = create_block(L, "for.post");
    IrBlock *done = create_block(L, "for.end");

    AstExpr *iterable = ast_for->iterable;

    // for { ... }
    if (iterable == NULL) {
        terminate_jmp(L, body);
        set_block(L, body);
        push_loop(L, ast_for, done, body);
        lower_block(L, ast_for->body);
        L->loops.count--;
        terminate_jmp(L, body);
        set_block(L, done);
        return;
    }

    // for i in a..b { ... }
    if (iterable->head.kind == AST_RANGE_EXPR) {
        AstRangeExpr *range = (AstRangeExpr *) iterable;
        AstIdentifier *iterator = resolve_override(ast_for->iterator);
        Type *it_type = iterator->type;
        IrType it_ir = ir_type_of(it_type);

        int it_slot  = declare_local(L, iterator);
        int end_slot = ir_new_slot(L->func, ir_type_size(it_ir), ir_type_size(it_ir), "range end");
        int index_slot = -1;

        emit_store(L, it_ir, addr_slot(it_slot, 0),  lower_scalar_as(L, range->start, it_type));
        emit_store(L, it_ir, addr_slot(end_slot, 0), lower_scalar_as(L, range->end,   it_type));
        if (ast_for->index) {
            AstIdentifier *index = resolve_override(ast_for->index);
            index_slot = declare_local(L, index);
            emit_store(L, ir_type_of(index->type), addr_slot(index_slot, 0), emit_const(L, ir_type_of(index->type), 0));
        }
        terminate_jmp(L, cond);

        set_block(L, cond);
        int it  = emit_load(L, it_ir, addr_slot(it_slot, 0));
        int end = emit_load(L, it_ir, addr_slot(end_slot, 0));
        bool is_signed = type_is_signed(it_type);
        IrCmp cmp = range->inclusive ? (is_signed ? IR_CMP_SLE : IR_CMP_ULE) : (is_signed ? IR_CMP_SLT : IR_CMP_ULT);
        terminate_br(L, emit_cmp(L, IR_ICMP, cmp, it_ir, it, end), body, done);

        set_block(L, body);
        push_loop(L, ast_for, done, post);
        lower_block(L, ast_for->body);
        L->loops.count--;
        terminate_jmp(L, post);

        set_block(L, post);
        int next = emit_binop(L, IR_ADD, it_ir, emit_load(L, it_ir, addr_slot(it_slot, 0)), emit_const(L, it_ir, 1));
        emit_store(L, it_ir, addr_slot(it_slot, 0), next);
        if (index_slot >= 0) {
            IrType index_ir = ir_type_of(resolve_override(ast_for->index)->type);
            int next_index = emit_binop(L, IR_ADD, index_ir, emit_load(L, index_ir, addr_slot(index_slot, 0)), emit_const(L, index_ir, 1));
            emit_store(L, index_ir, addr_slot(index_slot, 0), next_index);
        }
        terminate_jmp(L, cond);

        set_block(L, done);
        return;
    }

    // for x in array / slice / dynamic array / variadic / string
    Type *iterable_type = iterable->type;
    Type *elem_type = NULL;
    int data_ptr, count;
    {
        Value place = value_to_place(L, lower_expr(L, iterable));
        if (is_fixed_array(iterable_type)) {
            TypeArray *array = (TypeArray *) iterable_type;
            elem_type = array->elem_type;
            data_ptr  = addr_to_ptr(L, place.addr);
            count     = emit_const(L, IR_I64, fixed_array_count(array));
        } else {
            if (iterable_type->kind == TYPE_ARRAY)         elem_type = ((TypeArray *) iterable_type)->elem_type;
            else if (iterable_type->kind == TYPE_VARIADIC) elem_type = ((TypeVariadic *) iterable_type)->type;
            else if (iterable_type->kind == TYPE_STRING)   elem_type = primitive_type(PRIMITIVE_U8);
            else LOWER_NOT_IMPLEMENTED(L, iterable, "iterating over this type");
            data_ptr = emit_load(L, IR_PTR, place.addr);
            count    = emit_load(L, IR_I64, addr_add(place.addr, 8));
        }
    }

    // Loop state: data pointer, count and an i64 counter, all in slots
    int data_slot    = ir_new_slot(L->func, 8, 8, "for data");
    int count_slot   = ir_new_slot(L->func, 8, 8, "for count");
    int counter_slot = ir_new_slot(L->func, 8, 8, "for counter");
    emit_store(L, IR_PTR, addr_slot(data_slot, 0), data_ptr);
    emit_store(L, IR_I64, addr_slot(count_slot, 0), count);
    emit_store(L, IR_I64, addr_slot(counter_slot, 0), emit_const(L, IR_I64, 0));

    AstIdentifier *iterator = resolve_override(ast_for->iterator);
    int iterator_slot = declare_local(L, iterator);
    int index_slot = -1;
    if (ast_for->index) index_slot = declare_local(L, ast_for->index);

    terminate_jmp(L, cond);

    set_block(L, cond);
    int counter = emit_load(L, IR_I64, addr_slot(counter_slot, 0));
    int limit   = emit_load(L, IR_I64, addr_slot(count_slot, 0));
    terminate_br(L, emit_cmp(L, IR_ICMP, IR_CMP_SLT, IR_I64, counter, limit), body, done);

    set_block(L, body);
    {
        int i    = emit_load(L, IR_I64, addr_slot(counter_slot, 0));
        int data = emit_load(L, IR_PTR, addr_slot(data_slot, 0));
        int elem = emit_ptr_add(L, data, emit_binop(L, IR_MUL, IR_I64, i, emit_const(L, IR_I64, elem_type->size)));

        if (ast_for->is_by_pointer) {
            emit_store(L, IR_PTR, addr_slot(iterator_slot, 0), elem);
        } else {
            store_value(L, addr_slot(iterator_slot, 0), iterator->type, value_place(addr_vreg(elem, 0), elem_type), (Ast *) ast_for);
        }
        if (index_slot >= 0) {
            Type *index_type = resolve_override(ast_for->index)->type;
            emit_store(L, ir_type_of(index_type), addr_slot(index_slot, 0), convert_scalar(L, i, primitive_type(PRIMITIVE_I64), index_type));
        }
    }
    push_loop(L, ast_for, done, post);
    lower_block(L, ast_for->body);
    L->loops.count--;
    terminate_jmp(L, post);

    set_block(L, post);
    int next = emit_binop(L, IR_ADD, IR_I64, emit_load(L, IR_I64, addr_slot(counter_slot, 0)), emit_const(L, IR_I64, 1));
    emit_store(L, IR_I64, addr_slot(counter_slot, 0), next);
    terminate_jmp(L, cond);

    set_block(L, done);
}

static void lower_break_or_continue(Lowerer *L, AstBreakOrContinue *boc) {
    EnclosingLoop *loop = boc->enclosing_loop;
    if (loop == NULL) lower_fail(L, (Ast *) boc, "Compiler Error: break/continue outside of a loop");
    void *key = loop->type == TOKEN_FOR ? (void *) loop->for_loop : (void *) loop->while_loop;

    for (int i = L->loops.count - 1; i >= 0; i--) {
        if (L->loops.items[i].loop == key) {
            bool is_break = boc->token.type == TOKEN_BREAK;
            emit_defers(L, L->loops.items[i].defer_count);
            terminate_jmp(L, is_break ? L->loops.items[i].break_block : L->loops.items[i].continue_block);
            start_unreachable_block(L);
            return;
        }
    }
    lower_fail(L, (Ast *) boc, "Compiler Error: Could not find the loop for this break/continue");
}

static void lower_return(Lowerer *L, AstReturn *ret) {
    AstFunctionDefn *func_defn = L->ast_func;

    for (int i = 0; i < ret->values.count; i++) {
        AstExpr *value = da_get_deref(ret->values, i);
        if (value->type->kind == TYPE_VOID) continue; // `return;`

        // return f() of a function that gives several values: they are the values of this one
        if (value->type->kind == TYPE_TUPLE) {
            if (value->head.kind != AST_FUNCTION_CALL || ret->values.count != 1) {
                LOWER_NOT_IMPLEMENTED(L, value, "returning several values of a call together with other values");
            }
            TypeTuple *tuple = (TypeTuple *) value->type;
            if (tuple->types.count != func_defn->lowered_return_params.count) {
                lower_fail(L, (Ast *) value, "Compiler Error: The call gives %d values, and the function returns %d", tuple->types.count, func_defn->lowered_return_params.count);
            }
            int ret_slots[16];
            lower_call(L, (AstFunctionCall *) value, ret_slots, 16);
            for (int j = 0; j < tuple->types.count; j++) {
                AstIdentifier *ret_param = da_get_deref(func_defn->lowered_return_params, j);
                Type *ret_type = da_get_deref(func_defn->return_types, j);
                int out_ptr = emit_load(L, IR_PTR, addr_slot(local_slot(L, ret_param), 0));
                store_value(L, addr_vreg(out_ptr, 0), ret_type, value_place(addr_slot(ret_slots[j], 0), da_get_deref(tuple->types, j)), (Ast *) value);
            }
            break;
        }
        if (i >= func_defn->lowered_return_params.count) {
            lower_fail(L, (Ast *) value, "Compiler Error: More return values than return parameters");
        }

        AstIdentifier *ret_param = da_get_deref(func_defn->lowered_return_params, i);
        Type *ret_type = da_get_deref(func_defn->return_types, i);
        int out_ptr = emit_load(L, IR_PTR, addr_slot(local_slot(L, ret_param), 0));
        store_expr(L, addr_vreg(out_ptr, 0), ret_type, value, false);

        // Where an error that is returned was made, unless it is null
        if (is_error_pointer(L, ret_type) && !(value->head.kind == AST_LITERAL && ((AstLiteral *)value)->kind == LITERAL_NULL)) {
            emit_trace_of_returned_error(L, emit_load(L, IR_PTR, addr_vreg(out_ptr, 0)), ret->head.start.line);
        }
    }

    // The values are taken before the deferred statements run
    emit_defers(L, 0);
    terminate_return(L, false);
    start_unreachable_block(L);
}

static void lower_statement(Lowerer *L, Ast *stmt) {
    // The lines are of the file of the function. A statement from another file keeps the line of where it is used
    if (!L->ast_func || stmt->file == L->ast_func->head.head.file) L->line = stmt->start.line;

    switch (stmt->kind) {
    case AST_DECLARATION:       lower_declaration(L, (AstDeclaration *) stmt); return;
    case AST_ASSIGNMENT:        lower_assignment(L, (AstAssignment *) stmt); return;
    case AST_EXPR_STMT:         lower_expr(L, ((AstExprStmt *) stmt)->expr); return;
    case AST_BLOCK:             lower_block(L, (AstBlock *) stmt); return;
    case AST_ASSERT:            lower_assert(L, (AstAssert *) stmt); return;
    case AST_IF:                lower_if(L, (AstIf *) stmt); return;
    case AST_SWITCH:            lower_block(L, ((AstSwitch *) stmt)->lowered); return;
    case AST_SWITCH_ARM: {
        // The value of a case of a switch expression goes in the slot of the switch
        AstSwitchArm *arm = (AstSwitchArm *) stmt;
        store_expr(L, addr_slot(arm->owner->result_slot, 0), arm->owner->result_type, arm->value, false);
        return;
    }
    case AST_WHILE:             lower_while(L, (AstWhile *) stmt); return;
    case AST_FOR: {
        // The loop variables are in a scope of the for, so two loops can have an 'i' each
        int outer = enter_scope(L, stmt);
        lower_for(L, (AstFor *) stmt);
        L->scope = outer;
        return;
    }
    case AST_BREAK_OR_CONTINUE: lower_break_or_continue(L, (AstBreakOrContinue *) stmt); return;
    case AST_RETURN:            lower_return(L, (AstReturn *) stmt); return;
    case AST_DEFER:             ir_array_push(L->defers, ((AstDefer *) stmt)->statement); return; // Runs when the block is left
    case AST_TRY: {
        AstTry *ast_try = (AstTry *) stmt;
        IrBlock *catch_block = create_block(L, "catch");
        IrBlock *done        = create_block(L, "try.end");
        TryTarget target = { catch_block, L->defers.count };
        ir_array_push(L->tries, target);
        lower_block(L, ast_try->body);
        L->tries.count--;
        terminate_jmp(L, done);

        // The handler has the error, and nothing is being thrown anymore
        set_block(L, catch_block);
        int outer = enter_scope(L, (Ast *) ast_try->handler);    // The error is only seen in the handler
        int err = emit_load(L, IR_PTR, thrown_error_addr(L));
        emit_store(L, IR_PTR, thrown_error_addr(L), emit_const(L, IR_PTR, 0));
        int slot = declare_local(L, ast_try->error);
        emit_store(L, IR_PTR, addr_slot(slot, 0), err);
        lower_block(L, ast_try->handler);
        L->scope = outer;
        terminate_jmp(L, done);
        set_block(L, done);
        return;
    }
    case AST_THROW: {
        AstThrow *ast_throw = (AstThrow *) stmt;
        int err = lower_scalar_as(L, ast_throw->value, ast_throw->error_type ? ast_throw->error_type : ast_throw->value->type);
        emit_throw(L, err);
        return;
    }

    // Declarations that produce no code at the point where they appear
    case AST_STRUCT:
    case AST_ENUM:
    case AST_IMPORT:
    case AST_EXTERN:
    case AST_DIRECTIVE:
    case AST_FUNCTION_DEFN:
    case AST_SEMICOLON_EXPR:
        return;

    default:
        lower_fail(L, stmt, "Not implemented in the IR backend yet: statement '%s'", ast_to_str(stmt));
    }
}

static void lower_block(Lowerer *L, AstBlock *block) {
    // A block has a scope, except the body of the function, whose scope is the function
    int outer = L->scope;
    if (!L->ast_func || block != L->ast_func->body) enter_scope(L, (Ast *) block);
    int defers = L->defers.count;
    for (int i = 0; i < block->statements.count; i++) {
        lower_statement(L, da_get_deref(block->statements, i));
    }
    // What comes after the statements, e.g the deferred ones or the step of a loop, is at the line of the '}'
    if ((!L->ast_func || block->head.file == L->ast_func->head.head.file) && block->head.end.line > 0) L->line = block->head.end.line;
    emit_defers(L, defers); // At the end of the block. A return, break or continue in it ran them already
    L->defers.count = defers;
    L->scope = outer;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Functions and the module
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct PendingFunctions {
    IrArray(AstFunctionDefn *) list;
    PtrMap queued;      // AstFunctionDefn* -> global index
} PendingFunctions;

static PendingFunctions pending_functions; // Reset for every module

// The invoke() of a function with attribute values (see AttributedFunction of runtime.sd): it reads the arguments from
// a block, calls the function, and writes the results to a block
typedef struct PendingThunk {
    AstFunctionDefn *target;
    int              global;
    int             *argument_offsets;
    int             *result_offsets;
} PendingThunk;

static IrArray(PendingThunk) pending_thunks;

// The global of a function symbol. Saad functions get queued for lowering the first time they are referenced
static int function_global(Lowerer *L, AstFunctionDefn *func_defn) {
    int global = ptr_map_get(&pending_functions.queued, func_defn);
    if (global >= 0) return global;

    char *name = func_defn->symbol_name ? func_defn->symbol_name : func_defn->identifier->name;

    if (func_defn->is_extern) {
        global = ir_find_or_add_extern(L->module, name);
        if (func_defn->extern_library) L->module->globals.items[global]->library = func_defn->extern_library;
    } else {
        // Different functions can share a name (e.g inner functions in different scopes), so a Saad
        // function whose name is already taken gets a unique symbol
        char unique[256];
        ir_unique_name(L->module, name, unique, sizeof(unique));
        IrGlobal *g = ir_global_new(L->module, IR_GLOBAL_EXTERN, unique); // Becomes a FUNCTION when lowered
        g->is_saad = true;
        global = g->index;
        ir_array_push(pending_functions.list, func_defn);
    }

    ptr_map_put(&pending_functions.queued, func_defn, global);
    return global;
}

// The global of the invoke() of a function, which is lowered after the functions (lower_thunk)
static int thunk_global(Lowerer *L, AstFunctionDefn *target, int *argument_offsets, int *result_offsets) {
    function_global(L, target);
    char name[300];
    snprintf(name, sizeof(name), "invoke_%s", target->identifier ? target->identifier->name : "function");
    char unique[320];
    ir_unique_name(L->module, name, unique, sizeof(unique));
    IrGlobal *g = ir_global_new(L->module, IR_GLOBAL_EXTERN, unique); // Becomes a FUNCTION when lowered
    g->is_saad = true;
    PendingThunk thunk = { target, g->index, argument_offsets, result_offsets };
    ir_array_push(pending_thunks, thunk);
    return g->index;
}

// invoke(arguments: *u8, results: *u8): calls the target with the arguments of the block, and its out-pointers into the
// block of the results. It returns the flag of the target, so what the target throws goes on
static void lower_thunk(Lowerer *L, PendingThunk *thunk) {
    AstFunctionDefn *target = thunk->target;
    IrFunction *func = ir_function_new(L->module, L->module->globals.items[thunk->global]->name);
    func->return_type = IR_I32;
    L->func     = func;
    L->ast_func = target;
    L->scope    = 0;
    L->loops.count = 0;
    L->defers.count = 0;
    L->tries.count = 0;
    L->thread_block_slot = -1;
    ptr_map_clear(&L->locals);

    IrBlock *entry = create_block(L, "entry");
    set_block(L, entry);
    int arguments = ir_new_vreg(func, IR_PTR);
    int results   = ir_new_vreg(func, IR_PTR);
    ir_array_push(func->params, arguments);
    ir_array_push(func->params, results);

    int ret_count   = target->lowered_return_params.count;
    int param_count = target->parameters.count;
    IrCall *call = new_call(L, IR_CALLEE_DIRECT, function_global(L, target), IR_I32, ret_count + param_count);
    for (int i = 0; i < ret_count; i++) {
        call_add_arg(call, emit_ptr_add(L, results, emit_const(L, IR_I64, thunk->result_offsets[i])), false);
    }
    for (int i = 0; i < param_count; i++) {
        AstIdentifier *param = resolve_override(da_get_deref(target->lowered_params, ret_count + i));
        Type *type = param->type;
        IrAddr addr = addr_vreg(arguments, thunk->argument_offsets[i]);
        int value;
        switch (classify_argument(type)) {
        case ARG_SCALAR:                 value = emit_load(L, ir_type_of(type), addr); break;
        case ARG_AGGREGATE_BY_VALUE:     value = emit_load(L, int_ir_type_of_size(type->size), addr); break;
        default:                         value = emit_ptr_add(L, arguments, emit_const(L, IR_I64, thunk->argument_offsets[i])); break;
        }
        call_add_arg(call, value, false);
    }
    int threw = emit_call(L, call);
    terminate_ret(L, threw);
    ir_function_finalize(func);
}

static void lower_function(Lowerer *L, AstFunctionDefn *func_defn) {
    int global = ptr_map_get(&pending_functions.queued, func_defn);
    IrFunction *func = ir_function_new(L->module, L->module->globals.items[global]->name);
    func->is_entry_point = strcmp(func_defn->symbol_name, "main") == 0 && func_defn->parent_function == NULL;
    func->return_type    = IR_I32; // If an error was thrown out of it, see terminate_return
    AstFile *file = func_defn->head.head.file;
    if (file && file->absolute_path) {
        func->source_file = ir_strdup(L->module, file->absolute_path);
        func->source_name = ir_strdup(L->module, func_defn->display_name ? func_defn->display_name : func->name);
        func->source_line = func_defn->head.head.start.line;
    }

    L->func     = func;
    L->ast_func = func_defn;
    L->line     = func_defn->head.head.start.line;
    L->scope    = 0;
    IrScope function_scope = { 0, L->line };
    ir_array_push(func->scopes, function_scope);
    L->loops.count = 0;
    L->defers.count = 0;
    L->tries.count = 0;
    L->thread_block_slot = -1;
    ptr_map_clear(&L->locals);

    // Filled in at the end, e.g with loading the block of the @threadLocal globals when the function uses one
    IrBlock *prologue = create_block(L, "prologue");
    set_block(L, prologue);
    IrBlock *entry = create_block(L, "entry");
    set_block(L, entry);

    // Parameters arrive in vregs and are moved into their home slots right away
    for (int i = 0; i < func_defn->lowered_params.count; i++) {
        AstIdentifier *param = resolve_override(da_get_deref(func_defn->lowered_params, i));
        Type *type = param->type;

        int vreg = ir_new_vreg(func, ir_param_type(type));
        ir_array_push(func->params, vreg);

        int locals = func->locals.count;
        int slot = declare_local(L, param);
        // A parameter of the program is numbered for the debug information. The hidden ones, e.g for the results, aren't
        if (func->locals.count > locals) {
            int ret_count = func_defn->lowered_return_params.count;
            if (i < ret_count) func->locals.count--;
            else               func->locals.items[func->locals.count - 1].arg = i - ret_count + 1;
        }
        switch (classify_argument(type)) {
        case ARG_SCALAR:
            emit_store(L, ir_type_of(type), addr_slot(slot, 0), vreg);
            break;
        case ARG_AGGREGATE_BY_VALUE:
            emit_store(L, int_ir_type_of_size(type->size), addr_slot(slot, 0), vreg);
            break;
        case ARG_AGGREGATE_BY_REFERENCE:
            emit_memcpy(L, emit_slot_addr(L, slot), vreg, type->size);
            break;
        }
    }

    lower_block(L, func_defn->body);
    terminate_return(L, false);

    set_block(L, prologue);
    L->line  = func_defn->head.head.start.line;   // The prologue is at the line of the function, not of its last statement
    L->scope = 0;
    if (func->is_entry_point) {
        int args[2] = { emit_global_addr(L, L->thread_template), emit_const(L, IR_I64, L->thread_block_size) };
        emit_errors_call(L, "startThreadBlocks", 2, args);
    }
    bool is_thread_helper = func_defn->head.head.file == L->parser->thread_blocks_file && func_defn->identifier &&
                            (strcmp(func_defn->identifier->name, "startThreadBlocks") == 0 || strcmp(func_defn->identifier->name, "newThreadBlock") == 0);
    if (L->thread_block_slot >= 0) {
        if (is_thread_helper) lower_fail(L, (Ast *)func_defn, "Compiler Error: %s uses a @threadLocal global", func_defn->identifier->name);
        emit_store(L, IR_PTR, addr_slot(L->thread_block_slot, 0), emit_lookup_thread_block(L));
    }
    terminate_jmp(L, entry);

    ir_function_finalize(func);
}

// Lowers the program starting from 'main' and every function reachable from it.
// Returns NULL if lowering failed (the error has already been reported)
IrModule *lower_program(Parser *parser, AstFile *main_file) {
    Lowerer *L = calloc(1, sizeof(Lowerer));
    L->parser = parser;
    L->module = ir_module_new();
    L->assert_filename_global = -1;

    pending_functions = (PendingFunctions){0};
    pending_thunks    = (typeof(pending_thunks)){0};

    if (setjmp(L->on_error)) {
        return NULL;
    }

    AstIdentifier *main_ident = NULL;
    for (int i = 0; i < main_file->scope->identifiers.count; i++) {
        AstIdentifier *ident = da_get_deref(main_file->scope->identifiers, i);
        if (strcmp(ident->name, "main") == 0) {
            main_ident = ident;
            break;
        }
    }
    if (main_ident == NULL || main_ident->type == NULL || main_ident->type->kind != TYPE_FUNCTION) {
        report_error_ast(parser, LABEL_ERROR, NULL, "No main function was found in '%s'", main_file->absolute_path);
        return NULL;
    }

    for (int i = 0; i < main_file->imports.count; i++) {
        AstImport *import = da_get_deref(main_file->imports, i);
        if (import->imported_file && strcmp(import->string, "runtime") == 0) L->runtime_scope = import->imported_file->scope;
    }

    layout_thread_locals(L);
    function_global(L, ((TypeFunction *) main_ident->type)->node);

    // A thunk can queue functions, and a function can queue thunks (by using attributedFunctions())
    int next_function = 0, next_thunk = 0;
    while (next_function < pending_functions.list.count || next_thunk < pending_thunks.count) {
        if (next_function < pending_functions.list.count) {
            lower_function(L, pending_functions.list.items[next_function++]);
        } else {
            lower_thunk(L, &pending_thunks.items[next_thunk++]);
        }
    }

    return L->module;
}
