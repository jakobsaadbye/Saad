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
} LoopTargets;

typedef struct PtrMapEntry {
    void *key;
    int   value;
} PtrMapEntry;

typedef struct PtrMap {
    PtrMapEntry *entries;
    int          capacity;
    int          count;
} PtrMap;

typedef struct Lowerer {
    Parser          *parser;
    IrModule        *module;

    // Per function state
    IrFunction      *func;
    IrBlock         *block;
    AstFunctionDefn *ast_func;
    PtrMap           locals;          // AstIdentifier* -> slot index
    IrArray(LoopTargets) loops;
    int              line;

    // Module state
    PtrMap           const_globals;   // AstIdentifier* -> global index of an aggregate constant
    int              assert_filename_global; // Cached per file
    AstFile         *assert_filename_file;

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
static int convert_scalar(Lowerer *L, int vreg, Type *from, Type *to) {
    IrType src = ir_vreg_type(L->func, vreg);
    IrType dst = ir_type_of(to);
    assert(dst != IR_VOID);

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

// NUL terminated character data used as is (no escape sequences)
static int string_data_global_cstr(Lowerer *L, const char *text) {
    IrGlobal *data = ir_global_data(L->module, "str", text, (int) strlen(text) + 1, 1, true);
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
static void write_constant(Lowerer *L, IrGlobal *global, int offset, AstExpr *expr, Type *type) {
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
    return slot;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

static Value lower_expr(Lowerer *L, AstExpr *expr);
static Value lower_place(Lowerer *L, AstExpr *expr);
static int   lower_scalar_as(Lowerer *L, AstExpr *expr, Type *target);
static void  lower_condition(Lowerer *L, AstExpr *expr, IrBlock *on_true, IrBlock *on_false);
static void  lower_block(Lowerer *L, AstBlock *block);
static void  lower_statement(Lowerer *L, Ast *stmt);
static void  lower_print(Lowerer *L, AstPrint *print);
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

static int64_t dynamic_array_initial_capacity(TypeArray *array, int64_t count) {
    int64_t capacity = array->capacity > count ? array->capacity : count;
    return capacity > 0 ? capacity : 8; // The runtime grows by doubling, so it must never start at 0
}

// Writes the header of an empty dynamic array: { data = calloc(capacity, elem_size), count = 0, capacity, elem_size }
static void emit_new_dynamic_array(Lowerer *L, IrAddr dst, TypeArray *array, int64_t count, int *out_data_ptr) {
    int64_t capacity = dynamic_array_initial_capacity(array, count);
    int args[2] = { emit_const(L, IR_I64, capacity), emit_const(L, IR_I64, array->elem_type->size) };
    int data_ptr = emit_c_call(L, "calloc", IR_PTR, 2, args);
    emit_store(L, IR_PTR, dst, data_ptr);
    emit_store(L, IR_I64, addr_add(dst, 8),  emit_const(L, IR_I64, count));
    emit_store(L, IR_I64, addr_add(dst, 16), emit_const(L, IR_I64, capacity));
    emit_store(L, IR_I64, addr_add(dst, 24), emit_const(L, IR_I64, array->elem_type->size));
    if (out_data_ptr) *out_data_ptr = data_ptr;
}

// Dynamic arrays are the only type whose default value is not all zeros. Initializes the ones inside 'type'
static void emit_init_dynamic_arrays(Lowerer *L, IrAddr addr, Type *type) {
    if (type->kind == TYPE_ARRAY) {
        TypeArray *array = (TypeArray *) type;
        if (array->array_kind == ARRAY_DYNAMIC) {
            emit_new_dynamic_array(L, addr, array, 0, NULL);
        } else if (array->array_kind == ARRAY_FIXED) {
            Type *elem = array->elem_type;
            bool contains = elem->kind == TYPE_STRUCT || (elem->kind == TYPE_ARRAY && ((TypeArray *) elem)->array_kind != ARRAY_SLICE);
            if (contains) {
                int64_t count = fixed_array_count(array);
                for (int64_t i = 0; i < count && i < 256; i++) emit_init_dynamic_arrays(L, addr_add(addr, (int)(i * elem->size)), elem);
            }
        }
        return;
    }
    if (type->kind == TYPE_STRUCT) {
        DynamicArray members = ((TypeStruct *) type)->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
            emit_init_dynamic_arrays(L, addr_add(addr, member->member_offset), member->type);
        }
    }
}

// The value of a variable declared without an initializer
static void emit_default_init(Lowerer *L, IrAddr addr, Type *type) {
    emit_zero(L, addr, type_storage_size(type));
    emit_init_dynamic_arrays(L, addr, type);
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

// Converts an aggregate value to another aggregate type with a compatible layout, e.g a fixed array to a slice
static Value convert_aggregate(Lowerer *L, Value value, Type *to, Ast *site) {
    Type *from = value.type;
    if (from == to) return value;

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
        if (!initialized) emit_init_dynamic_arrays(L, addr_add(dst, member->member_offset), member->type);
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
    case ARRAY_DYNAMIC: {
        // { data, count, capacity, elem_size } with the data allocated on the heap
        int data_ptr;
        emit_new_dynamic_array(L, dst, array_type, count, &data_ptr);
        lower_array_elements_into(L, lit, addr_vreg(data_ptr, 0), elem_type);
        return;
    }
    }
}

// Lowers aggregate literals directly into their destination. Returns false if 'expr' is not such a literal.
// Only safe when the destination is fresh memory that the expression can not refer to.
static bool lower_into(Lowerer *L, AstExpr *expr, IrAddr dst, Type *dst_type) {
    if (expr->type != dst_type) return false;
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
        // A string literal typed as a pointer (e.g passed to a *u8 parameter) is just the character data
        int len = 0;
        int data = string_data_global(L, lit->as.value.string.data, lit->as.value.string.length, &len);
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

static Value lower_identifier(Lowerer *L, AstLiteral *lit) {
    AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
    if (ident == NULL) lower_fail(L, (Ast *) lit, "Compiler Error: Unresolved identifier");
    ident = resolve_override(ident);

    if (ident->flags & IDENTIFIER_IS_CONSTANT) {
        return lower_constant(L, ident, (Ast *) lit);
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
static IrAddr member_base(Lowerer *L, AstExpr *left) {
    if (left->type->kind == TYPE_POINTER) {
        int ptr = value_to_scalar(L, lower_expr(L, left));
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

        IrAddr base = member_base(L, ma->left);
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
    if (left_type->kind == TYPE_POINTER) {
        data = value_to_scalar(L, lower_expr(L, aa->left));
    } else if (is_fixed_array(left_type)) {
        data = addr_to_ptr(L, lower_place(L, aa->left).addr);
    } else {
        // slices, dynamic arrays, variadics and strings all start with a data pointer
        data = emit_load(L, IR_PTR, lower_place(L, aa->left).addr);
    }

    int index  = lower_scalar_as(L, aa->index_expr, primitive_type(PRIMITIVE_I64));
    int offset = emit_binop(L, IR_MUL, IR_I64, index, emit_const(L, IR_I64, elem_type->size));
    return addr_vreg(emit_ptr_add(L, data, offset), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Operators
//////////////////////////////////////////////////////////////////////////////////////////

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

static int lower_comparison(Lowerer *L, AstBinary *bin) {
    Type *left  = bin->left->type;
    Type *right = bin->right->type;

    if (left->kind == TYPE_STRING && right->kind == TYPE_STRING) {
        // runtime_compare_strings(*string, *string) -> int  (<0, 0, >0)
        int args[2];
        args[0] = addr_to_ptr(L, value_to_place(L, lower_expr(L, bin->left)).addr);
        args[1] = addr_to_ptr(L, value_to_place(L, lower_expr(L, bin->right)).addr);
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

static Value lower_binary(Lowerer *L, AstBinary *bin) {
    int op = bin->operator;
    Type *type = bin->head.type;

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
        Value place = lower_place(L, unary->expr);
        return value_scalar(addr_to_ptr(L, place.addr), type);
    }
    case OP_POINTER_DEREFERENCE: {
        int ptr = value_to_scalar(L, lower_expr(L, unary->expr));
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

static Value lower_cast(Lowerer *L, AstCast *cast) {
    Type *from = cast->expr->type;
    Type *to   = cast->cast_to;

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

// dst = cond ? if_true : if_false
static int emit_select(Lowerer *L, int cond, int if_true, int if_false) {
    IrType type = ir_vreg_type(L->func, if_true);
    int slot = ir_new_slot(L->func, ir_type_size(type), ir_type_size(type), "select");
    IrBlock *on_true  = create_block(L, "select.true");
    IrBlock *on_false = create_block(L, "select.false");
    IrBlock *done     = create_block(L, "select.end");
    terminate_br(L, cond, on_true, on_false);
    set_block(L, on_true);
    emit_store(L, type, addr_slot(slot, 0), if_true);
    terminate_jmp(L, done);
    set_block(L, on_false);
    emit_store(L, type, addr_slot(slot, 0), if_false);
    terminate_jmp(L, done);
    set_block(L, done);
    return emit_load(L, type, addr_slot(slot, 0));
}

// Returns a ptr to a string ({data, len}) holding the name of an enum value. Unknown values give "<invalid>"
static int enum_value_name(Lowerer *L, int value, TypeEnum *enum_type) {
    AstEnum *ast_enum = enum_type->node;
    int64_t min = enum_type->min_value, max = enum_type->max_value;
    int64_t range = max - min + 1;
    if (range <= 0 || range > 4096) LOWER_NOT_IMPLEMENTED(L, ast_enum, "names of enums with a value range above 4096");

    int table = ptr_map_get(&L->const_globals, enum_type);
    if (table < 0) {
        // One string per value in [min, max], holes get "?"
        IrGlobal *names = ir_global_data(L->module, "enum_value_names", NULL, (int)(range * 16), 8, true);
        int len = 0;
        int unknown = string_data_global(L, "?", 1, &len);
        for (int64_t v = 0; v < range; v++) {
            IrReloc reloc = { (int)(v * 16), unknown };
            int name_len = 1;
            for (int i = 0; i < ast_enum->enumerators.count; i++) {
                AstEnumerator *etor = da_get_deref(ast_enum->enumerators, i);
                if (etor->value == min + v) {
                    reloc.target = string_data_global(L, etor->name, (int) strlen(etor->name), &name_len);
                    break;
                }
            }
            ir_array_push(names->relocs, reloc);
            write_int_bytes(&names->data[v * 16 + 8], name_len, 8);
        }
        table = names->index;
        ptr_map_put(&L->const_globals, enum_type, table);
    }

    Type *backing = enum_type->backing_type;
    int value64 = convert_scalar(L, value, backing, primitive_type(PRIMITIVE_I64));
    int index   = emit_binop(L, IR_SUB, IR_I64, value64, emit_const(L, IR_I64, min));
    int in_range = emit_cmp(L, IR_ICMP, IR_CMP_ULT, IR_I64, index, emit_const(L, IR_I64, range));
    int entry   = emit_ptr_add(L, emit_global_addr(L, table), emit_binop(L, IR_MUL, IR_I64, index, emit_const(L, IR_I64, 16)));
    int invalid = emit_global_addr(L, string_global_raw(L, "<invalid>"));
    return emit_select(L, in_range, entry, invalid);
}

static int string_arg(Lowerer *L, const char *text) {
    // Strings are passed to C functions by reference
    return emit_global_addr(L, string_global_raw(L, text));
}

// Builds the runtime type descriptor (a *Type, see runtime.sd) for a value of 'type' stored at 'value_ptr'.
// Struct and enum descriptors describe the specific value (member value pointers, the enum value's name)
static int type_descriptor(Lowerer *L, Type *type, int value_ptr, Ast *site) {
    switch (type->kind) {
    case TYPE_VOID:
    case TYPE_BOOL:
    case TYPE_INTEGER:
    case TYPE_FLOAT: {
        char name[64];
        snprintf(name, sizeof(name), "Type_%s", ((TypePrimitive *) type)->name);
        if (type_is_untyped_int(type))   snprintf(name, sizeof(name), "Type_untyped_int");
        if (type_is_untyped_float(type)) snprintf(name, sizeof(name), "Type_untyped_float");
        return emit_load(L, IR_PTR, addr_global(ir_find_or_add_extern(L->module, name), 0));
    }
    case TYPE_STRING:
        return emit_load(L, IR_PTR, addr_global(ir_find_or_add_extern(L->module, "Type_string"), 0));
    case TYPE_ANY:
        return emit_c_call(L, "runtime_get_type_any", IR_PTR, 0, NULL);
    case TYPE_POINTER: {
        Type *pointee = ((TypePointer *) type)->pointer_to;
        int pointee_value = emit_load(L, IR_PTR, addr_vreg(value_ptr, 0));
        int args[2];
        args[0] = string_arg(L, type_to_str(type));
        args[1] = pointee->kind == TYPE_VOID ? emit_load(L, IR_PTR, addr_global(ir_find_or_add_extern(L->module, "Type_void"), 0))
                                             : type_descriptor(L, pointee, pointee_value, site);
        return emit_c_call(L, "runtime_get_type_pointer", IR_PTR, 2, args);
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *) type;
        int args[4];
        args[0] = string_arg(L, type_to_str(type));
        args[1] = emit_const(L, IR_I32, array->array_kind);
        args[2] = type_descriptor(L, array->elem_type, value_ptr, site);
        if (array->array_kind == ARRAY_FIXED) args[3] = emit_const(L, IR_I64, fixed_array_count(array));
        else                                  args[3] = emit_load(L, IR_I64, addr_vreg(value_ptr, 8));
        return emit_c_call(L, "runtime_get_type_array", IR_PTR, 4, args);
    }
    case TYPE_ENUM: {
        TypeEnum *enum_type = (TypeEnum *) type;
        int value = emit_load(L, ir_type_of(type), addr_vreg(value_ptr, 0));
        int args[5];
        args[0] = string_arg(L, enum_type->identifier->name);
        args[1] = enum_value_name(L, value, enum_type);
        args[2] = type_descriptor(L, enum_type->backing_type, value_ptr, site);
        args[3] = emit_const(L, IR_I32, enum_type->min_value);
        args[4] = emit_const(L, IR_I32, enum_type->max_value);
        return emit_c_call(L, "runtime_get_type_enum", IR_PTR, 5, args);
    }
    case TYPE_STRUCT: {
        TypeStruct *struct_type = (TypeStruct *) type;
        DynamicArray members = struct_type->node->members->identifiers;

        // StructMember :: struct { name: string; type: *Type; value: *void; index: i64; offset: i64; }   (48 bytes)
        int count = 0;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (!(member->flags & IDENTIFIER_IS_CONSTANT)) count++;
        }
        int size = emit_const(L, IR_I64, count > 0 ? count * 48 : 1);
        int array = emit_c_call(L, "malloc", IR_PTR, 1, &size);

        int entry_index = 0;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;

            int member_value = emit_ptr_add(L, value_ptr, emit_const(L, IR_I64, member->member_offset));
            int member_type  = type_descriptor(L, member->type, member_value, site);
            IrAddr entry = addr_vreg(array, entry_index * 48);

            int name_len = 0;
            int name = string_data_global(L, member->name, (int) strlen(member->name), &name_len);
            emit_store(L, IR_PTR, entry, emit_global_addr(L, name));
            emit_store(L, IR_I64, addr_add(entry, 8),  emit_const(L, IR_I64, name_len));
            emit_store(L, IR_PTR, addr_add(entry, 16), member_type);
            emit_store(L, IR_PTR, addr_add(entry, 24), member_value);
            emit_store(L, IR_I64, addr_add(entry, 32), emit_const(L, IR_I64, member->member_index));
            emit_store(L, IR_I64, addr_add(entry, 40), emit_const(L, IR_I64, member->member_offset));
            entry_index++;
        }

        int args[5];
        args[0] = string_arg(L, struct_type->identifier->name);
        args[1] = array;
        args[2] = emit_const(L, IR_I32, count);
        args[3] = emit_const(L, IR_I32, type->size);
        args[4] = emit_const(L, IR_I32, struct_type->alignment);
        return emit_c_call(L, "runtime_get_type_struct", IR_PTR, 5, args);
    }
    default:
        lower_fail(L, site, "Not implemented in the IR backend yet: runtime type information for %s", type_to_str(type));
    }
}

// Boxes a value into an 'any': { data: *void (pointer to a copy of the value), type: *Type }
static Value box_to_any(Lowerer *L, Value value, Type *any_type, Ast *site) {
    Type *from = value.type;
    Value boxed;

    if (is_fixed_array(from)) {
        // Fixed arrays are boxed as a pointer to their elements (like the runtime expects)
        boxed = temp_place(L, (Type *) type_defn_null_ptr, "boxed array pointer");
        emit_store(L, IR_PTR, boxed.addr, addr_to_ptr(L, value_to_place(L, value).addr));
    } else {
        boxed = temp_place(L, from, "boxed value");
        store_value(L, boxed.addr, from, value, site);
    }

    int data = addr_to_ptr(L, boxed.addr);
    int type = type_descriptor(L, from, data, site);

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

typedef IrArray(int) VregList;

// Appends the printf format for one value and its arguments (structs expand to one argument per member)
static void print_value(Lowerer *L, StringBuilder *format, VregList *args, Value value, Type *type, Ast *site) {
    switch (type->kind) {
    case TYPE_BOOL: {
        int cond = value_to_scalar(L, value);
        int true_str  = emit_global_addr(L, string_data_global_cstr(L, "true"));
        int false_str = emit_global_addr(L, string_data_global_cstr(L, "false"));
        sb_append(format, "%%s");
        int arg = emit_select(L, cond, true_str, false_str);
        ir_array_push(*args, arg);
        return;
    }
    case TYPE_INTEGER: {
        int v = value_to_scalar(L, value);
        if (!type_is_untyped_int(type) && ((TypePrimitive *) type)->kind == PRIMITIVE_CHAR) {
            sb_append(format, "%%c");
            ir_array_push(*args, convert_scalar(L, v, type, primitive_type(PRIMITIVE_U32)));
            return;
        }
        bool is_signed = type_is_signed(type);
        sb_append(format, is_signed ? "%%lld" : "%%llu");
        ir_array_push(*args, convert_scalar(L, v, type, primitive_type(is_signed ? PRIMITIVE_I64 : PRIMITIVE_U64)));
        return;
    }
    case TYPE_FLOAT: {
        int v = value_to_scalar(L, value);
        sb_append(format, "%%f");
        if (ir_vreg_type(L->func, v) == IR_F32) v = emit_unop(L, IR_FPEXT, IR_F64, v);
        ir_array_push(*args, v);
        return;
    }
    case TYPE_STRING: {
        // Saad strings are not NUL terminated, so print them with an explicit length
        Value str = value_to_place(L, value);
        sb_append(format, "%%.*s");
        int len = emit_load(L, IR_I64, addr_add(str.addr, 8));
        ir_array_push(*args, emit_unop(L, IR_TRUNC, IR_I32, len));
        ir_array_push(*args, emit_load(L, IR_PTR, str.addr));
        return;
    }
    case TYPE_ENUM: {
        int name = enum_value_name(L, value_to_scalar(L, value), (TypeEnum *) type);
        sb_append(format, "%%.*s");
        ir_array_push(*args, emit_unop(L, IR_TRUNC, IR_I32, emit_load(L, IR_I64, addr_vreg(name, 8))));
        ir_array_push(*args, emit_load(L, IR_PTR, addr_vreg(name, 0)));
        return;
    }
    case TYPE_POINTER:
    case TYPE_FUNCTION:
        sb_append(format, "0x%%p");
        ir_array_push(*args, value_to_scalar(L, value));
        return;
    case TYPE_ARRAY:
    case TYPE_ANY:
        sb_append(format, "0x%%p");
        ir_array_push(*args, addr_to_ptr(L, value_to_place(L, value).addr));
        return;
    case TYPE_STRUCT: {
        TypeStruct *struct_type = (TypeStruct *) type;
        Value place = value_to_place(L, value);
        DynamicArray members = struct_type->node->members->identifiers;
        sb_append(format, "%s{ ", struct_type->identifier->name);
        bool first = true;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (member->flags & IDENTIFIER_IS_CONSTANT) continue;
            if (!first) sb_append(format, ", ");
            first = false;
            sb_append(format, "%s = ", member->name);
            print_value(L, format, args, value_place(addr_add(place.addr, member->member_offset), member->type), member->type, site);
        }
        sb_append(format, " }");
        return;
    }
    default:
        lower_fail(L, site, "Not implemented in the IR backend yet: printing values of type %s", type_to_str(type));
    }
}

static void lower_print(Lowerer *L, AstPrint *print) {
    AstLiteral *format_lit = da_get_deref(print->arguments, 0);
    const char *text = format_lit->as.value.string.data;
    int text_len = (int) format_lit->as.value.string.length;

    StringBuilder format = sb_init(64);
    VregList args = {0};

    int arg_index = 1;
    for (int i = 0; i < text_len; i++) {
        if (text[i] == '%' && arg_index < print->arguments.count) {
            AstExpr *arg = da_get_deref(print->arguments, arg_index++);
            print_value(L, &format, &args, lower_expr(L, arg), arg->type, (Ast *) arg);
        } else {
            char c[2] = { text[i], 0 };
            sb_append(&format, "%s", c);
        }
    }

    int len = 0;
    int format_global = string_data_global(L, format.buffer, (int) format.cursor, &len);

    int callee = ir_find_or_add_extern(L->module, "printf");
    IrCall *call = new_call(L, IR_CALLEE_DIRECT, callee, IR_I32, args.count + 1);
    call_add_arg(call, emit_global_addr(L, format_global), false);
    for (int i = 0; i < args.count; i++) call_add_arg(call, args.items[i], true);
    emit_call(L, call);

    sb_free(&format);
    free(args.items);
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

static Value lower_append_call(Lowerer *L, AstFunctionCall *call) {
    // runtime_builtin_append(arr: *RawDynamicArray, elem: *void, byValue: bool)
    // The typer already turned the first argument into an address-of. We always pass the element
    // by reference (a pointer to a copy of it), which works for every element type.
    AstExpr *array_ptr = da_get_deref(call->lowered_arguments, 0);
    AstExpr *item      = da_get_deref(call->lowered_arguments, 1);

    TypeArray *array_type = (TypeArray *) ((TypePointer *) array_ptr->type)->pointer_to;
    Type      *elem_type  = array_type->elem_type;

    int args[3];
    args[0] = value_to_scalar(L, lower_expr(L, array_ptr));

    Value item_copy = temp_place(L, elem_type, "append item");
    store_expr(L, item_copy.addr, elem_type, item, true);
    args[1] = addr_to_ptr(L, item_copy.addr);
    args[2] = emit_const(L, IR_I8, 0);

    emit_c_call(L, "runtime_builtin_append", IR_VOID, 3, args);
    return value_none();
}

static Value lower_call(Lowerer *L, AstFunctionCall *call, int *ret_slots, int max_ret_slots) {
    AstFunctionDefn *func_defn = call->func_defn;
    if (func_defn == NULL) lower_fail(L, (Ast *) call, "Compiler Error: Call without a resolved function");

    if (func_defn->is_extern && strcmp(func_defn->symbol_name, "runtime_builtin_append") == 0) {
        return lower_append_call(L, call);
    }

    bool is_c = func_defn->is_extern || func_defn->calling_convention == CALLING_CONV_MSVC;

    // Callee (evaluated before the arguments)
    IrCalleeKind callee_kind;
    int callee;
    if (call_is_direct(call)) {
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
    IrType ir_return_type = IR_VOID;
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

        call_add_arg(ir_call, lower_argument(L, arg, param_type, is_vararg), is_vararg);
    }

    int result = emit_call(L, ir_call);

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
        Value place = temp_place(L, expr->type, "array literal");
        lower_array_literal_into(L, (AstArrayLiteral *) expr, place.addr);
        return place;
    }
    case AST_SIZEOF: {
        AstSizeof *ast_sizeof = (AstSizeof *) expr;
        return value_scalar(emit_const(L, ir_type_of(expr->type), ast_sizeof->type->size), expr->type);
    }
    case AST_NEW: {
        AstNew *ast_new = (AstNew *) expr;
        Type *value_type = ast_new->expr->type;
        int size = emit_const(L, IR_I64, value_type->size > 0 ? value_type->size : 1);
        int ptr = emit_c_call(L, "malloc", IR_PTR, 1, &size);
        store_expr(L, addr_vreg(ptr, 0), value_type, ast_new->expr, true);
        return value_scalar(ptr, expr->type);
    }
    case AST_SEMICOLON_EXPR:
        return value_none();
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
    default: LOWER_NOT_IMPLEMENTED(L, assign, "this assignment operator");
    }

    // The place is evaluated once, e.g `a[f()] += 1` only calls f once
    int current = value_to_scalar(L, lhs);
    int rhs     = lower_scalar_as(L, assign->expr, type);
    int result  = emit_arithmetic(L, op, type, current, rhs, (Ast *) assign);
    emit_store(L, ir_type_of(type), lhs.addr, result);
}

static void lower_assert(Lowerer *L, AstAssert *assertion) {
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
    LoopTargets targets = { loop, break_block, continue_block };
    ir_array_push(L->loops, targets);
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

        if (value->type->kind == TYPE_TUPLE) {
            LOWER_NOT_IMPLEMENTED(L, value, "returning the result of a multi-value call directly");
        }
        if (i >= func_defn->lowered_return_params.count) {
            lower_fail(L, (Ast *) value, "Compiler Error: More return values than return parameters");
        }

        AstIdentifier *ret_param = da_get_deref(func_defn->lowered_return_params, i);
        Type *ret_type = da_get_deref(func_defn->return_types, i);
        int out_ptr = emit_load(L, IR_PTR, addr_slot(local_slot(L, ret_param), 0));
        store_expr(L, addr_vreg(out_ptr, 0), ret_type, value, false);
    }

    terminate_ret(L, IR_NO_VREG);
    start_unreachable_block(L);
}

static void lower_statement(Lowerer *L, Ast *stmt) {
    L->line = stmt->start.line;

    switch (stmt->kind) {
    case AST_DECLARATION:       lower_declaration(L, (AstDeclaration *) stmt); return;
    case AST_ASSIGNMENT:        lower_assignment(L, (AstAssignment *) stmt); return;
    case AST_EXPR_STMT:         lower_expr(L, ((AstExprStmt *) stmt)->expr); return;
    case AST_BLOCK:             lower_block(L, (AstBlock *) stmt); return;
    case AST_ASSERT:            lower_assert(L, (AstAssert *) stmt); return;
    case AST_IF:                lower_if(L, (AstIf *) stmt); return;
    case AST_WHILE:             lower_while(L, (AstWhile *) stmt); return;
    case AST_FOR:               lower_for(L, (AstFor *) stmt); return;
    case AST_BREAK_OR_CONTINUE: lower_break_or_continue(L, (AstBreakOrContinue *) stmt); return;
    case AST_RETURN:            lower_return(L, (AstReturn *) stmt); return;

    // Declarations that produce no code at the point where they appear
    case AST_STRUCT:
    case AST_ENUM:
    case AST_IMPORT:
    case AST_EXTERN:
    case AST_DIRECTIVE:
    case AST_FUNCTION_DEFN:
    case AST_SEMICOLON_EXPR:
        return;

    case AST_PRINT:         lower_print(L, (AstPrint *) stmt); return;
    default:
        lower_fail(L, stmt, "Not implemented in the IR backend yet: statement '%s'", ast_to_str(stmt));
    }
}

static void lower_block(Lowerer *L, AstBlock *block) {
    for (int i = 0; i < block->statements.count; i++) {
        lower_statement(L, da_get_deref(block->statements, i));
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// Functions and the module
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct PendingFunctions {
    IrArray(AstFunctionDefn *) list;
    PtrMap queued;      // AstFunctionDefn* -> global index
} PendingFunctions;

static PendingFunctions pending_functions; // Reset for every module

// The global of a function symbol. Saad functions get queued for lowering the first time they are referenced
static int function_global(Lowerer *L, AstFunctionDefn *func_defn) {
    int global = ptr_map_get(&pending_functions.queued, func_defn);
    if (global >= 0) return global;

    char *name = func_defn->symbol_name ? func_defn->symbol_name : func_defn->identifier->name;

    if (func_defn->is_extern) {
        global = ir_find_or_add_extern(L->module, name);
    } else {
        // Different functions can share a name (e.g inner functions in different scopes), so a Saad
        // function whose name is already taken gets a unique symbol
        char unique[256];
        snprintf(unique, sizeof(unique), "%s", name);
        for (int n = 2; ; n++) {
            bool taken = false;
            for (int i = 0; i < L->module->globals.count; i++) {
                if (strcmp(L->module->globals.items[i]->name, unique) == 0) { taken = true; break; }
            }
            if (!taken) break;
            snprintf(unique, sizeof(unique), "%s__%d", name, n);
        }
        global = ir_global_new(L->module, IR_GLOBAL_EXTERN, unique)->index; // Becomes a FUNCTION when lowered
        ir_array_push(pending_functions.list, func_defn);
    }

    ptr_map_put(&pending_functions.queued, func_defn, global);
    return global;
}

static void lower_function(Lowerer *L, AstFunctionDefn *func_defn) {
    int global = ptr_map_get(&pending_functions.queued, func_defn);
    IrFunction *func = ir_function_new(L->module, L->module->globals.items[global]->name);
    func->is_entry_point = strcmp(func_defn->symbol_name, "main") == 0 && func_defn->parent_function == NULL;

    L->func     = func;
    L->ast_func = func_defn;
    L->line     = func_defn->head.head.start.line;
    L->loops.count = 0;
    ptr_map_clear(&L->locals);

    IrBlock *entry = create_block(L, "entry");
    set_block(L, entry);

    // Parameters arrive in vregs and are moved into their home slots right away
    for (int i = 0; i < func_defn->lowered_params.count; i++) {
        AstIdentifier *param = resolve_override(da_get_deref(func_defn->lowered_params, i));
        Type *type = param->type;

        int vreg = ir_new_vreg(func, ir_param_type(type));
        ir_array_push(func->params, vreg);

        int slot = declare_local(L, param);
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
    terminate_ret(L, IR_NO_VREG);

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

    function_global(L, ((TypeFunction *) main_ident->type)->node);

    for (int i = 0; i < pending_functions.list.count; i++) {
        lower_function(L, pending_functions.list.items[i]);
    }

    return L->module;
}
