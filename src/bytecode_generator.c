#include "typer.c"

#define SHOW_IR_LINE_NUMBERS 1
#define SHOW_IR_LIVE_INTERVALS 1
#define SHOW_IR_DEF_USE 0

typedef struct BasicBlock BasicBlock;

typedef enum TerminatorKind {
    TERMINATOR_NONE,
    TERMINATOR_JUMP,
    TERMINATOR_COND_JUMP,
    TERMINATOR_RETURN,
} TerminatorKind;

typedef struct Terminator {
    TerminatorKind kind;
    int            condition_reg;
    int            index;
    u8             target_count;
    union {
        struct {
            BasicBlock    *target1;
            BasicBlock    *target2;
        };
        BasicBlock *targets[2];
    };
} Terminator;

typedef struct BasicBlock {
    int          id;
    char        *name;
    DynamicArray instructions;    // of Inst.
    Terminator   terminator;

    DynamicArray predeccessors; // of *BasicBlock.

    // Stuff for liveness analysis
    DynamicArray usages;   // of int
    DynamicArray defines;  // of int
    DynamicArray live_in;  // of int
    DynamicArray live_out; // of int

} BasicBlock;

typedef struct BytecodeFunctionParameter {
    int   vreg;
    StackSlot stack_slot;
    Type *type;
} BytecodeFunctionParameter;

typedef struct LiveInterval {
    int vreg;
    int start;
    int end;
    int assigned_reg; // -1 = spilled
    StackSlot spill_slot;
    bool is_active;
    bool is_sse;
    bool has_reg;
    bool crosses_call; // Weather it crosses a function call
    bool has_gone_through_register_allocation;
} LiveInterval;

typedef struct BytecodeFunction {
    char        *symbol_name;
    BasicBlock  *entry;
    DynamicArray basic_blocks; // of *BasicBlock.
    int          next_vreg;
    int          next_instruction_index;
    int          next_stack_slot;

    DynamicArray stack_slots; // of StackSlot
    DynamicArray params; // of BytecodeFunctionParameter
    bool         is_extern;

    // Stuff for liveness analysis
    DynamicArray live_intervals; // of LiveInterval

    // Emission stage
    int          local_stack_size;
    int          temporary_stack_size;
    int          base_ptr;
    int          temp_ptr;

    DynamicArray vreg_to_stack_slot; // of int
    DynamicArray vreg_is_sse;        // of bool
    DynamicArray vreg_to_type;       // of *Type
    DynamicArray used_gpr_callee_saved_registers; // of Register
    DynamicArray used_sse_callee_saved_registers; // of Register


} BytecodeFunction;

typedef struct CallArgument {
    int   vreg;
    bool  is_vararg;
    Type *type;
} CallArgument;

typedef struct InstFunctionCall {
    char *function_symbol; // in case of a named function

    DynamicArray arguments;   // of CallArgument
    int          ret_vreg;
} InstFunctionCall;

typedef enum ConstantKind {
    CONSTANT_NONE,
    CONSTANT_STRING,
    CONSTANT_FLOAT,
} ConstantKind;

typedef struct Constant {
    ConstantKind kind;
    Type        *type;
    int          id;
    union {
        String value_string;
        double value_float;
    } as;
} Constant;

typedef struct ConstantPool {
    DynamicArray constants; // of Constant
} ConstantPool;

typedef enum IrValueKind {
    IR_VALUE_INVALID,
    IR_VALUE_RVALUE,
    IR_VALUE_LVALUE_STATIC,
    IR_VALUE_LVALUE_DYNAMIC,
} IrValueKind;

typedef struct IrValue {
    IrValueKind kind;
    int   vreg;
    int   slot_index;
    int   offset;
    Type *type;
} IrValue;

static IrValue InvalidValue = {
    .vreg = -1,
    .type = NULL,
};

typedef enum BytecodeGeneratorFlags {
    BCG_FLAG_EMIT_EXPRESSION_AS_LVALUE = 1 << 0,
} BytecodeGeneratorFlags;

typedef struct BytecodeGenerator {
    Parser *parser;

    int current_basic_block_id;

    Arena        bytecode_arena;
    ConstantPool constant_pool;

    DynamicArray bytecode_functions;    // of *ByteCodeFunction.

    BasicBlock        *current_basic_block;
    BytecodeFunction  *current_function;

    // This string builder must never resize, as we rely on stable pointers to null terminated strings inside it
    StringBuilder temporary_strings;

    AstFile         *current_file;
    AstBlock        *current_scope;
    AstFunctionDefn *enclosing_function;  // Current enclosing function

    bool optimization_use_ir_values_for_identifiers;
    BytecodeGeneratorFlags flags;
} BytecodeGenerator;

typedef enum Register {
    REG_NONE,

    REG_RAX,
    REG_RBX,
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
    REG_RSI,
    REG_RDI,
    REG_RBP,
    REG_RSP,

    REG_XMM0,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7,
    REG_XMM8,
    REG_XMM9,
    REG_XMM10,
    REG_XMM11,
    REG_XMM12,
    REG_XMM13,
    REG_XMM14,
    REG_XMM15,

    REG_COUNT,
} Register;

// static Register x64_caller_saved_registers_gpr[] = {REG_RAX, REG_RCX, REG_RDX, REG_R8, REG_R9, REG_R10, REG_R11};
// static int      x64_caller_saved_registers_gpr_count = 7;

// static Register x64_caller_saved_registers_sse[] = {REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, REG_XMM4, REG_XMM5};
// static int      x64_caller_saved_registers_sse_count = 6;

static Register x64_callee_saved_registers_gpr[] = {REG_RBX, REG_R12, REG_R13, REG_R14, REG_R15};
static int      x64_callee_saved_registers_gpr_count = 5;

static Register x64_callee_saved_registers_sse[] = {REG_XMM6, REG_XMM7, REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11, REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15};
static int      x64_callee_saved_registers_sse_count = 10;

static Register x64_usable_registers_sse[] = {REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, REG_XMM4, REG_XMM6, REG_XMM7, REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11, REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15};
static int      x64_usable_registers_sse_count = 15;

static Register x64_usable_registers_gpr[] = {REG_RAX, REG_RCX, REG_RDX, REG_R8, REG_R9, REG_R10, REG_RBX, REG_R12, REG_R13, REG_R14, REG_R15};
static int      x64_usable_registers_gpr_count = 11;

static Register x64_scratch_registers_gpr[] = {REG_R11};
static Register x64_scratch_registers_sse[] = {REG_XMM5};

char *gpr_register_names[17][4] = {
    { "non", "non",  "non",  "non"  },
    { "rax", "eax",  "ax",   "al"   },
    { "rbx", "ebx",  "bx",   "bl"   },
    { "rcx", "ecx",  "cx",   "cl"   },
    { "rdx", "edx",  "dx",   "dl"   },
    { "r8",  "r8d",  "r8w",  "r8b"  },
    { "r9",  "r9d",  "r9w",  "r9b"  },
    { "r10", "r10d", "r10w", "r10b" },
    { "r11", "r11d", "r11w", "r11b" },
    { "r12", "r12d", "r12w", "r12b" },
    { "r13", "r13d", "r13w", "r13b" },
    { "r14", "r14d", "r14w", "r14b" },
    { "r15", "r15d", "r15w", "r15b" },
    { "rsi", "esi",  "si",   "sil"  },
    { "rdi", "edi",  "di",   "dil"  },
    { "rbp", "ebp",  "bp",   "bpl"  },
    { "rsp", "esp",  "sp",   "spl"  },
};

char *sse_register_names[16] = {
    "xmm0",
    "xmm1",
    "xmm2",
    "xmm3",
    "xmm4",
    "xmm5",
    "xmm6",
    "xmm7",
    "xmm8",
    "xmm9",
    "xmm10",
    "xmm11",
    "xmm12",
    "xmm13",
    "xmm14",
    "xmm15",
};

char *register_to_str(Register reg, int width) {
    if (width > 8) width = 8;

    if (reg <= REG_R15) {
        int i = -1;
        if (width == 8) i = 0;
        if (width == 4) i = 1;
        if (width == 2) i = 2;
        if (width == 1) i = 3;
        if (i == -1) {
            assert(false && "Got unexpected register width");
        }

        return gpr_register_names[reg][i];
    } 
    else if (reg <= REG_XMM15) {
        int i = reg - REG_XMM0;
        return sse_register_names[i];
    }
    else {
        XXX;
    }
}

typedef enum OperandKind {
    OPERAND_NONE,
    OPERAND_IMM_INT,        // Immediate value:  ADD r0, 42
    OPERAND_IMM_UINT,       // Immediate value:  ADD r0, 42
    OPERAND_BIG_CONSTANT,   // Constant value
    OPERAND_REG,            // Virtual register: ADD r0, r1
    OPERAND_MEMORY_SLOT,    // Memory slot that will be translated to an offset relative to rbp
    OPERAND_MEMORY_OFFSET,  // Memory offset relative to rbp
    OPERAND_MEMORY_REL_OFFSET, // Memory relative offset from a stack slot
    OPERAND_LABEL,          // Branch label
    OPEARND_LABEL_ID,       // Branch label id only
} OperandKind;

typedef enum InstKind {
    INST_NOOP,

    INST_COMMENT,

    INST_MOV,
    INST_LOAD,
    INST_STORE,
    INST_LEA,
    INST_PUSH,
    INST_POP,
    INST_COND_JUMP,
    INST_LABEL,
    INST_CALL,
    INST_MEMSET,
    INST_MEMCPY,
    INST_MALLOC,
    INST_CALLOC,
    INST_RETURN,
    INST_DEREF,

    // Boolean operators arithmetic
    INST_LOGICAL_OR,
    INST_LOGICAL_AND,
    INST_DOUBLE_EQUAL,
    INST_NOT_EQUAL,
    INST_NOT,
    INST_GREATER_THAN,
    INST_LESS_THAN,
    INST_GREATER_THAN_EQUAL,
    INST_LESS_THAN_EQUAL,

    // Integer arithmetic
    INST_ADD_INT,
    INST_SUB_INT,
    INST_MUL_INT,
    INST_DIV_INT,

    // Float arithmetic
    INST_ADD_FLOAT,
    INST_SUB_FLOAT,
    INST_MUL_FLOAT,
    INST_DIV_FLOAT,

    // ------------- Casts -------------
    // integer width changes
    INST_SEXT,     // sign-extend: i32 -> i64 (signed source)
    INST_ZEXT,     // zero-extend: u32 -> u64, or bool -> anything
    INST_TRUNC,    // truncate: i64 -> i32 (just take low bits)

    // int <-> float
    INST_SITOFP,   // signed int -> float
    INST_UITOFP,   // unsigned int -> float
    INST_FPTOSI,   // float -> signed int (truncating toward zero)
    INST_FPTOUI,   // float -> unsigned int

    // float width changes
    INST_FPEXT,    // f32 -> f64
    INST_FPTRUNC,  // f64 -> f32

    // no-op / reinterpret
    INST_BITCAST,  // same size, reinterpret bits (e.g. i32 <-> f32, ptr <-> u64)
    // ----------------------------
    
} InstKind;

char *inst_kind_to_string(InstKind kind) {
    switch (kind) {
    case INST_NOOP:                 return "noop";
    case INST_COMMENT:              return "comment";
    case INST_MOV:                  return "mov";
    case INST_LOAD:                 return "load";
    case INST_STORE:                return "store";
    case INST_LEA:                  return "lea";
    case INST_PUSH:                 return "push";
    case INST_POP:                  return "pop";
    case INST_COND_JUMP:            return "jmpif";
    case INST_LABEL:                return "label";
    case INST_CALL:                 return "call";
    case INST_MEMSET:               return "memset";
    case INST_MEMCPY:               return "memcpy";
    case INST_MALLOC:               return "malloc";
    case INST_CALLOC:               return "calloc";
    case INST_RETURN:               return "ret";
    case INST_DEREF:                return "deref";
    case INST_LOGICAL_OR:           return "or";
    case INST_LOGICAL_AND:          return "and";
    case INST_DOUBLE_EQUAL:         return "eq";
    case INST_NOT_EQUAL:            return "noteq";
    case INST_NOT:                  return "not";
    case INST_GREATER_THAN:         return "gt";
    case INST_LESS_THAN:            return "lt";
    case INST_GREATER_THAN_EQUAL:   return "gte";
    case INST_LESS_THAN_EQUAL:      return "lte";
    case INST_ADD_INT:              return "addi";
    case INST_SUB_INT:              return "subi";
    case INST_MUL_INT:              return "muli";
    case INST_DIV_INT:              return "divi";
    case INST_ADD_FLOAT:            return "addf";
    case INST_SUB_FLOAT:            return "subf";
    case INST_MUL_FLOAT:            return "mulf";
    case INST_DIV_FLOAT:            return "divf";
    case INST_SEXT:                 return "sext";
    case INST_ZEXT:                 return "zext";
    case INST_TRUNC:                return "trunc";
    case INST_SITOFP:               return "sitofp";
    case INST_UITOFP:               return "uitofp";
    case INST_FPTOSI:               return "fptosi";
    case INST_FPTOUI:               return "fptoui";
    case INST_FPEXT:                return "fpext";
    case INST_FPTRUNC:              return "fptrunc";
    case INST_BITCAST:              return "bitcast";
    }

    return "???";
}

char *operand_kind_to_string(OperandKind kind) {
    switch(kind) {
    case OPERAND_NONE: return "none";
    case OPERAND_IMM_INT: return "imm_int";
    case OPERAND_IMM_UINT: return "imm_uint";
    case OPERAND_BIG_CONSTANT: return "big_constant";
    case OPERAND_REG: return "reg";
    case OPERAND_MEMORY_SLOT: return "mem";
    case OPERAND_LABEL: return "label";
    case OPEARND_LABEL_ID: return "label_id";
    default: return "unknown";
    }
}

typedef enum OperandFlags {
    OPERAND_FLAG_IMM_UINT_IS_BOOLEAN = 1 << 0,
} OperandFlags;

typedef struct Operand {
    u8    kind;
    u8    size; // 1, 2, 4, 8 bytes in case of immediate values. 
    bool  is_sse;  // Weather this operand needs to be in SSE registers (xmm0, etc. ...) instead of general purpose registers
    u32   flags; // OperandFlags
    Type *type;
    StackSlot slot; // Memory slot (e.g. from rbp)
    union {
        i64    imm_int;       // Immediate signed integer value
        u64    imm_uint;      // Immediate unsigned integer value
        double imm_float;     // Immediate float value
        Constant *constant;   // Constant value. E.g strings, structs and arrays
        int    vreg;          // Virtual register index
        int    preg;          // Physical register index set after it has gone through IR rewrite
        int    offset;        // Memory offset (e.g. from rbp)
        char  *label;         // E.g function call or branch label
        int    label_id;      // E.g id of a block
    };
} Operand;

typedef struct Inst {
    InstKind kind;
    u8       op_count;
    int      index;
    void    *data;       // Pointer to arbitrary extra data the instruction might need
    union {
        struct {
            Operand op1;
            Operand op2;
            Operand op3;
        };
        Operand operands[3];
    };
} Inst;

void *bytecode_allocate(BytecodeGenerator *bcg, size_t size) {
    return arena_allocate(&bcg->bytecode_arena, size);
}

BytecodeFunction *new_bytecode_function(BytecodeGenerator *bcg, AstFunctionDefn *func_defn) {
    BytecodeFunction *func = bytecode_allocate(bcg, sizeof(BytecodeFunction));

    func->symbol_name = func_defn->symbol_name;
    func->basic_blocks = da_init(16, sizeof(BasicBlock *));
    func->params = da_init(func_defn->parameters.count, sizeof(BytecodeFunctionParameter));
    func->stack_slots = da_init(16, sizeof(StackSlot));
    func->vreg_to_stack_slot = da_init(128, sizeof(int));
    func->vreg_to_stack_slot.count = 128;
    func->vreg_is_sse = da_init(128, sizeof(int));
    func->vreg_to_type = da_init(128, sizeof(Type *));
    func->used_gpr_callee_saved_registers = da_init(8, sizeof(Register));
    func->used_sse_callee_saved_registers = da_init(16, sizeof(Register));
    func->is_extern = func_defn->is_extern;

    da_append(&bcg->bytecode_functions, func);
    bcg->current_function = func;

    return func;
}

BytecodeGenerator bytecode_generator_init(CompilerConfig *config, Parser *parser) {
    BytecodeGenerator bcg = {0};
    bcg.parser = parser;

    bcg.bytecode_arena = arena_init(8192);
    bcg.constant_pool.constants = da_init(64, sizeof(Constant));
    bcg.bytecode_functions = da_init(32, sizeof(BytecodeFunction*));
    bcg.temporary_strings = sb_init(40000);

    if (config->optimization_level == OPTIMIZATION_LEVEL_O1) {
        bcg.optimization_use_ir_values_for_identifiers = true;
    }

    return bcg;
}

void bcg_file(BytecodeGenerator *bcg, AstFile *code);
void bcg_block(BytecodeGenerator *bcg, AstBlock *block);
void bcg_statement(BytecodeGenerator *bcg, Ast *node);
IrValue bcg_binary(BytecodeGenerator *bcg, AstBinary *bin);
IrValue bcg_unary(BytecodeGenerator *bcg, AstUnary *unary);
IrValue bcg_function_call(BytecodeGenerator *bcg, AstFunctionCall *call);
void bcg_function_defn(BytecodeGenerator *bcg, AstFunctionDefn *func_defn);
void bcg_return(BytecodeGenerator *bcg, AstReturn *ast_return);
void bcg_print(BytecodeGenerator *bcg, AstPrint *print_stmt);
void bcg_assert(BytecodeGenerator *bcg, AstAssert *assertion);
void bcg_typeof(BytecodeGenerator *bcg, AstTypeof *ast_typeof);
void bcg_if(BytecodeGenerator *bcg, AstIf *ast_if, BasicBlock *merge);
void bcg_for(BytecodeGenerator *bcg, AstFor *ast_for);
void bcg_while(BytecodeGenerator *bcg, AstWhile *ast_while);
void bcg_break_or_continue(BytecodeGenerator *bcg, AstBreakOrContinue *boc);
void bcg_enum_defn(BytecodeGenerator *bcg, AstEnum *ast_enum);
void bcg_struct_defn(BytecodeGenerator *bcg, AstStruct *ast_struct);
IrValue bcg_array_literal(BytecodeGenerator *bcg, AstArrayLiteral *array_lit);
void bcg_struct_literal(BytecodeGenerator *bcg, AstStructLiteral *struct_lit, StackSlot base_slot, int base_offset);
void bcg_declaration(BytecodeGenerator *bcg, AstDeclaration *decl);
void bcg_assignment(BytecodeGenerator *bcg, AstAssignment *assign);
IrValue bcg_array_access(BytecodeGenerator *bcg, AstArrayAccess *array_ac, bool lvalue);
IrValue bcg_expression(BytecodeGenerator *bcg, AstExpr *expr);
IrValue bcg_lvalue_expression(BytecodeGenerator *bcg, AstExpr *expr);

#define REG(i)   ((Operand){ .kind = OPERAND_REG,       .vreg = (i) })
#define LABEL(s) ((Operand){ .kind = OPERAND_LABEL,     .label = (s) })
#define LABEL_ID(id) ((Operand){ .kind = OPEARND_LABEL_ID, .label_id = (id) })
#define NO_OP    ((Operand){ .kind = OPERAND_NONE })

void begin_bytecode_generation(BytecodeGenerator *bcg, AstFile *file) {
    for (int i = 0; i < file->flattened_function_defns.count; i++) {
        AstFunctionDefn *func_defn = da_get_deref(file->flattened_function_defns, i);
        bcg_function_defn(bcg, func_defn);
    }
}

IrValue make_value(int vreg, Type *type) {
    IrValue result = {0};
    result.kind = IR_VALUE_RVALUE;
    result.vreg = vreg;
    result.type = type;
    return result;
}

IrValue make_static_lvalue(int slot_index, int offset, Type *type) {
    IrValue result = {0};
    result.kind = IR_VALUE_LVALUE_STATIC;
    result.slot_index = slot_index;
    result.offset = offset;
    result.type = type;
    return result;
}

IrValue make_dynamic_lvalue(int vreg, int offset, Type *type) {
    IrValue result = {0};
    result.kind = IR_VALUE_LVALUE_DYNAMIC;
    result.vreg = vreg;
    result.offset = offset;
    result.type = type;
    return result;
}

IrValue fresh_register(BytecodeGenerator *bcg, Type *type) {
    int vreg = bcg->current_function->next_vreg++;

    bool is_sse = type->kind == TYPE_FLOAT;
    da_append(&bcg->current_function->vreg_is_sse, is_sse);
    da_append(&bcg->current_function->vreg_to_type, type);

    return make_value(vreg, type);
}

bool is_vreg_sse(BytecodeFunction *func, int vreg) {
    bool *slot = da_get(func->vreg_is_sse, vreg);
    return *slot;
}

void add_instruction(BytecodeGenerator *bcg, Inst inst) {
    da_append(&bcg->current_basic_block->instructions, inst);
}

int get_next_instruction_index(BytecodeGenerator *bcg) {
    return bcg->current_function->next_instruction_index++;
}

static Inst make_instruction_3(BytecodeGenerator *bcg, InstKind kind, Operand op1, Operand op2, Operand op3) {
    return (Inst) {
        .kind = kind,
        .index = get_next_instruction_index(bcg),
        .op_count = 3,
        .op1 = op1,
        .op2 = op2,
        .op3 = op3,
    };
}

static Inst make_instruction_2(BytecodeGenerator *bcg, InstKind kind, Operand op1, Operand op2) {
    return (Inst) {
        .kind = kind,
        .index = get_next_instruction_index(bcg),
        .op_count = 2,
        .op1 = op1,
        .op2 = op2,
        .op3 = NO_OP,
    };
}

static Inst make_instruction_1(BytecodeGenerator *bcg, InstKind kind, Operand op1) {
    return (Inst) {
        .kind = kind,
        .index = get_next_instruction_index(bcg),
        .op_count = 1,
        .op1 = op1,
        .op2 = NO_OP,
        .op3 = NO_OP,
    };
}

static Inst make_instruction_0(BytecodeGenerator *bcg, InstKind kind) {
    return (Inst) {
        .kind = kind,
        .index = get_next_instruction_index(bcg),
        .op_count = 0,
        .op1 = NO_OP,
        .op2 = NO_OP,
        .op3 = NO_OP
    };
}

void emit_instruction_3(BytecodeGenerator *bcg, InstKind inst_kind, Operand op1, Operand op2, Operand op3) {
    Inst inst = make_instruction_3(
        bcg,
        inst_kind,
        op1,
        op2,
        op3
    );

    add_instruction(bcg, inst);
}

void emit_instruction_2(BytecodeGenerator *bcg, InstKind inst_kind, Operand op1, Operand op2) {
    Inst inst = make_instruction_2(
        bcg,
        inst_kind,
        op1,
        op2
    );

    add_instruction(bcg, inst);
}

void emit_instruction_1(BytecodeGenerator *bcg, InstKind inst_kind, Operand op1) {
    Inst inst = make_instruction_1(
        bcg,
        inst_kind,
        op1
    );

    add_instruction(bcg, inst);
}

static Operand make_op_register(IrValue value) {
    return (Operand) {
        .kind = OPERAND_REG,
        .size = value.type->size,
        .vreg = value.vreg,
        .is_sse = value.type->kind == TYPE_FLOAT,
        .type = value.type,
    };
}

static Operand make_register_ex(int vreg, int size, bool is_sse, Type *type) {
    return (Operand) {
        .kind = OPERAND_REG,
        .size = size,
        .vreg = vreg,
        .is_sse = is_sse,
        .type = type,
    };
}

Operand make_immediate_uint(u64 value, int size, u32 flags) {
    return (Operand){
        .kind = OPERAND_IMM_UINT,
        .size = size,
        .imm_uint = value,
        .flags = flags,
    };
}

Operand make_immediate_int(i64 value, int size) {
    return (Operand){
        .kind = OPERAND_IMM_INT,
        .size = size,
        .imm_int = value,
    };
}

Operand make_constant_operand(Constant *constant) {
    return (Operand){
        .kind = OPERAND_BIG_CONSTANT,
        .constant = constant,
    };
}

static inline Operand make_op_memory_relative_offset(StackSlot base, int relative_offset, Type *type) {
    return (Operand) {
        .kind = OPERAND_MEMORY_REL_OFFSET,
        .slot = base,
        .offset = relative_offset,
        .type = type,
    };
}

// A fixed offset from rbp. E.g `%d[rbp]` 
static inline Operand make_op_memory_offset(int offset, Type *type) {
    return (Operand) {
        .kind = OPERAND_MEMORY_OFFSET,
        .offset = offset,
        .type = type,
    };
}

static inline Operand make_op_memory_slot(StackSlot slot) {
    return (Operand) {
        .kind = OPERAND_MEMORY_SLOT,
        .slot = slot,
    };
}

bool type_uses_sse(Type *type) {
    if (type == NULL) return false;
    return type->kind == TYPE_FLOAT;
}

void add_comment(BytecodeGenerator *bcg, char *comment) {
    Inst inst = make_instruction_0(bcg, INST_COMMENT);
    inst.data = comment;
    add_instruction(bcg, inst);
}

bool set_int_in(DynamicArray da, int value) {
    for (int i = 0; i < da.count; i++) {
        int *e = da_get(da, i);
        if (*e == value) {
            // Already exists in the set
            return true;
        }
    }

    return false;
}

void set_int_add(DynamicArray *da, int value) {
    if (set_int_in(*da, value)) {
        return;
    }

    da_append(da, value);
}

DynamicArray set_int_diff(DynamicArray set1, DynamicArray set2) {
    DynamicArray result = da_init(set1.count > set2.count ? set1.count : set2.count, sizeof(int));

    for (int i = 0; i < set1.count; i++) {
        int *el = da_get(set1, i);
        if (!set_int_in(set2, *el)) {
            da_append(&result, *el);
        }
    }

    return result;
}

DynamicArray set_int_union(DynamicArray set1, DynamicArray set2) {
    DynamicArray result = da_init(set1.count + set2.count, sizeof(int));

    for (int i = 0; i < set1.count; i++) {
        int *el = da_get(set1, i);
        set_int_add(&result, *el);
    }

    for (int i = 0; i < set2.count; i++) {
        int *el = da_get(set2, i);
        set_int_add(&result, *el);
    }

    return result;
}

void set_int_append_string_to_builder(StringBuilder *sb, DynamicArray set) {
    sb_append(sb, "{");
    for (int i = 0; i < set.count; i++) {
        int *el = da_get(set, i);
        sb_append(sb, "v%d", *el);
        if (i + 1 < set.count) {
            sb_append(sb, ",");
        }
    }
    sb_append(sb, "}");
}

void bcg_dump_operand(StringBuilder *sb, Operand op) {
    switch (op.kind) {
    case OPERAND_NONE: {
        sb_append(sb, "noop");
        return;
    }
    case OPERAND_IMM_INT: {
        sb_append(sb, "%lld", op.imm_int);
        return;
    }
    case OPERAND_IMM_UINT: {
        if (op.flags & OPERAND_FLAG_IMM_UINT_IS_BOOLEAN) {
            if (op.imm_uint) {
                sb_append(sb, "true");
            } else {
                sb_append(sb, "false");
            }
            return;
        }
        
        sb_append(sb, "%lld", op.imm_int);
        return;
    }
    case OPERAND_BIG_CONSTANT: {
        switch (op.constant->kind) {
        case CONSTANT_STRING: {
            sb_append(sb, "\"%s\"", op.constant->as.value_string.data);
            return;
        }
        case CONSTANT_FLOAT: {
            sb_append(sb, "%llf", op.constant->as.value_float);
            return;
        }
        default: {
            sb_append(sb, "constant %d", op.constant->id);
            return;
        }
        }
        return;
    }
    case OPERAND_REG: {
        sb_append(sb, "v%d", op.vreg);
        return;
    }
    case OPERAND_MEMORY_SLOT: {
        sb_append(sb, "[%d]", op.slot.index);
        return;
    }
    case OPERAND_MEMORY_OFFSET: {
        sb_append(sb, "%d[rbp]", op.offset);
        return;
    }
    case OPERAND_MEMORY_REL_OFFSET: {
        sb_append(sb, "rel %d[%d]", op.offset, op.slot.index);
        return;
    }
    case OPERAND_LABEL: {
        sb_append(sb, "%s", op.label);
        return;
    }
    }

    sb_append(sb, "???");
}

char *generate_function_call_string(BytecodeGenerator *bcg, InstFunctionCall *call) {
    char *str = &bcg->temporary_strings.buffer[bcg->temporary_strings.cursor];
    sb_append(&bcg->temporary_strings, "%s(", call->function_symbol);
    for (int i = 0; i < call->arguments.count; i++) {
        CallArgument *arg = da_get(call->arguments, i);

        sb_append(&bcg->temporary_strings, "v%d", arg->vreg);
        if (i + 1 < call->arguments.count) {
            sb_append(&bcg->temporary_strings, ",");
        }
    }
    sb_append(&bcg->temporary_strings, ")\0");

    return str;
}

void bcg_dump_instruction(BytecodeGenerator *bcg, StringBuilder *sb, Inst *inst) {

    if (inst->kind == INST_COMMENT) {
        sb_append(sb, "\t; %s\n", (char *) inst->data);
        return;
    }

    char *inst_kind_str = inst_kind_to_string(inst->kind);

    sb_append(sb, "\t");

    // Show instruction numbers?
    #if SHOW_IR_LINE_NUMBERS
        sb_append(sb, "%d: ",inst->index);
    #endif

    // Special case for calls
    if (inst->kind == INST_CALL) {
        InstFunctionCall *call = inst->data;
        char *function_name = generate_function_call_string(bcg, call);
        sb_append(sb, "call\t\tv%d, %s\n", inst->op1.vreg, function_name);
        return;
    }

    if (strlen(inst_kind_str) > 4) {
        sb_append(sb, "%s\t", inst_kind_str);
    } else {
        sb_append(sb, "%s\t\t", inst_kind_str);
    }

    for (int k = 0; k < inst->op_count; k++) {
        bcg_dump_operand(sb, inst->operands[k]);
        if (k + 1 != inst->op_count) {
            sb_append(sb, ", ");
        }
    }
    sb_append(sb, "\n");
}

void bcg_dump_block_liveness_info(StringBuilder *sb, BasicBlock *bb) {
    sb_append(sb, "; Def = ");
    set_int_append_string_to_builder(sb, bb->defines);
    sb_append(sb, "\n");


    sb_append(sb, "; Use = ");
    set_int_append_string_to_builder(sb, bb->usages);
    sb_append(sb, "\n");

    sb_append(sb, "; Live in = ");
    set_int_append_string_to_builder(sb, bb->live_in);
    sb_append(sb, "\n");

    sb_append(sb, "; Live out = ");
    set_int_append_string_to_builder(sb, bb->live_out);
    sb_append(sb, "\n");
}

void bcg_dump_live_intervals(StringBuilder *sb, BytecodeFunction *func) {

    sb_append(sb, "; Live-intervals\n");
    for (int i = 0; i < func->live_intervals.count; i++) {
        LiveInterval *interval = da_get(func->live_intervals, i);

        sb_append(sb, "; v%d = [%d, %d] %s", interval->vreg, interval->start, interval->end, interval->crosses_call ? "X" : "O");

        if (interval->has_gone_through_register_allocation) {
            sb_append(sb, ", ");
            if (interval->assigned_reg == -1) {
                sb_append(sb, "S[%d]", interval->spill_slot.index);
            } else {
                sb_append(sb, "p%d", interval->assigned_reg);
            }

            if (interval->assigned_reg != -1) {
                sb_append(sb, ", %s", register_to_str((Register) interval->assigned_reg, 8));
            }
        }


        sb_append(sb, "\n");
    }
}

void bcg_dump_function_defn(BytecodeGenerator *bcg, StringBuilder *sb, BytecodeFunction *func) {

    if (func->is_extern) {
        sb_append(sb, "extern %s\n", func->symbol_name);
        return;
    }

    sb_append(sb, "%s:\n", func->symbol_name);

    #if SHOW_IR_LIVE_INTERVALS
        bcg_dump_live_intervals(sb, func);
    #endif

    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        sb_append(sb, "\n");

        #if SHOW_IR_DEF_USE
            bcg_dump_block_liveness_info(sb, bb);
        #endif

        sb_append(sb, "L%d:\n", bb->id);
        for (int j = 0; j < bb->instructions.count; j++) {
            Inst *inst = da_get(bb->instructions, j);

            bcg_dump_instruction(bcg, sb, inst);
        }

        // Dump terminator
        sb_append(sb, "\t");

        #if SHOW_IR_LINE_NUMBERS
            sb_append(sb, "%d: ", bb->terminator.index);
        #endif

        switch (bb->terminator.kind) {
            case TERMINATOR_NONE: {
                sb_append(sb, "term_none\n");
                break;
            }
            case TERMINATOR_JUMP: {
                sb_append(sb, "jmp\t\tL%d\n", bb->terminator.target1->id);
                break;
            }
            case TERMINATOR_COND_JUMP: {
                sb_append(sb, "jmp_if\tv%d, L%d, L%d\n", bb->terminator.condition_reg, bb->terminator.target1->id, bb->terminator.target2->id);
                break;
            }
            case TERMINATOR_RETURN: {
                sb_append(sb, "ret\n");
                break;
            }
        }
    }
}

void bcg_dump_bytecode_to_file(BytecodeGenerator *bcg, const char *output_path) {
    StringBuilder sb = sb_init(1024);

    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
     
        bcg_dump_function_defn(bcg, &sb, func);
    }

    FILE *f = fopen(output_path, "w");
    if (f == NULL) {
        printf("%s:%d: error: Failed to open file '%s'\n", __FILE__, __LINE__, output_path);
        exit(1);
    }

    fwrite(sb.buffer, 1, sb.cursor, f);
}

void bcg_block(BytecodeGenerator *bcg, AstBlock *block) {
    for (int i = 0; i < block->statements.count; i++) {
        Ast *stmt = da_get_deref(block->statements, i);
        bcg_statement(bcg, stmt);
    }
}

StackSlot bc_allocate_stack_slot(BytecodeFunction *func, int size) {
    int slot_index = func->next_stack_slot;
    func->next_stack_slot += 1;

    StackSlot slot = {.index = slot_index, .size = size };
    da_append(&func->stack_slots, slot);

    return slot;
}

int bc_allocate_local(BytecodeGenerator *bcg, int size) {
    assert(bcg->current_function);

    bcg->current_function->base_ptr -= size;
    bcg->current_function->base_ptr = align_value(bcg->current_function->base_ptr, size > 8 ? 8 : size);

    if (bcg->current_function->base_ptr == 0) {
        report_error_ast(bcg->parser, LABEL_WARNING, NULL, "Compiler Error: Suspicious 0 local offset in bc_allocate_local");
    }

    return bcg->current_function->base_ptr;
}

Operand memory_value_to_operand(IrValue memory_value) {
    if (memory_value.kind == IR_VALUE_RVALUE) {
        return make_op_register(memory_value);
    }
    else if (memory_value.kind == IR_VALUE_LVALUE_STATIC) {
        StackSlot slot = (StackSlot){
            .index = memory_value.slot_index,
            .size = memory_value.type->size,
        };
        return make_op_memory_relative_offset(slot, memory_value.offset, memory_value.type);
    } 
    else if (memory_value.kind == IR_VALUE_LVALUE_DYNAMIC) {
        XXX;
        return NO_OP;
    }
    else {
        XXX;
        return NO_OP;
    }
}

IrValue bcg_emit_load(BytecodeGenerator *bcg, IrValue src) {
    IrValue dst = fresh_register(bcg, src.type);

    Operand src_op = memory_value_to_operand(src);

    emit_instruction_2(
        bcg,
        INST_LOAD,
        make_op_register(dst),
        src_op
    );

    return dst;
}

IrValue bcg_emit_lea(BytecodeGenerator *bcg, IrValue src) {
    // TODO: type_defn_null_ptr looks weird. Should just be some kind of generic pointer type
    IrValue dst = fresh_register(bcg, (Type *)type_defn_null_ptr);

    Operand src_op = memory_value_to_operand(src);

    emit_instruction_2(bcg, INST_LEA, make_op_register(dst), src_op);

    return dst;
}

void bcg_declaration(BytecodeGenerator *bcg, AstDeclaration *decl) {

    if (decl->flags & DECLARATION_CONSTANT) {
        XXX;
    }

    if ((decl->flags & DECLARATION_INFER) || (decl->flags & DECLARATION_TYPED)) {
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = da_get_deref(decl->idents, i);
            AstExpr       *value = da_get_deref(decl->values, i);

            IrValue src = bcg_expression(bcg, value);

            if (bcg->optimization_use_ir_values_for_identifiers) {
                IrValue dst = fresh_register(bcg, ident->type);
                ident->virtual_register = dst.vreg;

                add_instruction(bcg, make_instruction_2(bcg,
                    INST_MOV, 
                    make_op_register(dst),
                    make_op_register(src)
                ));
            } else {
                ident->stack_slot = bc_allocate_stack_slot(bcg->current_function, ident->type->size);

                if (ident->type->size <= 8) {
                    add_instruction(bcg, make_instruction_2(bcg,
                        INST_STORE, 
                        make_op_memory_slot(ident->stack_slot),
                        make_op_register(src)
                    ));
                } else {
                    // IrValue src_pointer = bcg_emit_load(bcg, src);
                    IrValue dst = make_static_lvalue(ident->stack_slot.index, 0, ident->type);
                    IrValue dst_ptr = bcg_emit_lea(bcg, dst);

                    add_instruction(bcg, make_instruction_3(bcg,
                        INST_MEMCPY,
                        make_op_register(dst_ptr),
                        make_op_register(src),
                        make_immediate_uint(ident->type->size, 8, 0)
                    ));

                }
            }
            
        }

        return;
    }

    if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        // TODO
    }
    
}

BasicBlock *create_basic_block(BytecodeGenerator *bcg, char *internal_name) {
    BasicBlock *bb = arena_allocate(&bcg->bytecode_arena, sizeof(BasicBlock));
    bb->id = 0; // Set in set_basic_block
    bb->name = internal_name;
    bb->instructions  = da_init(64, sizeof(Inst));
    bb->predeccessors = da_init(2,  sizeof(BasicBlock));
    bb->usages        = da_init(16, sizeof(int));
    bb->defines       = da_init(16, sizeof(int));
    bb->live_in       = da_init(16, sizeof(int));
    bb->live_out      = da_init(16, sizeof(int));

    return bb;
}

void add_predecessor(BasicBlock *basic_block, BasicBlock *predecessor) {
    da_append(&basic_block->predeccessors, predecessor);
}

void terminate_with_jump(BytecodeGenerator *bcg, BasicBlock *target) {
    bcg->current_basic_block->terminator = (Terminator){ 
        .kind = TERMINATOR_JUMP,
        .index = get_next_instruction_index(bcg),
        .target_count = 1,
        .target1 = target,
    };

    add_predecessor(target, bcg->current_basic_block);
}

void terminate_with_cond_jump(BytecodeGenerator *bcg, int cond_vreg, BasicBlock *true_bb, BasicBlock *false_bb) {
    bcg->current_basic_block->terminator = (Terminator){ 
        .kind = TERMINATOR_COND_JUMP,
        .index = get_next_instruction_index(bcg),
        .condition_reg = cond_vreg,
        .target_count = 2,
        .target1 = true_bb,
        .target2 = false_bb,
    };

    add_predecessor(false_bb, bcg->current_basic_block);
    add_predecessor(true_bb, bcg->current_basic_block);
}

void terminate_with_return(BytecodeGenerator *bcg) {
    bcg->current_basic_block->terminator = (Terminator){ 
        .kind = TERMINATOR_RETURN,
        .index = get_next_instruction_index(bcg),
        .target_count = 0,
    };
}

void seal_to_merge(BytecodeGenerator *bcg, BasicBlock *merge) {
    if (bcg->current_basic_block->terminator.kind == TERMINATOR_NONE) {
        terminate_with_jump(bcg, merge);
    }
}

void set_current_basic_block(BytecodeGenerator *bcg, BasicBlock *basic_block) {
    basic_block->id = bcg->current_basic_block_id++;
    da_append(&bcg->current_function->basic_blocks, basic_block);
    bcg->current_basic_block = basic_block;
}


void bcg_if(BytecodeGenerator *bcg, AstIf *ast_if, BasicBlock *merge) {
    IrValue cond = bcg_expression(bcg, ast_if->condition);

    BasicBlock *then_bb = create_basic_block(bcg, "then");
    BasicBlock *next_bb = create_basic_block(bcg, "next");
    terminate_with_cond_jump(bcg, cond.vreg, then_bb, next_bb);

    set_current_basic_block(bcg, then_bb);
    bcg_block(bcg, ast_if->then_block);
    seal_to_merge(bcg, merge);

    for (int i = 0; i < ast_if->else_ifs.count; i++) {
        AstIf *else_if = da_get_deref(ast_if->else_ifs, i);

        set_current_basic_block(bcg, next_bb);
        IrValue else_if_cond = bcg_expression(bcg, else_if->condition);

        BasicBlock *else_if_then = create_basic_block(bcg, "else_if_then");
        next_bb = create_basic_block(bcg, "else_if_next");
        terminate_with_cond_jump(bcg, else_if_cond.vreg, else_if_then, next_bb);

        set_current_basic_block(bcg, else_if_then);
        bcg_block(bcg, else_if->then_block);
        seal_to_merge(bcg, merge);
    }

    set_current_basic_block(bcg, next_bb);

    if (ast_if->else_block != NULL) {
        bcg_block(bcg, ast_if->else_block);
        seal_to_merge(bcg, merge);
    } else {
        terminate_with_jump(bcg, merge);
    }

}

void bcg_assignment(BytecodeGenerator *bcg, AstAssignment *assign) {
    IrValue lhs = bcg_expression(bcg, assign->lhs);
    IrValue rhs = bcg_expression(bcg, assign->expr);

    Inst result = make_instruction_2(bcg, INST_MOV, 
        make_op_register(lhs), 
        make_op_register(rhs)
    );

    add_instruction(bcg, result);
}

void bcg_statement(BytecodeGenerator *bcg, Ast *node) {
    switch (node->kind) {
    case AST_DECLARATION: {
        bcg_declaration(bcg, (AstDeclaration *)node);
        return;
    }
    case AST_EXPR_STMT: {
        AstExprStmt *expr_stmt = (AstExprStmt *)node;
        bcg_expression(bcg, expr_stmt->expr);
        return;
    }
    case AST_ASSIGNMENT: {
        bcg_assignment(bcg, (AstAssignment *)node);
        return;
    }
    case AST_BLOCK: {
        bcg_block(bcg, (AstBlock *) node);
        return;
    }
    // case AST_PRINT: {
    //     bcg_print(bcg, (AstPrint *) node);
    //     return;
    // }
    case AST_IF: {
        BasicBlock *merge = create_basic_block(bcg, "after_if");
        bcg_if(bcg, (AstIf *)node, merge);
        set_current_basic_block(bcg, merge);
        return;
    }
        
    default:
        return;
    }
}

IrValue bcg_function_call(BytecodeGenerator *bcg, AstFunctionCall *ast_call) {
    InstFunctionCall *call = bytecode_allocate(bcg, sizeof(InstFunctionCall));

    if (!ast_call->is_lambda_call) {
        call->function_symbol = ast_call->func_defn->identifier->name;
    }
    
    call->arguments = da_init(ast_call->arguments.count, sizeof(CallArgument));
    
    for (int i = 0; i < ast_call->arguments.count; i++) {
        AstArgument *arg = da_get_deref(ast_call->arguments, i);
        
        IrValue value = bcg_expression(bcg, arg->value);
        
        CallArgument ir_arg = (CallArgument) {
            .vreg = value.vreg,
            .type = arg->value->type,
            .is_vararg = arg->is_vararg,
        };

        da_append(&call->arguments, ir_arg);
    }

    // TODO: Right now we assume only 1 return value
    Type *return_type = da_get_deref(ast_call->func_defn->return_types, 0); 
    IrValue dst = fresh_register(bcg, return_type);

    // Emit the call instruction
    Inst call_instruction = {0};
    if (ast_call->func_defn->is_lambda) {
        XXX;
    } else {
        call_instruction = make_instruction_2(
            bcg,
            INST_CALL,
            make_op_register(dst),
            LABEL(ast_call->func_defn->identifier->name)
        );
    }
    
    call_instruction.data = call;

    add_instruction(bcg, call_instruction);

    return dst;
}

void bcg_function_defn(BytecodeGenerator *bcg, AstFunctionDefn *func_defn) {
    if (func_defn->head.head.flags & AST_FLAG_IS_BYTECODE_GENERATED) {
        return;
    }

    new_bytecode_function(bcg, func_defn);

    if (func_defn->is_extern) {
        // No need to generate bytecode for external functions
        return;
    }

    BasicBlock *entry = create_basic_block(bcg, func_defn->symbol_name);
    set_current_basic_block(bcg, entry);

    bcg->current_function->entry = entry;
    bcg->enclosing_function = func_defn;

    // add_instruction(bcg, make_instruction_1(INST_LABEL, LABEL(func_defn->symbol_name)));
    // Prologue:

    // Function parameters
    for (int i = 0; i < func_defn->lowered_params.count; i++) {
        AstIdentifier *param = da_get_deref(func_defn->lowered_params, i);

        param->stack_slot = bc_allocate_stack_slot(bcg->current_function, param->type->size);

        BytecodeFunctionParameter bc_param = {
            .vreg = -1,
            .stack_slot = param->stack_slot,
            .type = param->type
        };

        da_append(&bcg->current_function->params, bc_param);
    }

    // Function body
    bcg_block(bcg, func_defn->body);


    // Epilogue:
    // Free local space

    terminate_with_return(bcg);

    // TODO: This should be popping from a stack rather than setting to NULL to allow inner functions
    bcg->enclosing_function = NULL;
    bcg->current_function = NULL;

    func_defn->head.head.flags |= AST_FLAG_IS_BYTECODE_GENERATED;
}

IrValue bcg_emit_arithmetic_conversion(BytecodeGenerator *bcg, IrValue src, Type *to) {
    Type *from = src.type;
    if (from->kind == to->kind && from->size == to->size) {
        return src;
    }

    InstKind kind;
    if (is_integral_type(from) && is_integral_type(to)) {
        if (from->size < to->size) {
            kind = is_unsigned_integer_ish(from) ? INST_ZEXT : INST_SEXT;
        } else {
            return src; // equal size, mixed-sign reinterpret needs no instruction
        }
    } else if (is_integral_type(from) && to->kind == TYPE_FLOAT) {
        kind = is_unsigned_integer_ish(from) ? INST_UITOFP : INST_SITOFP;
    } else { // float -> float
        kind = (from->size < to->size) ? INST_FPEXT : INST_FPTRUNC;
    }

    IrValue dst = fresh_register(bcg, to);
    add_instruction(bcg, make_instruction_2(
        bcg, 
        kind,
        make_op_register(dst), 
        make_op_register(src))
    );
    return dst;
}

IrValue bcg_arithmetic_operator(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    Type *result_type = bin->head.type;

    IrValue lhs = bcg_expression(bcg, bin->left);
    IrValue rhs = bcg_expression(bcg, bin->right);

    // Insert any necessary truncation / widening or type conversion
    lhs = bcg_emit_arithmetic_conversion(bcg, lhs, result_type);
    rhs = bcg_emit_arithmetic_conversion(bcg, rhs, result_type);


    IrValue dst = fresh_register(bcg, bin->head.type);

    Inst result = make_instruction_3(bcg, INST_NOOP, 
        make_op_register(dst),
        make_op_register(lhs),
        make_op_register(rhs)
    );

    if (bin->head.type->kind == TYPE_FLOAT) {
        if (op == '+') {
            result.kind = INST_ADD_FLOAT;
        }
        if (op == '-') {
            result.kind = INST_SUB_FLOAT;
        }
        if (op == '*') {
            result.kind = INST_MUL_FLOAT;
        }
        if (op == '/') {
            result.kind = INST_DIV_FLOAT;
        }
    } else {
        if (op == '+') {
            result.kind = INST_ADD_INT;
        }
        if (op == '-') {
            result.kind = INST_SUB_INT;
        }
        if (op == '*') {
            result.kind = INST_MUL_INT;
        }
        if (op == '/') {
            result.kind = INST_DIV_INT;
        }
    }

    add_instruction(bcg, result);

    return dst;
}

IrValue bcg_comparison_operator(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    IrValue lhs = bcg_expression(bcg, bin->left);
    IrValue rhs = bcg_expression(bcg, bin->right);
    IrValue dst = fresh_register(bcg, bin->head.type);

    Inst result = make_instruction_3(bcg, INST_NOOP, 
        make_op_register(dst),
        make_op_register(lhs),
        make_op_register(rhs)
    );

    if      (op == '<')                 result.kind = INST_LESS_THAN;
    else if (op == '>')                 result.kind = INST_GREATER_THAN;
    else if (op == TOKEN_GREATER_EQUAL) result.kind = INST_GREATER_THAN_EQUAL;
    else if (op == TOKEN_LESS_EQUAL)    result.kind = INST_LESS_THAN_EQUAL;
    else if (op == TOKEN_DOUBLE_EQUAL)  result.kind = INST_DOUBLE_EQUAL;
    else if (op == TOKEN_NOT_EQUAL)     result.kind = INST_NOT_EQUAL;
    else {
        XXX;
    }

    add_instruction(bcg, result);

    return dst;
}

IrValue bcg_boolean_operator(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    IrValue lhs = bcg_expression(bcg, bin->left);
    IrValue rhs = bcg_expression(bcg, bin->right);
    IrValue dst = fresh_register(bcg, bin->head.type);

    Inst result = make_instruction_3(bcg, INST_NOOP, 
        make_op_register(dst),
        make_op_register(lhs),
        make_op_register(rhs)
    );

    if      (op == TOKEN_LOGICAL_AND)   result.kind = INST_LOGICAL_AND;
    else if (op == TOKEN_LOGICAL_OR)    result.kind = INST_LOGICAL_OR;
    else {
        XXX;
    }

    add_instruction(bcg, result);

    return dst;
}

IrValue bcg_binary(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    if (is_arithmetic_operator(op)) {
        return bcg_arithmetic_operator(bcg, bin);
    }

    if (is_comparison_operator(op)) {
        return bcg_comparison_operator(bcg, bin);
    }

    if (is_boolean_operator(op)) {
        return bcg_boolean_operator(bcg, bin);
    }

    XXX;
    return InvalidValue;
}

IrValue bcg_unary(BytecodeGenerator *bcg, AstUnary *unary) {
    OperatorType op = unary->operator;

    IrValue value = bcg_expression(bcg, unary->expr);
    IrValue dst = fresh_register(bcg, unary->head.type);

    if (op == OP_NOT) {
        add_instruction(bcg, make_instruction_2(
            bcg, 
            INST_NOT, 
            make_op_register(dst),
            make_op_register(value)
        ));
        return dst;
    }

    XXX;
    return InvalidValue;
}

Constant *bcg_add_constant(BytecodeGenerator *bcg, Constant *constant) {
    int index = bcg->constant_pool.constants.count;
    constant->id = index;
    da_append(&bcg->constant_pool.constants, *constant);
    return da_get(bcg->constant_pool.constants, index);
}

Constant *bcg_lower_constant(BytecodeGenerator *bcg, AstExpr *expr) {
    switch (expr->head.kind) {
        case AST_LITERAL: {
            AstLiteral *lit = (AstLiteral *)expr;

            switch (lit->kind) {
                case LITERAL_BOOLEAN: {
                    break;
                }
                case LITERAL_INTEGER: {
                    break;
                }
                case LITERAL_FLOAT: {
                    Constant constant = {0};
                    constant.kind = CONSTANT_FLOAT;
                    constant.type = expr->type;
                    constant.as.value_float = lit->as.value.floating;

                    return bcg_add_constant(bcg, &constant);
                    break;
                }
                case LITERAL_STRING: {
                    Constant constant = {0};
                    constant.kind = CONSTANT_STRING;
                    constant.type = expr->type;
                    constant.as.value_string = (String) {
                        .data = lit->as.value.string.data,
                        .len = lit->as.value.string.length,
                    };

                    return bcg_add_constant(bcg, &constant);
                }
                case LITERAL_NULL: {
                    break;
                }
                case LITERAL_IDENTIFIER: {
                    break;
                }
            }

            XXX;
        }
        default:
            XXX;
    }

    return NULL;
}

IrValue bcg_cast(BytecodeGenerator *bcg, AstCast *cast) {

    Type *from = cast->expr->type;
    Type *to   = cast->cast_to;

    IrValue src = bcg_expression(bcg, cast->expr);

    InstKind inst_kind = 0;

    if (is_integral_type(from) && is_integral_type(to)) {

        if (from->size < to->size) {
            // Widening
            if (is_unsigned_integer_ish(from)) {
                // Zero extend
                inst_kind = INST_ZEXT;
            } else {
                // Sign extend
                inst_kind = INST_SEXT;
            }
        } else if (from->size > to->size) {
            // Truncate
            inst_kind = INST_TRUNC;
        } else {
            // No change
            return src;
        }
    }
    else if (is_integral_type(from) && to->kind == TYPE_FLOAT) {
        if (is_unsigned_integer_ish(from)) {
            inst_kind = INST_UITOFP;
        } else {
            inst_kind = INST_SITOFP;
        }
    }
    else if (from->kind == TYPE_FLOAT && is_integral_type(to)) {
        if (is_unsigned_integer_ish(to)) {
            inst_kind = INST_FPTOUI;
        } else {
            inst_kind = INST_FPTOSI;
        }
    }
    else if (from->kind == TYPE_FLOAT && to->kind == TYPE_FLOAT) {
        if (from->size < to->size) {
            inst_kind = INST_FPEXT;
        } else if (from->size > to->size) {
            inst_kind = INST_FPTRUNC;
        } else {
            return src;
        }
    }
    else if (from->kind == TYPE_STRING && to->kind == TYPE_POINTER) {

        // Produce a pointer dereference of .data
        IrValue dst = fresh_register(bcg, to);

        add_instruction(bcg, make_instruction_2(
            bcg,
            INST_DEREF,
            make_op_register(dst),
            make_op_register(src)
        ));

        return dst;
    }
    else {
        // No change ???
        return src;
    }

    IrValue dst = fresh_register(bcg, to);

    Inst inst = make_instruction_2(
        bcg,
        inst_kind,
        make_op_register(dst),
        make_op_register(src)
    );

    add_instruction(bcg, inst);

    return dst;
}

static inline bool is_memory_operand(Operand op) {
    return op.kind == OPERAND_MEMORY_OFFSET || op.kind == OPERAND_MEMORY_REL_OFFSET || op.kind == OPERAND_MEMORY_SLOT;
}

void bcg_emit_store(BytecodeGenerator *bcg, Operand dst, Operand src) {
    assert(is_memory_operand(dst));
    assert(!is_memory_operand(src));

    Inst inst = make_instruction_2(
        bcg,
        INST_STORE,
        dst,
        src
    );

    add_instruction(bcg, inst);
}

void bcg_emit_move(BytecodeGenerator *bcg, Operand dst, Operand src) {
    Inst inst = make_instruction_2(
        bcg,
        INST_MOV,
        dst,
        src
    );

    add_instruction(bcg, inst);
}

void emit_memset(BytecodeGenerator *bcg, StackSlot slot, u64 value, u64 size) {
    emit_instruction_3(bcg, INST_MEMSET, make_op_memory_slot(slot), make_immediate_uint(value, 8, 0), make_immediate_uint(size, 8, 0));
}

void zero_initialize_struct_literal(BytecodeGenerator *bcg, AstStructLiteral *struct_lit, StackSlot stack_slot) {
    (void)struct_lit;

    // @Speed: We could be smarter about zero initializing. Only zero initialize if not all the members are set
    emit_memset(bcg, stack_slot, 0, stack_slot.size);
}

void bcg_struct_literal(BytecodeGenerator *bcg, AstStructLiteral *struct_lit, StackSlot base_slot, int base_offset) {
    for (int i = 0; i < struct_lit->initializers.count; i++) {
        AstStructInitializer *init = da_get_deref(struct_lit->initializers, i);
        
        if (init->value->head.kind == AST_STRUCT_LITERAL) {
            bcg_struct_literal(bcg, (AstStructLiteral *)init->value, base_slot, base_offset + init->member->member_offset);
            continue;
        }

        IrValue value = bcg_expression(bcg, init->value);
        
        Operand dst = make_op_memory_relative_offset(base_slot, base_offset + init->member->member_offset, init->member->type);
        Operand src = make_op_register(value);

        if (dst.type->size >= 8) {
            int k = 0;
        }

        bcg_emit_store(bcg, dst, src);
    }
}

IrValue bcg_member_access(BytecodeGenerator *bcg, AstMemberAccess *ma) {
    IrValue base = bcg_lvalue_expression(bcg, ma->left);

    IrValue result = base;

    result.offset += ma->struct_member->member_offset;
    result.type = ma->head.type;

    return result;
}

IrValue bcg_expression(BytecodeGenerator *bcg, AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_UNARY: {
        return bcg_unary(bcg, (AstUnary *)expr);
    }
    case AST_BINARY: {
        return bcg_binary(bcg, (AstBinary *)expr);
    }
    case AST_FUNCTION_CALL: {
        return bcg_function_call(bcg, (AstFunctionCall *)expr);
    }
    case AST_CAST: {
        return bcg_cast(bcg, (AstCast *)expr);
    }
    case AST_STRUCT_LITERAL: {
        AstStructLiteral *struct_lit = (AstStructLiteral *)expr;

        StackSlot base_slot = bc_allocate_stack_slot(bcg->current_function, struct_lit->head.type->size);

        zero_initialize_struct_literal(bcg, struct_lit, base_slot);
        bcg_struct_literal(bcg, (AstStructLiteral*)expr, base_slot, 0);

        IrValue dst = fresh_register(bcg, struct_lit->head.type);

        add_instruction(bcg, make_instruction_2(
            bcg,
            INST_LEA,
            make_op_register(dst),
            make_op_memory_slot(base_slot)
        ));

        return dst;
    }
    case AST_MEMBER_ACCESS: {
        AstMemberAccess *ma = (AstMemberAccess *)expr;

        IrValue lvalue = bcg_member_access(bcg, ma);

        return bcg_emit_load(bcg, lvalue);
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)expr;

        Operand op = {0};
        switch (lit->kind) {
            case LITERAL_BOOLEAN: {
                op = make_immediate_uint((u64) lit->as.value.boolean, expr->type->size, OPERAND_FLAG_IMM_UINT_IS_BOOLEAN);
                break;
            }
            case LITERAL_INTEGER: {
                if (is_unsigned_integer(expr->type)) {
                    op = make_immediate_uint((u64)lit->as.value.integer, expr->type->size, 0);
                } else {
                    op = make_immediate_int(lit->as.value.integer, expr->type->size);
                }
                break;
            }
            case LITERAL_FLOAT: {
                Constant *constant = bcg_lower_constant(bcg, expr);
                if (!constant) XXX;

                op = make_constant_operand(constant);
                break;
            }
            case LITERAL_STRING: {
                Constant *constant = bcg_lower_constant(bcg, expr);
                if (!constant) {
                    XXX;
                }

                op = make_constant_operand(constant);
                break;
            }
            case LITERAL_NULL: {
                XXX;
            }
            case LITERAL_IDENTIFIER: {
                AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
                assert(ident);

                if (ident->stack_slot.size == 0) {
                    // Assume that this identifier was assigned a virtual register
                    return make_value(ident->virtual_register, ident->type);
                }

                assert(ident->stack_slot.size > 0);

                IrValue dst = fresh_register(bcg, expr->type);

                add_instruction(bcg, make_instruction_2(
                    bcg, 
                    INST_LOAD, 
                    make_op_register(dst),
                    make_op_memory_slot(ident->stack_slot)
                ));

                return dst;
            }
        }
        
        IrValue dst = fresh_register(bcg, expr->type);
        add_instruction(bcg, make_instruction_2(
            bcg,
            INST_MOV, 
            make_op_register(dst),
            op
        ));

        return dst;
    }
    default: {
        report_error_ast(bcg->parser, LABEL_ERROR, (Ast *) expr, "Bytecode expression '%s' not implemented", ast_to_str((Ast *) expr));
        XXX;
        return InvalidValue;
    }
    }

    XXX;
    return InvalidValue;
}

IrValue bcg_lvalue_expression(BytecodeGenerator *bcg, AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_UNARY: {
        AstUnary *unary = (AstUnary *)expr;

        return bcg_lvalue_expression(bcg, unary->expr);
    }
    case AST_MEMBER_ACCESS: {
        return bcg_member_access(bcg, (AstMemberAccess *)expr);
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)expr;

        if (lit->kind != LITERAL_IDENTIFIER) {
            return InvalidValue;
        }

        AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;

        if (ident->stack_slot.size == 0) {
            // TODO: Return as lvalue as register
            XXX;
        }

        return make_static_lvalue(ident->stack_slot.index, 0, ident->type);
    }
    default: {
        return InvalidValue;
    }
    }
}

bool is_vreg_defined_in_block(BasicBlock *bb, int vreg) {
    for (int i = 0; i < bb->defines.count; i++) {
        int *def_vreg = da_get(bb->defines, i);

        if (*def_vreg == vreg) {
            return true;
        }
    }

    return false;
}

bool is_vreg_used_in_block(BasicBlock *bb, int vreg) {
    for (int i = 0; i < bb->usages.count; i++) {
        int *use_vreg = da_get(bb->usages, i);

        if (*use_vreg == vreg) {
            return true;
        }
    }

    return false;
}

void bcg_maybe_add_def(BasicBlock *bb, int vreg) {
    if (!is_vreg_defined_in_block(bb, vreg)) {
        da_append(&bb->defines, vreg);
    }
}

void bcg_maybe_add_use(BasicBlock *bb, int vreg) {
    if (is_vreg_defined_in_block(bb, vreg)) {
        return;
    }

    if (!is_vreg_used_in_block(bb, vreg)) {
        da_append(&bb->usages, vreg);
    }
}

void compute_def_uses_in_basic_block(BasicBlock *bb) {
    for (int i = 0; i < bb->instructions.count; i++) {
        Inst *inst = da_get(bb->instructions, i);

        // Special case: Function call
        if (inst->kind == INST_CALL) {
            InstFunctionCall *call = (InstFunctionCall *) inst->data;

            for (int j = 0; j < call->arguments.count; j++) {
                CallArgument *arg = da_get(call->arguments, j);
                bcg_maybe_add_use(bb, arg->vreg);
            }

            continue;
        }

        for (int j = 0; j < inst->op_count; j++) {
            Operand op = inst->operands[j];

            if (op.kind == OPERAND_REG) {

                if (j == 0) {
                    // Destination = possible def
                    bcg_maybe_add_def(bb, op.vreg);
                } else {
                    // Read = possible use
                    bcg_maybe_add_use(bb, op.vreg);
                }
            }
        }
    }

    if (bb->terminator.kind == TERMINATOR_COND_JUMP) {
        bcg_maybe_add_use(bb, bb->terminator.condition_reg);
    }
}

void compute_live_in_out(DynamicArray basic_blocks) {
    // live_out[B] = ∪ live_in[S]   for every successor S of B
    // live_in[B]  = use[B] ∪ (live_out[B] − def[B])

    bool changed = true;
    while (changed) {
        changed = false; // assume we're done...

        // Go through the blocks in reverse order
        for (int i = basic_blocks.count - 1; i >= 0; i--) {
            BasicBlock *bb = da_get_deref(basic_blocks, i);

            // Take a snapshot of how the live in/out looks before updating
            DynamicArray old_in  = bb->live_in;
            DynamicArray old_out = bb->live_out;

            // live_out[B] = ∪ live_in[S]
            DynamicArray new_out = da_init(8, sizeof(int));
            for (int j = 0; j < bb->terminator.target_count; j++) {
                BasicBlock *succ = bb->terminator.targets[j];

                for (int k = 0; k < succ->live_in.count; k++) {
                    int *vreg = da_get(succ->live_in, k);
                    set_int_add(&new_out, *vreg);
                }
            }

            // live_in[B]  = use[B] ∪ (live_out[B] − def[B])
            // (live_out[B] − def[B])
            DynamicArray live_out_minus_def = set_int_diff(new_out, bb->defines);
            DynamicArray new_in = set_int_union(bb->usages, live_out_minus_def);

            bb->live_out = new_out;
            bb->live_in  = new_in;

            if (old_in.count != new_in.count || old_out.count != new_out.count) {
                changed = true;
            }
        }
    }
}

void bcg_widen_live_interval(BytecodeFunction *func, int vreg, int index) {
    LiveInterval *interval = da_get(func->live_intervals, vreg);
    if (!interval->is_active) {
        interval->vreg = vreg;
        interval->start = index;
        interval->end = index;
        interval->assigned_reg = -1;
        interval->is_active = true;
        interval->is_sse = is_vreg_sse(func, vreg);
    } else {
        if (index < interval->start) interval->start = index;
        if (index > interval->end)   interval->end = index;
    }
}

void compute_live_intervals(BytecodeFunction *func) {
    int num_vregs = func->next_vreg;

    func->live_intervals = da_init(num_vregs, sizeof(LiveInterval));
    func->live_intervals.count = num_vregs;

    bool *is_live = calloc(num_vregs, sizeof(bool));

    for (int i = func->basic_blocks.count - 1; i >= 0; i--) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        if (bb->instructions.count <= 0) {
            continue;
        }

        int low  = ((Inst *)da_get(bb->instructions, 0))->index;
        int high = bb->terminator.index;

        for (int j = 0; j < num_vregs; j++) {
            is_live[j] = false;
        }

        for (int j = 0; j < bb->live_out.count; j++) {
            int v = *(int *)da_get(bb->live_out, j);
            is_live[v] = true;
            bcg_widen_live_interval(func, v, high);
        }

        if (bb->terminator.kind == TERMINATOR_COND_JUMP) {
            int v = bb->terminator.condition_reg;
            is_live[v] = true;
            bcg_widen_live_interval(func, v, high);
        }

        for (int j = bb->instructions.count - 1; j >= 0; j--) {
            Inst *inst = da_get(bb->instructions, j);

            // Special case: Function calls
            if (inst->kind == INST_CALL) {
                InstFunctionCall *call = (InstFunctionCall *) inst->data;

                for (int k = 0; k < call->arguments.count; k++) {
                    CallArgument *arg = da_get(call->arguments, k);

                    // Mark arg as a use
                    bcg_widen_live_interval(func, arg->vreg, inst->index);
                    is_live[arg->vreg] = true;
                }

            }

            for (int k = 0; k < inst->op_count; k++) {
                Operand op = inst->operands[k];

                if (op.kind == OPERAND_REG) {

                    if (k == 0) {
                        // Defs end a range (going backward): record, then mark not-live above.
                        bcg_widen_live_interval(func, op.vreg, inst->index);
                        is_live[op.vreg] = false;
                    } else {
                        // Uses start/extend a range (going backward).
                        bcg_widen_live_interval(func, op.vreg, inst->index);
                        is_live[op.vreg] = true;
                    }
                }
            }
        }

        for (int v = 0; v < num_vregs; v++) {
            if (is_live[v]) {
                bcg_widen_live_interval(func, v, low);
            }
        }
    }
}

bool is_instruction_containing_function_call(Inst inst) {
    if (inst.kind == INST_CALL) return true;
    if (inst.kind == INST_MEMSET) return true;
    if (inst.kind == INST_MEMCPY) return true;
    if (inst.kind == INST_MALLOC) return true;
    if (inst.kind == INST_CALLOC) return true;

    return false;
}

void mark_intervals_crossing_function_calls(BytecodeFunction *func) {
    for (int b = 0; b < func->basic_blocks.count; b++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, b);
        for (int i = 0; i < bb->instructions.count; i++) {
            Inst *inst = da_get(bb->instructions, i);

            if (!is_instruction_containing_function_call(*inst)) {
                continue;
            }

            for (int j = 0; j < func->live_intervals.count; j++) {
                LiveInterval *interval = da_get(func->live_intervals, j);
                if (interval->is_active && interval->start < inst->index && interval->end > inst->index) {
                    interval->crosses_call = true;
                }
            }
        }
    }
}

void spill_at_interval(BytecodeFunction *func, LiveInterval *interval) {
    // find the register-holding interval with the furthest end

    LiveInterval *spill = NULL;
    for (int i = 0; i < func->live_intervals.count; i++) {
        LiveInterval *act = da_get(func->live_intervals, i);
        if (!act->has_reg) continue;
        if (act->is_sse != interval->is_sse) continue; // Not comparable as they use different pools
        if (spill == NULL || act->end > spill->end) {
            spill = act;
        }
    }

    if (spill != NULL && spill->end > interval->end) {
        // evict spill: give its register to the new interval
        interval->assigned_reg = spill->assigned_reg;
        interval->has_reg = true;
        interval->spill_slot = (StackSlot){0};

        spill->assigned_reg = -1;
        spill->has_reg = false;
        spill->spill_slot = bc_allocate_stack_slot(func, 8);
    } else {
        // new interval lives longest: spill it
        interval->assigned_reg = -1;
        interval->has_reg = false;
        interval->spill_slot = bc_allocate_stack_slot(func, 8);
    }
}

int bcg_get_next_free_register(DynamicArray free_regs, LiveInterval *interval) {
    Register *pool = NULL;
    int pool_count = 0;

    if (interval->is_sse) {
        if (interval->crosses_call) {
            pool       = x64_callee_saved_registers_sse;
            pool_count = x64_callee_saved_registers_sse_count;
        } else {
            pool       = x64_usable_registers_sse;
            pool_count = x64_usable_registers_sse_count;
        }
    }
    else {
        if (interval->crosses_call) {
            pool       = x64_callee_saved_registers_gpr;
            pool_count = x64_callee_saved_registers_gpr_count;
        } else {
            pool       = x64_usable_registers_gpr;
            pool_count = x64_usable_registers_gpr_count;
        }
    }

    for (int i = 0; i < pool_count; i++) {
        Register reg = pool[i];

        bool *is_free = da_get(free_regs, reg);
        if (*is_free) {
            return (int) reg;
        }
    }

    // No free register
    return -1;
}

void compute_register_allocation(BytecodeFunction *func) {
    // Assigned_reg of -1 = spill
    DynamicArray free_registers = da_init(REG_COUNT, sizeof(bool));
    free_registers.count = REG_COUNT;
    memset(free_registers.items, 1, free_registers.capacity);

    for (int i = 0; i < func->live_intervals.count; i++) {
        LiveInterval *interval = da_get(func->live_intervals, i);

        interval->has_gone_through_register_allocation = true;
        
        // Skip dead vregs (never live during construction)
        if (!interval->is_active) {
            interval->assigned_reg = -1;
            interval->has_reg = false;
            continue;
        }

        // --- ExpireOldIntervals ---
        // scan everything currently holding a register; free the expired ones
        for (int j = 0; j < func->live_intervals.count; j++) {
            LiveInterval *act = da_get(func->live_intervals, j);
            if (!act->has_reg || act->is_sse != interval->is_sse) continue;

            if (act->end < interval->start) {
                bool *slot = da_get(free_registers, act->assigned_reg);
                *slot = true;
                act->has_reg = false;   // no longer holds a register
            }
        }

        // --- Assign or spill ---
        int free_reg = bcg_get_next_free_register(free_registers, interval);
        if (free_reg != -1) {
            interval->assigned_reg = free_reg;
            interval->has_reg = true;
            bool *slot = da_get(free_registers, free_reg);
            *slot = false;
        } else {
            // --- SpillAtInterval ---
            spill_at_interval(func, interval);
        }
    }
}

void bcg_compute_liveness(BytecodeGenerator *bcg) {

    // Compute def/use per block
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;

        for (int j = 0; j < func->basic_blocks.count; j++) {
            BasicBlock *bb = da_get_deref(func->basic_blocks, j);

            compute_def_uses_in_basic_block(bb);
        }
    }

    // Compute live_in / live_out of each block
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;
        
        compute_live_in_out(func->basic_blocks);
    }

    // Compute live-intervals
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;
        
        compute_live_intervals(func);
    }

    // Mark live-intervals crossing function calls
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;
        
        mark_intervals_crossing_function_calls(func);
    }
}

bool is_sse_register(Register reg) {
    return reg >= REG_XMM0 && reg <= REG_XMM15;
}

bool is_gpr_callee_saved_register(Register reg) {
    if (reg == REG_RBX) return true;
    if (reg >= REG_R12 && reg <= REG_R15) return true;
    return false;
}

bool is_sse_callee_saved_register(Register reg) {
    if (reg >= REG_XMM6 && reg <= REG_XMM15) return true;
    return false;
}

bool is_callee_saved_register(Register reg) {
    return is_gpr_callee_saved_register(reg) || is_sse_callee_saved_register(reg);
}

void mark_used_callee_saved_registers(BytecodeFunction *func) {
    for (int i = 0; i < func->live_intervals.count; i++) {
        LiveInterval *interval = da_get(func->live_intervals, i);

        if (interval->assigned_reg != -1) {
            if (is_gpr_callee_saved_register((Register) interval->assigned_reg)) {
                set_int_add(&func->used_gpr_callee_saved_registers, interval->assigned_reg);
            }
            if (is_sse_callee_saved_register((Register) interval->assigned_reg)) {
                set_int_add(&func->used_sse_callee_saved_registers, interval->assigned_reg);
            }
        }
    }
}

void bcg_compute_last_stages(BytecodeGenerator *bcg) {
    // Compute register allocation
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;
        
        compute_register_allocation(func);
    }

    // Mark used callee saved register on the function
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        if (func->is_extern) continue;
        
        mark_used_callee_saved_registers(func);
    }
}

int reg_of(BytecodeFunction *func, int vreg) {
    LiveInterval *li = da_get(func->live_intervals, vreg);
    return li->assigned_reg;
}

StackSlot slot_of(BytecodeFunction *func, int vreg) {
    LiveInterval *li = da_get(func->live_intervals, vreg);
    return li->spill_slot;
}

bool is_sse_of(BytecodeFunction *func, int vreg) {
    LiveInterval *li = da_get(func->live_intervals, vreg);
    return li->is_sse;
}

Type *get_type_of_vreg(BytecodeFunction *func, int vreg) {
    return da_get_deref(func->vreg_to_type, vreg);
}

bool instruction_has_destination(Inst *inst) {
    if (inst->op_count > 0) {
        if (inst->op1.kind == OPERAND_REG) {
            return true;
        }
    }

    return false;
}

void bcg_rewrite_vreg(BytecodeGenerator *bcg, BytecodeFunction *func, DynamicArray *rewritten_instructions, int vreg, int *out_vreg, int *next_gpr_scratch, int *next_sse_scratch) {
    int reg = reg_of(func, vreg);
    
    if (reg == -1) {
        // Spilled
        Type *type = get_type_of_vreg(func, vreg);
        bool is_sse = is_sse_of(func, vreg);
        StackSlot spill_slot = slot_of(func, vreg);
        int scratch_reg = is_sse 
            ? x64_scratch_registers_sse[(*next_sse_scratch)++]
            : x64_scratch_registers_gpr[(*next_gpr_scratch)++];

        Inst load_inst = make_instruction_2(
            bcg,
            INST_LOAD,
            make_register_ex(scratch_reg, 8, is_sse, type),
            make_op_memory_slot(spill_slot)
        );
        da_append(rewritten_instructions, load_inst);

        *out_vreg = scratch_reg;
    } else {
        *out_vreg = reg;
    }
}

void bcg_rewrite_basic_block_spills(BytecodeGenerator *bcg, BytecodeFunction *func, BasicBlock *bb) {
    DynamicArray rewritten_instructions = da_init(bb->instructions.count, sizeof(Inst));

    int next_gpr_scratch = 0;
    int next_sse_scratch = 0;

    for (int i = 0; i < bb->instructions.count; i++) {
        Inst *inst = da_get(bb->instructions, i);
        Inst out = *inst;

        next_gpr_scratch = 0;
        next_sse_scratch = 0;

        // Special case: Function call
        if (inst->kind == INST_CALL) {
            InstFunctionCall *call = (InstFunctionCall *) inst->data;
            for (int j = 0; j < call->arguments.count; j++) {
                CallArgument *arg = da_get(call->arguments, j);

                bcg_rewrite_vreg(bcg, func, &rewritten_instructions, arg->vreg, &arg->vreg, &next_gpr_scratch, &next_sse_scratch);
            }

            da_append(&rewritten_instructions, out);
            continue;
        }

        // For every read operand that spilled, insert a load instruction
        for (int j = 1; j < inst->op_count; j++) {
            Operand op = inst->operands[j];

            if (op.kind == OPERAND_REG) {
                bcg_rewrite_vreg(bcg, func, &rewritten_instructions, op.vreg, &out.operands[j].preg, &next_gpr_scratch, &next_sse_scratch);
            }
        }

        Type *dst_type = NULL;
        bool  dst_spilled = false;
        bool  dst_is_sse = false;
        int   dst_scratch_reg = 0;
        StackSlot dst_spill_slot = {0};
        if (instruction_has_destination(inst)) {
            Operand op = inst->operands[0];
            int reg = reg_of(func, op.vreg);

            if (reg == -1) {
                // Spilled
                dst_type = get_type_of_vreg(func, op.vreg);
                dst_is_sse = is_sse_of(func, op.vreg);
                dst_spilled = true;
                dst_scratch_reg = dst_is_sse 
                    ? x64_scratch_registers_sse[next_sse_scratch++]
                    : x64_scratch_registers_gpr[next_gpr_scratch++];

                dst_spill_slot = slot_of(func, op.vreg);

                out.operands[0].preg = dst_scratch_reg;
            } else {
                out.operands[0].preg = reg;
            }
        }

        // Check for a redundant move to the same register (copy coalescing)
        bool is_noop = false;
        if (out.kind == INST_MOV && out.op1.kind == OPERAND_REG && out.op2.kind == OPERAND_REG) {
            if (out.op1.preg == out.op2.preg) {
                is_noop = true;
            }
        }

        if (!is_noop) {
            da_append(&rewritten_instructions, out);
        }

        if (dst_spilled) {
            Inst store_inst = make_instruction_2(
                bcg,
                INST_STORE,
                make_op_memory_slot(dst_spill_slot),
                make_register_ex(dst_scratch_reg, 8, dst_is_sse, dst_type)
            );

            da_append(&rewritten_instructions, store_inst);
        }
    }

    // Reset scratch registers
    next_gpr_scratch = 0;
    next_sse_scratch = 0;

    // Rewrite the terminator
    if (bb->terminator.kind == TERMINATOR_COND_JUMP) {
        int reg = reg_of(func, bb->terminator.condition_reg);

        if (reg == -1) {
            // Spilled
            int scratch_reg = x64_scratch_registers_gpr[next_gpr_scratch++];
            StackSlot spill_slot = slot_of(func, bb->terminator.condition_reg);
            Type *cond_type = get_type_of_vreg(func, bb->terminator.condition_reg);

            Inst load_inst = make_instruction_2(
                bcg,
                INST_LOAD,
                make_register_ex(scratch_reg, 8, false, cond_type),
                make_op_memory_slot(spill_slot)
            );

            da_append(&rewritten_instructions, load_inst);

            bb->terminator.condition_reg = scratch_reg;
        } else {
            bb->terminator.condition_reg = reg;
        }
    }

    // Swap in the rewritten instructions
    bb->instructions = rewritten_instructions;
}

void bcg_rewrite_function_spills(BytecodeGenerator *bcg, BytecodeFunction *func) {
    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        bcg_rewrite_basic_block_spills(bcg, func, bb);
    }
}

void bcg_rewrite_entire_ir(BytecodeGenerator *bcg) {
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        bcg_rewrite_function_spills(bcg, func);
    }
}