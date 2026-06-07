#include "typer.c"

#define SHOW_IR_LINE_NUMBERS 1
#define SHOW_IR_LIVE_INTERVALS 1
#define SHOW_IR_DEF_USE 1

typedef struct BasicBlock BasicBlock;

typedef enum TerminatorKind {
    TERMINATOR_NONE,
    TERMINATOR_JUMP,
    TERMINATOR_COND_JUMP,
    TERMINATOR_RETURN,
} TerminatorKind;

typedef struct Terminator {
    TerminatorKind kind;
    int            condition_vreg;
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
    Type *type;
} BytecodeFunctionParameter;

typedef struct LiveInterval {
    int vreg;
    int start;
    int end;
    int assigned_reg; // -1 = spilled
    bool is_active;
} LiveInterval;

typedef struct BytecodeFunction {
    char        *symbol_name;
    BasicBlock  *entry;
    DynamicArray basic_blocks; // of *BasicBlock.
    int          next_vreg;
    int          next_instruction_index;

    DynamicArray params; // of BytecodeFunctionParameter

    // Stuff for liveness analysis
    DynamicArray live_ranges;

    // Emission stage
    int          frame_size;
    DynamicArray vreg_to_stack_slot; // of int

} BytecodeFunction;

typedef struct InstFunctionCall {
    char *function_symbol; // in case of a named function

    DynamicArray arg_offsets; // of int
    DynamicArray arg_vregs;   // of int
    int          ret_vreg;
} InstFunctionCall;

typedef struct BytecodeGenerator {
    Parser *parser;

    int current_basic_block_id;

    Arena        bytecode_arena;

    DynamicArray bytecode_functions;    // of *ByteCodeFunction.

    BasicBlock        *current_basic_block;
    BytecodeFunction  *current_function;

    // This string builder must never resize, as we rely on stable pointers to null terminated strings inside it
    StringBuilder temporary_strings;

    AstFile         *current_file;
    AstBlock        *current_scope;
    AstFunctionDefn *enclosing_function;  // Current enclosing function
} BytecodeGenerator;

typedef enum OperandKind {
    OPERAND_NONE,
    OPERAND_IMM_INT,        // Immediate value:  ADD r0, 42
    OPERAND_IMM_UINT,        // Immediate value:  ADD r0, 42
    OPERAND_IMM_FLOAT,      // Immediate value:  ADD r0, 42
    OPERAND_REG,            // Virtual register: ADD r0, r1
    OPERAND_MEM,            // Memory address:   ADD r0, [rbp-8]
    OPERAND_LABEL,          // Branch label
    OPEARND_LABEL_ID,       // Branch label id only
} OperandKind;

typedef enum InstKind {
    INST_NOOP,

    INST_COMMENT,

    INST_MOV,
    INST_LOAD,
    INST_STORE,
    INST_PUSH,
    INST_POP,
    INST_COND_JUMP,
    INST_LABEL,
    INST_CALL,
    INST_RETURN,

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
    
} InstKind;

char *inst_kind_to_string(InstKind kind) {
    switch (kind) {
    case INST_NOOP:                 return "noop";
    case INST_COMMENT:              return "comment";
    case INST_MOV:                  return "mov";
    case INST_LOAD:                 return "load";
    case INST_STORE:                return "store";
    case INST_PUSH:                 return "push";
    case INST_POP:                  return "pop";
    case INST_COND_JUMP:            return "jmpif";
    case INST_LABEL:                return "label";
    case INST_CALL:                 return "call";
    case INST_RETURN:               return "ret";
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
    }

    return "???";
}

typedef struct Operand {
    u8  kind;
    u16 flags;
    u8  size; // 1, 2, 4, 8 bytes in case of immediate values. 
    union {
        i64    imm_int;       // Immediate signed integer value
        u64    imm_uint;      // Immediate unsigned integer value
        double imm_float;     // Immediate float value
        int    vreg;          // Virtual register index
        int    offset;        // Memory offset (e.g. from rbp)
        char  *label;         // E.g function call or branch label
        int    label_id;      // E.g id of a block
    };
} Operand;

typedef struct Inst {
    u16     kind;
    u8      op_count;
    int     index;
    void   *data;       // Pointer to arbitrary extra data the instruction might need
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
    func->vreg_to_stack_slot = da_init(128, sizeof(int));
    func->vreg_to_stack_slot.count = 128;

    da_append(&bcg->bytecode_functions, func);
    bcg->current_function = func;

    return func;
}

BytecodeGenerator bytecode_generator_init(Parser *parser) {
    BytecodeGenerator bcg = {0};
    bcg.parser = parser;

    bcg.bytecode_arena = arena_init(8192);
    bcg.bytecode_functions = da_init(32, sizeof(BytecodeFunction*));
    bcg.temporary_strings = sb_init(40000);

    return bcg;
}

void bcg_file(BytecodeGenerator *bcg, AstFile *code);
void bcg_block(BytecodeGenerator *bcg, AstBlock *block);
void bcg_statement(BytecodeGenerator *bcg, Ast *node);
int bcg_binary(BytecodeGenerator *bcg, AstBinary *bin);
int bcg_unary(BytecodeGenerator *bcg, AstUnary *unary);
int bcg_function_call(BytecodeGenerator *bcg, AstFunctionCall *call);
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
int bcg_array_literal(BytecodeGenerator *bcg, AstArrayLiteral *array_lit);
void bcg_struct_literal(BytecodeGenerator *bcg, AstStructLiteral *struct_lit);
void bcg_declaration(BytecodeGenerator *bcg, AstDeclaration *decl);
void bcg_assignment(BytecodeGenerator *bcg, AstAssignment *assign);
int bcg_array_access(BytecodeGenerator *bcg, AstArrayAccess *array_ac, bool lvalue);
int bcg_expression(BytecodeGenerator *bcg, AstExpr *expr);

#define REG(i)   ((Operand){ .kind = OPERAND_REG,       .vreg = (i) })
#define MEM(o)   ((Operand){ .kind = OPERAND_MEM,       .offset = (o) })
#define LABEL(s) ((Operand){ .kind = OPERAND_LABEL,     .label = (s) })
#define LABEL_ID(id) ((Operand){ .kind = OPEARND_LABEL_ID, .label_id = (id) })
#define NO_OP    ((Operand){ .kind = OPERAND_NONE })

void begin_bytecode_generation(BytecodeGenerator *bcg, AstFile *file) {
    for (int i = 0; i < file->flattened_function_defns.count; i++) {
        AstFunctionDefn *func_defn = da_get_deref(file->flattened_function_defns, i);
        bcg_function_defn(bcg, func_defn);
    }
}


int fresh_register(BytecodeGenerator *bcg) {
    return bcg->current_function->next_vreg++;
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

// static Inst make_instruction_1(InstKind kind, Operand op1) {
//     return (Inst) {
//         .kind = kind,
//         .op_count = 1,
//         .op1 = op1,
//         .op2 = NO_OP,
//         .op3 = NO_OP,
//     };
// }

static Inst make_instruction_0(BytecodeGenerator *bcg, InstKind kind) {
    return (Inst) {
        .kind = kind,
        .index = get_next_instruction_index(bcg),
        .op_count = 0,
        .op1 = NO_OP,
        .op2 = NO_OP,
        .op3 = NO_OP,
    };
}

static Operand make_sized_register(int vreg, u8 size) {
    return (Operand) {
        .kind = OPERAND_REG,
        .size = size,
        .vreg = vreg,
    };
}

Operand make_immediate_uint(u64 value, int size) {
    return (Operand){
        .kind = OPERAND_IMM_UINT,
        .size = size,
        .imm_uint = value,
    };
}

Operand make_immediate_int(i64 value, int size) {
    return (Operand){
        .kind = OPERAND_IMM_INT,
        .size = size,
        .imm_int = value,
    };
}

Operand make_immediate_float(double value, int size) {
    return (Operand){
        .kind = OPERAND_IMM_FLOAT,
        .size = size,
        .imm_float = value,
    };
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

void dump_operand_to_string(StringBuilder *sb, Operand op) {
    switch (op.kind) {
    case OPERAND_NONE: {
        sb_append(sb, "noop");
        break;
    }
    case OPERAND_IMM_INT: {
        sb_append(sb, "%lld", op.imm_int);
        break;
    }
    case OPERAND_IMM_FLOAT: {
        sb_append(sb, "%llf", op.imm_float);
        break;
    }
    case OPERAND_REG: {
        sb_append(sb, "v%d", op.vreg);
        break;
    }
    case OPERAND_MEM: {
        sb_append(sb, "[%d]", op.offset);
        break;
    }
    case OPERAND_LABEL: {
        sb_append(sb, "%s", op.label);
        break;
    }
    }
}

void bcg_dump_instruction(StringBuilder *sb, Inst *inst) {

    if (inst->kind == INST_COMMENT) {
        sb_append(sb, "\t; %s\n", (char *) inst->data);
        return;
    }

    // General case:
    char *inst_kind_str = inst_kind_to_string(inst->kind);

    sb_append(sb, "\t");

    // Show instruction numbers?
    #if SHOW_IR_LINE_NUMBERS
        sb_append(sb, "%d: ",inst->index);
    #endif

    if (strlen(inst_kind_str) > 3) {
        sb_append(sb, "%s\t", inst_kind_str);
    } else {
        sb_append(sb, "%s\t\t", inst_kind_str);
    }

    for (int k = 0; k < inst->op_count; k++) {
        dump_operand_to_string(sb, inst->operands[k]);
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
    for (int i = 0; i < func->live_ranges.count; i++) {
        LiveInterval *interval = da_get(func->live_ranges, i);

        sb_append(sb, "; v%d = [%d, %d]\n", interval->vreg, interval->start, interval->end);
    }
}

void bcg_dump_function_defn(StringBuilder *sb, BytecodeFunction *func) {
    sb_append(sb, "%s:\n", func->symbol_name);

    #if SHOW_IR_LIVE_INTERVALS
        bcg_dump_live_intervals(sb, func);
    #endif

    for (int i = 0; i < func->basic_blocks.count; i++) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        sb_append(sb, "\n");

        #if SHOW_IR_USE_DEFS
            bcg_dump_block_liveness_info(sb, bb);
        #endif

        sb_append(sb, "L%d:\n", bb->id);
        for (int j = 0; j < bb->instructions.count; j++) {
            Inst *inst = da_get(bb->instructions, j);

            bcg_dump_instruction(sb, inst);
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
                sb_append(sb, "jmp_if\tv%d, L%d, L%d\n", bb->terminator.condition_vreg, bb->terminator.target1->id, bb->terminator.target2->id);
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
     
        bcg_dump_function_defn(&sb, func);
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

void bcg_declaration(BytecodeGenerator *bcg, AstDeclaration *decl) {

    if (decl->flags & DECLARATION_CONSTANT) {
        XXX;
    }

    if ((decl->flags & DECLARATION_INFER) || (decl->flags & DECLARATION_TYPED)) {
        for (int i = 0; i < decl->idents.count; i++) {
            AstIdentifier *ident = da_get_deref(decl->idents, i);
            AstExpr       *value = da_get_deref(decl->values, i);

            int src = bcg_expression(bcg, value);
            ident->virtual_register = fresh_register(bcg);

            add_instruction(bcg, make_instruction_2(bcg,
                INST_MOV, 
                make_sized_register(ident->virtual_register, ident->type->size),
                make_sized_register(src, value->type->size)
            ));
        }

        return;
    }

    if (decl->flags & DECLARATION_TYPED_NO_EXPR) {
        XXX;
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
        .condition_vreg = cond_vreg,
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
    int cond_vreg = bcg_expression(bcg, ast_if->condition);

    BasicBlock *then_bb = create_basic_block(bcg, "then");
    BasicBlock *next_bb = create_basic_block(bcg, "next");
    terminate_with_cond_jump(bcg, cond_vreg, then_bb, next_bb);

    set_current_basic_block(bcg, then_bb);
    bcg_block(bcg, ast_if->then_block);
    seal_to_merge(bcg, merge);

    for (int i = 0; i < ast_if->else_ifs.count; i++) {
        AstIf *else_if = da_get_deref(ast_if->else_ifs, i);

        set_current_basic_block(bcg, next_bb);
        int else_if_cond_vreg = bcg_expression(bcg, else_if->condition);

        BasicBlock *else_if_then = create_basic_block(bcg, "else_if_then");
        next_bb = create_basic_block(bcg, "else_if_next");
        terminate_with_cond_jump(bcg, else_if_cond_vreg, else_if_then, next_bb);

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
    int lhs_vreg = bcg_expression(bcg, assign->lhs);
    int rhs_vreg = bcg_expression(bcg, assign->expr);

    Inst result = make_instruction_2(bcg, INST_MOV, 
        make_sized_register(lhs_vreg, assign->lhs->type->size), 
        make_sized_register(rhs_vreg, assign->expr->type->size)
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
        AstAssignment *assign = (AstAssignment *)node;
        bcg_assignment(bcg, assign);
        return;
    }
    case AST_BLOCK: {
        bcg_block(bcg, (AstBlock *) node);
        return;
    }
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

int bcg_function_call(BytecodeGenerator *bcg, AstFunctionCall *call) {
    InstFunctionCall *call_metadata = bytecode_allocate(bcg, sizeof(InstFunctionCall));

    call_metadata->arg_vregs   = da_init(call->arguments.count, sizeof(int));
    call_metadata->arg_offsets = da_init(call->arguments.count, sizeof(int));

    // int callee_vreg = bcg_expression(bcg, call->expression);
    int ret_vreg = fresh_register(bcg);

    Inst call_instruction = {0};
    if (call->func_defn->is_lambda) {
        XXX;
    } else {
        call_instruction = make_instruction_2(
            bcg,
            INST_CALL, 
            make_sized_register(ret_vreg, 8), 
            LABEL(call->func_defn->identifier->name)
        );
    }
    
    call_instruction.data = call_metadata;
    
    for (int i = 0; i < call->arguments.count; i++) {
        AstArgument *arg = da_get_deref(call->arguments, i);
        
        int arg_vreg = bcg_expression(bcg, arg->value);
        da_append(&call_metadata->arg_vregs, arg_vreg);
    }


    char *arg_list_string = &bcg->temporary_strings.buffer[bcg->temporary_strings.cursor];

    sb_append(&bcg->temporary_strings, "%s(", call->func_defn->symbol_name);
    for (int i = 0; i < call_metadata->arg_vregs.count; i++) {
        int *arg_vreg = da_get(call_metadata->arg_vregs, i);

        sb_append(&bcg->temporary_strings, "v%d", *arg_vreg);
        if (i + 1 < call_metadata->arg_vregs.count) {
            sb_append(&bcg->temporary_strings, ",");
        }
    }
    sb_append(&bcg->temporary_strings, ")\0");

    // add_comment(bcg, arg_list_string);
    add_instruction(bcg, call_instruction);

    return ret_vreg;
}

void bcg_function_defn(BytecodeGenerator *bcg, AstFunctionDefn *func_defn) {
    new_bytecode_function(bcg, func_defn);

    BasicBlock *entry = create_basic_block(bcg, func_defn->symbol_name);
    set_current_basic_block(bcg, entry);

    bcg->current_function->entry = entry;


    bcg->enclosing_function = func_defn;

    // add_instruction(bcg, make_instruction_1(INST_LABEL, LABEL(func_defn->symbol_name)));
    // Prologue:

    // Function parameters
    // Bind each parameter to a fresh vreg, in order.
    for (int i = 0; i < func_defn->lowered_params.count; i++) {
        AstIdentifier *param = da_get_deref(func_defn->lowered_params, i);
        param->virtual_register = fresh_register(bcg);

        BytecodeFunctionParameter bc_param = {
            .vreg = param->virtual_register,
            .type = param->type
        };

        da_append(&bcg->current_function->params, bc_param);
    }

    // Function body
    bcg_block(bcg, func_defn->body);


    // Epilogue:
    // Free local space

    terminate_with_return(bcg);

    bcg->enclosing_function = NULL;
}

int bcg_arithmetic_operator(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    int lhs_reg = bcg_expression(bcg, bin->left);
    int rhs_reg = bcg_expression(bcg, bin->right);
    int dst_reg = fresh_register(bcg);

    Inst result = make_instruction_3(bcg, INST_NOOP, 
        make_sized_register(dst_reg, bin->head.type->size),
        make_sized_register(lhs_reg, bin->left->type->size),
        make_sized_register(rhs_reg, bin->right->type->size)
    );

    if (op == '+') {
        result.kind = INST_ADD_INT;
        add_instruction(bcg, result);
        return dst_reg;
    }

    if (op == '-') {
        result.kind = INST_SUB_INT;
        add_instruction(bcg, result);
        return dst_reg;
    }

    if (op == '*') {
        result.kind = INST_MUL_INT;
        add_instruction(bcg, result);
        return dst_reg;
    }

    if (op == '/') {
        result.kind = INST_DIV_INT;
        add_instruction(bcg, result);
        return dst_reg;
    }

    return -1;
}

int bcg_comparison_operator(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    int lhs_reg = bcg_expression(bcg, bin->left);
    int rhs_reg = bcg_expression(bcg, bin->right);
    int dst_reg = fresh_register(bcg);

    Inst result = make_instruction_3(bcg, INST_NOOP, 
        make_sized_register(dst_reg, bin->head.type->size),
        make_sized_register(lhs_reg, bin->left->type->size),
        make_sized_register(rhs_reg, bin->right->type->size)
    );

    if      (op == '<')                 result.kind = INST_LESS_THAN;
    else if (op == '>')                 result.kind = INST_GREATER_THAN;
    else if (op == TOKEN_GREATER_EQUAL) result.kind = INST_GREATER_THAN_EQUAL;
    else if (op == TOKEN_LESS_EQUAL)    result.kind = INST_LESS_THAN_EQUAL;
    else if (op == TOKEN_DOUBLE_EQUAL)  result.kind = INST_DOUBLE_EQUAL;
    else {
        XXX;
    }

    add_instruction(bcg, result);

    return dst_reg;
}

int bcg_binary(BytecodeGenerator *bcg, AstBinary *bin) {
    TokenType op = bin->operator;

    if (is_arithmetic_operator(op)) {
        return bcg_arithmetic_operator(bcg, bin);
    }

    if (is_comparison_operator(op)) {
        return bcg_comparison_operator(bcg, bin);
    }

    return -1;
}

int bcg_expression(BytecodeGenerator *bcg, AstExpr *expr) {
    switch (expr->head.kind) {
    case AST_BINARY: {
        return bcg_binary(bcg, (AstBinary *)expr);
    }
    case AST_FUNCTION_CALL: {
        return bcg_function_call(bcg, (AstFunctionCall *)expr);
    }
    case AST_LITERAL: {
        AstLiteral *lit = (AstLiteral *)expr;

        Operand imm = {0};
        switch (lit->kind) {
            case LITERAL_BOOLEAN: {
                imm = make_immediate_uint((u64) lit->as.value.boolean, expr->type->size);
                break;
            }
            case LITERAL_INTEGER: {
                if (is_unsigned_integer(expr->type)) {
                    imm = make_immediate_uint((u64)lit->as.value.integer, expr->type->size);
                } else {
                    imm = make_immediate_int(lit->as.value.integer, expr->type->size);
                }
                break;
            }
            case LITERAL_FLOAT: {
                imm = make_immediate_float(lit->as.value.floating, expr->type->size);
                break;
            }
            case LITERAL_STRING: {
                break;
            }
            case LITERAL_NULL: {
                break;
            }
            case LITERAL_IDENTIFIER: {
                AstIdentifier *ident = lit->as.value.identifier.resolved_identifier;
                assert(ident);

                if (ident->ident_override) {
                    ident = ident;
                }

                return ident->virtual_register;

            }
        }
        
        int dst = fresh_register(bcg);
        add_instruction(bcg, make_instruction_2(
            bcg,
            INST_MOV, 
            make_sized_register(dst, expr->type->size),
            imm
        ));

        return dst;
    }
    default:
        return -1;
    }

    return -1;
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

void bcg_compute_def_uses_in_basic_block(BytecodeGenerator *bcg, BasicBlock *bb) {
    for (int i = 0; i < bb->instructions.count; i++) {
        Inst *inst = da_get(bb->instructions, i);

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
        bcg_maybe_add_use(bb, bb->terminator.condition_vreg);
    }
}

void bcg_compute_live_in_out(DynamicArray basic_blocks) {
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

void bcg_widen_live_interval(DynamicArray *intervals, int vreg, int index) {
    LiveInterval *interval = da_get(*intervals, vreg);
    if (!interval->is_active) {
        interval->vreg = vreg;
        interval->start = index;
        interval->end = index;
        interval->assigned_reg = -1;
        interval->is_active = true;
    } else {
        if (index < interval->start) interval->start = index;
        if (index > interval->end)   interval->end = index;
    }
}

void bcg_compute_live_intervals(BytecodeFunction *func) {
    int num_vregs = func->next_vreg;

    func->live_ranges = da_init(num_vregs, sizeof(LiveInterval));
    func->live_ranges.count = num_vregs;

    bool *is_live = calloc(num_vregs, sizeof(bool));

    for (int i = func->basic_blocks.count - 1; i >= 0; i--) {
        BasicBlock *bb = da_get_deref(func->basic_blocks, i);

        int low  = bb->instructions.count > 0 ? ((Inst *)da_get(bb->instructions, 0))->index : -1;
        int high = bb->terminator.index;

        for (int j = 0; j < num_vregs; j++) {
            is_live[j] = false;
        }

        for (int j = 0; j < bb->live_out.count; j++) {
            int v = *(int *)da_get(bb->live_out, j);
            is_live[v] = true;
            // bcg_widen_live_interval(&func->live_ranges, v, low);
            bcg_widen_live_interval(&func->live_ranges, v, high);
        }

        if (bb->terminator.kind == TERMINATOR_COND_JUMP) {
            int v = bb->terminator.condition_vreg;
            is_live[v] = true;
            bcg_widen_live_interval(&func->live_ranges, v, high);
        }

        for (int j = bb->instructions.count - 1; j >= 0; j--) {
            Inst *inst = da_get(bb->instructions, j);

            for (int k = 0; k < inst->op_count; k++) {
                Operand op = inst->operands[k];

                if (op.kind == OPERAND_REG) {

                    if (k == 0) {
                        // Defs end a range (going backward): record, then mark not-live above.
                        bcg_widen_live_interval(&func->live_ranges, op.vreg, inst->index);
                        is_live[op.vreg] = false;
                    } else {
                        // Uses start/extend a range (going backward).
                        bcg_widen_live_interval(&func->live_ranges, op.vreg, inst->index);
                        is_live[op.vreg] = true;
                    }
                }
            }
        }

        for (int v = 0; v < num_vregs; v++) {
            if (is_live[v]) {
                bcg_widen_live_interval(&func->live_ranges, v, low);
            }
        }
    }
}

void bcg_compute_liveness(BytecodeGenerator *bcg) {

    // Compute def/use per block
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);

        for (int j = 0; j < func->basic_blocks.count; j++) {
            BasicBlock *bb = da_get_deref(func->basic_blocks, j);

            bcg_compute_def_uses_in_basic_block(bcg, bb);
        }
    }

    // Compute live_in / live_out of each block
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        
        bcg_compute_live_in_out(func->basic_blocks);
    }

    // Compute live-intervals
    for (int i = 0; i < bcg->bytecode_functions.count; i++) {
        BytecodeFunction *func = da_get_deref(bcg->bytecode_functions, i);
        
        bcg_compute_live_intervals(func);
    }


}