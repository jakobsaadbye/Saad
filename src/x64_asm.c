//////////////////////////////////////////////////////////////////////////////////////////
//
// The assembler of the compiler. It turns the assembly that x64_emit.c writes into machine code, so no assembler (nasm)
// has to be installed and run. It only knows the part of NASM's syntax that x64_emit.c uses:
//
//     section .text / .data / .rdata, extern $name, global $name, bits 64, default rel
//     labels:      $name:  .L3:  __float2:      (a label starting with '.' belongs to the label before it)
//     data:        db 1, 2   dd 0x80000000   dq 0x3ff0000000000000   dq $name   times 16 db 0   align 8
//     operands:    rax eax ax al xmm0   12 -4 0x10   qword [rbp-8]   [rax+16]   [r8 + r9]   [rel $name+8]   .L3   $name
//
// The result is an AsmObject: the bytes of the sections, the symbols, and the relocations that a linker (or the
// writers of object_writer.c) fills in. Jumps to labels are made as short as they can be (rel8), like nasm does.
//
// The instructions are the ones x64_emit.c writes: integer arithmetic, moves, compares and setcc, jumps and calls,
// shifts, division, and the SSE instructions for f32 and f64. A new instruction in x64_emit.c needs a case here too,
// otherwise the assembler reports it.
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <ctype.h>

typedef enum AsmSectionId {
    ASM_TEXT,
    ASM_DATA,
    ASM_RDATA,
    ASM_SECTION_COUNT,
} AsmSectionId;

static const char *asm_section_names[ASM_SECTION_COUNT] = { ".text", ".data", ".rdata" };

typedef struct AsmBytes {
    uint8_t *data;
    int      size;
    int      capacity;
} AsmBytes;

typedef enum AsmRelocKind {
    ASM_RELOC_REL32,  // A 32-bit offset from the end of the instruction: S + addend - (P + 4 + trailing)
    ASM_RELOC_ABS64,  // A 64-bit address: S + addend
} AsmRelocKind;

typedef struct AsmReloc {
    int          section;
    int          offset;    // Of the field in the section
    AsmRelocKind kind;
    int          symbol;
    int64_t      addend;
    int          trailing;  // Bytes of the instruction after the field, e.g an immediate after a rip-relative address
} AsmReloc;

typedef struct AsmSymbol {
    char *name;
    int   section;   // -1 when it is not defined here (extern)
    int   offset;
    bool  is_global;
    bool  is_extern;
    char *library;   // The library an extern is in, e.g "kernel32.dll". NULL for the C runtime
} AsmSymbol;

typedef struct AsmObject {
    AsmBytes   sections[ASM_SECTION_COUNT];
    int        alignment[ASM_SECTION_COUNT];
    AsmSymbol *symbols;
    int        symbol_count;
    int        symbol_capacity;
    AsmReloc  *relocs;
    int        reloc_count;
    int        reloc_capacity;

    // Name -> symbol, open addressing
    int       *table;
    int        table_size;

    char       error[512];
} AsmObject;

static void asm_put(AsmBytes *b, const void *data, int size) {
    if (b->size + size > b->capacity) {
        int capacity = b->capacity ? b->capacity * 2 : 4096;
        while (capacity < b->size + size) capacity *= 2;
        b->data = realloc(b->data, capacity);
        b->capacity = capacity;
    }
    memcpy(&b->data[b->size], data, size);
    b->size += size;
}

static void asm_put_byte(AsmBytes *b, uint8_t byte) { asm_put(b, &byte, 1); }

//
// Symbols
//

static uint32_t asm_hash(const char *s) {
    uint32_t h = 2166136261u;
    for (; *s; s++) h = (h ^ (uint8_t)*s) * 16777619u;
    return h;
}

static void asm_table_insert(AsmObject *obj, int index) {
    uint32_t h = asm_hash(obj->symbols[index].name);
    for (int i = h & (obj->table_size - 1); ; i = (i + 1) & (obj->table_size - 1)) {
        if (obj->table[i] < 0) { obj->table[i] = index; return; }
    }
}

// The symbol with the name, which is made when there is none yet
static int asm_symbol(AsmObject *obj, const char *name) {
    if (obj->table_size > 0) {
        uint32_t h = asm_hash(name);
        for (int i = h & (obj->table_size - 1); obj->table[i] >= 0; i = (i + 1) & (obj->table_size - 1)) {
            if (strcmp(obj->symbols[obj->table[i]].name, name) == 0) return obj->table[i];
        }
    }

    if (obj->symbol_count == obj->symbol_capacity) {
        obj->symbol_capacity = obj->symbol_capacity ? obj->symbol_capacity * 2 : 256;
        obj->symbols = realloc(obj->symbols, obj->symbol_capacity * sizeof(AsmSymbol));
    }
    if ((obj->symbol_count + 1) * 2 > obj->table_size) {
        obj->table_size = obj->table_size ? obj->table_size * 2 : 1024;
        obj->table = realloc(obj->table, obj->table_size * sizeof(int));
        for (int i = 0; i < obj->table_size; i++) obj->table[i] = -1;
        for (int i = 0; i < obj->symbol_count; i++) asm_table_insert(obj, i);
    }
    int index = obj->symbol_count++;
    obj->symbols[index] = (AsmSymbol){ .name = strdup(name), .section = -1 };
    asm_table_insert(obj, index);
    return index;
}

static void asm_add_reloc(AsmObject *obj, AsmReloc reloc) {
    if (obj->reloc_count == obj->reloc_capacity) {
        obj->reloc_capacity = obj->reloc_capacity ? obj->reloc_capacity * 2 : 1024;
        obj->relocs = realloc(obj->relocs, obj->reloc_capacity * sizeof(AsmReloc));
    }
    obj->relocs[obj->reloc_count++] = reloc;
}

//
// Operands
//

typedef enum AsmOperandKind {
    ASM_NONE,
    ASM_REG,
    ASM_XMM,
    ASM_MEM,
    ASM_IMM,
    ASM_LABEL,
} AsmOperandKind;

typedef struct AsmOperand {
    AsmOperandKind kind;
    int     reg;       // ASM_REG, ASM_XMM: 0..15
    int     width;     // ASM_REG: 1, 2, 4 or 8. ASM_MEM: the size keyword, or 0
    bool    high8;     // spl, bpl, sil, dil: need a REX prefix
    // ASM_MEM
    int     base;      // -1 for rip-relative
    int     index;     // -1 for none
    int64_t disp;
    char    symbol[160];  // rip-relative, or ASM_LABEL
    int64_t imm;       // ASM_IMM
} AsmOperand;

static const char *asm_reg64[16] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };
static const char *asm_reg32[16] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };
static const char *asm_reg16[16] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };
static const char *asm_reg8[16]  = { "al", "cl", "dl", "bl", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };

static bool asm_parse_register(const char *s, int length, AsmOperand *op) {
    char name[16];
    if (length <= 0 || length >= (int)sizeof(name)) return false;
    memcpy(name, s, length);
    name[length] = 0;
    for (int r = 0; r < 16; r++) {
        if (strcmp(name, asm_reg64[r]) == 0) { op->kind = ASM_REG; op->reg = r; op->width = 8; return true; }
        if (strcmp(name, asm_reg32[r]) == 0) { op->kind = ASM_REG; op->reg = r; op->width = 4; return true; }
        if (strcmp(name, asm_reg16[r]) == 0) { op->kind = ASM_REG; op->reg = r; op->width = 2; return true; }
        if (strcmp(name, asm_reg8[r])  == 0) { op->kind = ASM_REG; op->reg = r; op->width = 1; op->high8 = r >= 4 && r <= 7; return true; }
    }
    if (strncmp(name, "xmm", 3) == 0 && isdigit((unsigned char)name[3])) {
        int r = atoi(name + 3);
        if (r < 0 || r > 15) return false;
        op->kind = ASM_XMM;
        op->reg = r;
        op->width = 16;
        return true;
    }
    return false;
}

static const char *asm_skip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static bool asm_parse_number(const char *s, int length, int64_t *value) {
    char text[40];
    if (length <= 0 || length >= (int)sizeof(text)) return false;
    memcpy(text, s, length);
    text[length] = 0;
    char *end;
    if (text[0] == '-' || isdigit((unsigned char)text[0])) {
        *value = (int64_t) strtoull(text[0] == '-' ? text + 1 : text, &end, 0);
        if (text[0] == '-') *value = -*value;
        return *end == 0;
    }
    return false;
}

// A symbol, e.g '$fmt$StringBuilder.add' (the '$' is left out) or '.L3' (made into 'function.L3')
static void asm_symbol_name(const char *s, int length, const char *scope, char *out, int out_size) {
    if (s[0] == '$') { s++; length--; }
    if (s[0] == '.' && scope) snprintf(out, out_size, "%s%.*s", scope, length, s);
    else                      snprintf(out, out_size, "%.*s", length, s);
}

// The inside of [...]: 'rbp-8', 'rax', 'r8 + r9', 'rel $name+8'
static bool asm_parse_memory(const char *s, int length, AsmOperand *op, const char *scope) {
    op->kind  = ASM_MEM;
    op->base  = -1;
    op->index = -1;
    op->disp  = 0;
    op->symbol[0] = 0;

    char text[256];
    int n = 0;
    for (int i = 0; i < length && n < (int)sizeof(text) - 1; i++) {
        if (s[i] != ' ' && s[i] != '\t') text[n++] = s[i];
    }
    text[n] = 0;

    char *p = text;
    if (strncmp(p, "rel", 3) == 0) {
        // rip-relative: rel$name+8
        p += 3;
        char *end = p;
        while (*end && *end != '+' && *end != '-') end++;
        asm_symbol_name(p, (int)(end - p), scope, op->symbol, sizeof(op->symbol));
        if (*end) {
            if (!asm_parse_number(end + (*end == '+'), (int)strlen(end + (*end == '+')), &op->disp)) return false;
        }
        return true;
    }

    // base, then +index or +disp or -disp
    char *end = p;
    while (*end && *end != '+' && *end != '-') end++;
    AsmOperand reg = {0};
    if (!asm_parse_register(p, (int)(end - p), &reg) || reg.kind != ASM_REG || reg.width != 8) return false;
    op->base = reg.reg;
    p = end;
    while (*p) {
        char sign = *p++;
        end = p;
        while (*end && *end != '+' && *end != '-') end++;
        AsmOperand other = {0};
        if (sign == '+' && asm_parse_register(p, (int)(end - p), &other)) {
            if (other.kind != ASM_REG || other.width != 8 || op->index >= 0 || other.reg == 4) return false;
            op->index = other.reg;
        } else {
            int64_t value;
            if (!asm_parse_number(p, (int)(end - p), &value)) return false;
            op->disp += sign == '-' ? -value : value;
        }
        p = end;
    }
    return true;
}

static bool asm_parse_operand(const char *s, int length, AsmOperand *op, const char *scope) {
    memset(op, 0, sizeof(*op));
    while (length > 0 && (s[length - 1] == ' ' || s[length - 1] == '\t')) length--;
    while (length > 0 && (*s == ' ' || *s == '\t')) { s++; length--; }
    if (length <= 0) return false;

    // A size keyword before a memory operand
    static const struct { const char *word; int width; } sizes[] = { {"byte", 1}, {"word", 2}, {"dword", 4}, {"qword", 8} };
    int width = 0;
    for (int i = 0; i < 4; i++) {
        int l = (int)strlen(sizes[i].word);
        if (length > l && strncmp(s, sizes[i].word, l) == 0 && (s[l] == ' ' || s[l] == '[')) {
            width = sizes[i].width;
            s += l;
            length -= l;
            while (length > 0 && *s == ' ') { s++; length--; }
            break;
        }
    }

    if (s[0] == '[') {
        if (s[length - 1] != ']') return false;
        if (!asm_parse_memory(s + 1, length - 2, op, scope)) return false;
        op->width = width;
        return true;
    }
    if (width) return false;
    if (asm_parse_register(s, length, op)) return true;
    if (asm_parse_number(s, length, &op->imm)) {
        op->kind = ASM_IMM;
        return true;
    }
    op->kind = ASM_LABEL;
    asm_symbol_name(s, length, scope, op->symbol, sizeof(op->symbol));
    return true;
}

//
// Encoding
//

// An instruction of the text section. Jumps to labels are sized after all of them are known (see asm_layout)
typedef struct AsmItem {
    int      start;        // In the bytes of the items
    int      size;
    bool     is_jump;
    int      condition;    // -1 for jmp, otherwise the condition code of jcc
    int      target;       // The symbol a jump goes to
    bool     is_short;
    int      reloc_start;  // The relocations of the instruction, in the pending list
    int      reloc_count;
    int      line;
} AsmItem;

typedef struct AsmEncoder {
    AsmObject *obj;
    uint8_t    bytes[32];
    int        size;
    AsmReloc   relocs[2];   // Offsets are in the instruction, fixed when it is placed
    int        reloc_count;
    int        rip_field;   // Where a rip-relative displacement is, or -1
} AsmEncoder;

static void enc_byte(AsmEncoder *e, uint8_t b) { e->bytes[e->size++] = b; }

static void enc_imm(AsmEncoder *e, int64_t value, int size) {
    for (int i = 0; i < size; i++) enc_byte(e, (uint8_t)(value >> (8 * i)));
}

static bool fits_i8(int64_t v)  { return v >= -128 && v <= 127; }
static bool fits_i32(int64_t v) { return v >= INT32_MIN && v <= INT32_MAX; }

// Writes the prefixes, the REX prefix, the opcode, the ModRM byte and what follows it. 'reg' is the reg field (a
// register or an opcode extension), 'rm' a register or memory operand. 'prefix' is 0x66, 0xF2, 0xF3 or 0
static bool enc_modrm(AsmEncoder *e, int prefix, bool rex_w, const uint8_t *opcode, int opcode_size, int reg, bool reg_high8,
                      const AsmOperand *rm) {
    if (prefix) enc_byte(e, (uint8_t)prefix);

    uint8_t rex = 0x40 | (rex_w ? 8 : 0) | ((reg & 8) ? 4 : 0);
    bool need_rex = rex_w || (reg & 8) || reg_high8;
    if (rm->kind == ASM_REG || rm->kind == ASM_XMM) {
        if (rm->reg & 8) { rex |= 1; need_rex = true; }
        if (rm->kind == ASM_REG && rm->high8) need_rex = true;
    } else if (rm->kind == ASM_MEM) {
        if (rm->base >= 0 && (rm->base & 8))  { rex |= 1; need_rex = true; }
        if (rm->index >= 0 && (rm->index & 8)) { rex |= 2; need_rex = true; }
    } else {
        return false;
    }
    if (need_rex) enc_byte(e, rex);
    for (int i = 0; i < opcode_size; i++) enc_byte(e, opcode[i]);

    int r = reg & 7;
    if (rm->kind == ASM_REG || rm->kind == ASM_XMM) {
        enc_byte(e, (uint8_t)(0xC0 | (r << 3) | (rm->reg & 7)));
        return true;
    }

    // Memory
    if (rm->base < 0) {
        // [rel symbol + disp]
        enc_byte(e, (uint8_t)(0x05 | (r << 3)));
        e->rip_field = e->size;
        e->relocs[e->reloc_count++] = (AsmReloc){ .offset = e->size, .kind = ASM_RELOC_REL32,
                                                   .symbol = asm_symbol(e->obj, rm->symbol), .addend = rm->disp };
        enc_imm(e, 0, 4);
        return true;
    }
    int base = rm->base & 7;
    int64_t disp = rm->disp;
    if (!fits_i32(disp)) return false;
    int mod;
    if (disp == 0 && base != 5) mod = 0;   // rbp and r13 always have a displacement
    else if (fits_i8(disp))     mod = 1;
    else                        mod = 2;

    if (rm->index >= 0) {
        enc_byte(e, (uint8_t)((mod << 6) | (r << 3) | 4));
        enc_byte(e, (uint8_t)(((rm->index & 7) << 3) | base)); // scale 1
    } else if (base == 4) {
        // rsp and r12 need a SIB byte
        enc_byte(e, (uint8_t)((mod << 6) | (r << 3) | 4));
        enc_byte(e, 0x24);
    } else {
        enc_byte(e, (uint8_t)((mod << 6) | (r << 3) | base));
    }
    if (mod == 1) enc_imm(e, disp, 1);
    if (mod == 2) enc_imm(e, disp, 4);
    return true;
}

// The bytes after a rip-relative displacement, e.g an immediate
static void enc_finish(AsmEncoder *e) {
    if (e->rip_field >= 0) {
        for (int i = 0; i < e->reloc_count; i++) {
            if (e->relocs[i].offset == e->rip_field) e->relocs[i].trailing = e->size - (e->rip_field + 4);
        }
    }
}

static bool is_gpr(const AsmOperand *op)  { return op->kind == ASM_REG; }
static bool is_xmm(const AsmOperand *op)  { return op->kind == ASM_XMM; }
static bool is_mem(const AsmOperand *op)  { return op->kind == ASM_MEM; }
static bool is_rm(const AsmOperand *op)   { return op->kind == ASM_REG || op->kind == ASM_MEM; }
static bool is_xm(const AsmOperand *op)   { return op->kind == ASM_XMM || op->kind == ASM_MEM; }

// The width of an instruction from its operands
static int asm_width(const AsmOperand *a, const AsmOperand *b) {
    if (a && a->kind == ASM_REG) return a->width;
    if (b && b->kind == ASM_REG) return b->width;
    if (a && a->kind == ASM_MEM && a->width) return a->width;
    if (b && b->kind == ASM_MEM && b->width) return b->width;
    return 0;
}

// An instruction on a general purpose register or memory of a width: the 0x66 prefix for 16 bits, REX.W for 64
static bool enc_gpr(AsmEncoder *e, int width, uint8_t opcode8, uint8_t opcode, int reg, bool reg_high8, const AsmOperand *rm) {
    uint8_t op = width == 1 ? opcode8 : opcode;
    return enc_modrm(e, width == 2 ? 0x66 : 0, width == 8, &op, 1, reg, reg_high8, rm);
}

static int asm_condition(const char *cc) {
    static const struct { const char *name; int code; } codes[] = {
        {"o", 0}, {"no", 1}, {"b", 2}, {"c", 2}, {"nae", 2}, {"ae", 3}, {"nb", 3}, {"nc", 3}, {"e", 4}, {"z", 4},
        {"ne", 5}, {"nz", 5}, {"be", 6}, {"na", 6}, {"a", 7}, {"nbe", 7}, {"s", 8}, {"ns", 9}, {"p", 10}, {"pe", 10},
        {"np", 11}, {"po", 11}, {"l", 12}, {"nge", 12}, {"ge", 13}, {"nl", 13}, {"le", 14}, {"ng", 14}, {"g", 15}, {"nle", 15},
    };
    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        if (strcmp(cc, codes[i].name) == 0) return codes[i].code;
    }
    return -1;
}

// The SSE instructions: a prefix, 0F and an opcode, with xmm registers and memory
typedef struct AsmSse {
    const char *name;
    int     prefix;
    uint8_t opcode;
} AsmSse;

static const AsmSse asm_sse_arith[] = {
    {"addsd", 0xF2, 0x58}, {"addss", 0xF3, 0x58}, {"subsd", 0xF2, 0x5C}, {"subss", 0xF3, 0x5C},
    {"mulsd", 0xF2, 0x59}, {"mulss", 0xF3, 0x59}, {"divsd", 0xF2, 0x5E}, {"divss", 0xF3, 0x5E},
    {"ucomisd", 0x66, 0x2E}, {"ucomiss", 0, 0x2E}, {"xorps", 0, 0x57},
    {"cvtss2sd", 0xF3, 0x5A}, {"cvtsd2ss", 0xF2, 0x5A}, {"sqrtsd", 0xF2, 0x51}, {"sqrtss", 0xF3, 0x51},
};

// Encodes one instruction. Returns false if it isn't known
static bool asm_encode(AsmEncoder *e, const char *m, AsmOperand *ops, int n) {
    AsmOperand *a = n > 0 ? &ops[0] : NULL;
    AsmOperand *b = n > 1 ? &ops[1] : NULL;

    // No operands
    if (n == 0) {
        if (strcmp(m, "ret") == 0)   { enc_byte(e, 0xC3); return true; }
        if (strcmp(m, "leave") == 0) { enc_byte(e, 0xC9); return true; }
        if (strcmp(m, "cqo") == 0)   { enc_byte(e, 0x48); enc_byte(e, 0x99); return true; }
        if (strcmp(m, "cdq") == 0)   { enc_byte(e, 0x99); return true; }
        if (strcmp(m, "nop") == 0)   { enc_byte(e, 0x90); return true; }
        return false;
    }

    // add, or, and, sub, xor, cmp
    static const char *alu[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
    for (int op = 0; op < 8; op++) {
        if (strcmp(m, alu[op]) != 0) continue;
        if (n != 2) return false;
        int width = asm_width(a, b);
        if (is_rm(a) && is_gpr(b)) return enc_gpr(e, width, (uint8_t)(op * 8), (uint8_t)(op * 8 + 1), b->reg, b->high8, a);
        if (is_gpr(a) && is_mem(b)) return enc_gpr(e, width, (uint8_t)(op * 8 + 2), (uint8_t)(op * 8 + 3), a->reg, a->high8, b);
        if (is_rm(a) && b->kind == ASM_IMM) {
            if (width == 0) return false;
            if (width == 1) {
                if (!enc_gpr(e, 1, 0x80, 0x80, op, false, a)) return false;
                enc_imm(e, b->imm, 1);
            } else if (fits_i8(b->imm)) {
                if (!enc_gpr(e, width, 0x83, 0x83, op, false, a)) return false;
                enc_imm(e, b->imm, 1);
            } else if (is_gpr(a) && a->reg == 0) {
                // The short form for rax/eax/ax
                if (width == 2) enc_byte(e, 0x66);
                if (width == 8) enc_byte(e, 0x48);
                enc_byte(e, (uint8_t)(op * 8 + 5));
                enc_imm(e, b->imm, width == 2 ? 2 : 4);
            } else {
                if (!enc_gpr(e, width, 0x81, 0x81, op, false, a)) return false;
                enc_imm(e, b->imm, width == 2 ? 2 : 4);
            }
            enc_finish(e);
            return true;
        }
        return false;
    }

    if (strcmp(m, "test") == 0 && n == 2 && is_rm(a) && is_gpr(b)) {
        return enc_gpr(e, asm_width(a, b), 0x84, 0x85, b->reg, b->high8, a);
    }

    if (strcmp(m, "mov") == 0 && n == 2) {
        int width = asm_width(a, b);
        if (is_rm(a) && is_gpr(b)) return enc_gpr(e, width, 0x88, 0x89, b->reg, b->high8, a);
        if (is_gpr(a) && is_mem(b)) return enc_gpr(e, width, 0x8A, 0x8B, a->reg, a->high8, b);
        if (is_gpr(a) && b->kind == ASM_IMM) {
            int64_t imm = b->imm;
            if (width == 8 && imm >= 0 && imm <= 0xFFFFFFFFLL) width = 4;   // mov eax, imm32 zeroes the upper half
            if (width == 8 && fits_i32(imm)) {
                AsmOperand rm = *a;
                if (!enc_gpr(e, 8, 0xC7, 0xC7, 0, false, &rm)) return false;
                enc_imm(e, imm, 4);
                return true;
            }
            if (width == 2) enc_byte(e, 0x66);
            if (width == 8 || (a->reg & 8) || a->high8) enc_byte(e, (uint8_t)(0x40 | (width == 8 ? 8 : 0) | ((a->reg & 8) ? 1 : 0)));
            enc_byte(e, (uint8_t)((width == 1 ? 0xB0 : 0xB8) + (a->reg & 7)));
            enc_imm(e, imm, width == 8 ? 8 : width);
            return true;
        }
        if (is_mem(a) && b->kind == ASM_IMM) {
            if (width == 0) return false;
            if (!enc_gpr(e, width, 0xC6, 0xC7, 0, false, a)) return false;
            enc_imm(e, b->imm, width == 1 ? 1 : width == 2 ? 2 : 4);
            enc_finish(e);
            return true;
        }
        return false;
    }

    if (strcmp(m, "lea") == 0 && n == 2 && is_gpr(a) && is_mem(b)) {
        uint8_t op = 0x8D;
        return enc_modrm(e, a->width == 2 ? 0x66 : 0, a->width == 8, &op, 1, a->reg, false, b);
    }

    if ((strcmp(m, "movzx") == 0 || strcmp(m, "movsx") == 0) && n == 2 && is_gpr(a) && is_rm(b)) {
        int from = b->kind == ASM_REG ? b->width : b->width;
        if (from != 1 && from != 2) return false;
        uint8_t op[2] = { 0x0F, (uint8_t)((m[3] == 'z' ? 0xB6 : 0xBE) + (from == 2 ? 1 : 0)) };
        return enc_modrm(e, a->width == 2 ? 0x66 : 0, a->width == 8, op, 2, a->reg, false, b) ;
    }
    if (strcmp(m, "movsxd") == 0 && n == 2 && is_gpr(a) && is_rm(b)) {
        uint8_t op = 0x63;
        return enc_modrm(e, 0, true, &op, 1, a->reg, false, b);
    }

    if (strcmp(m, "imul") == 0 && n == 2 && is_gpr(a) && is_rm(b)) {
        uint8_t op[2] = { 0x0F, 0xAF };
        return enc_modrm(e, a->width == 2 ? 0x66 : 0, a->width == 8, op, 2, a->reg, false, b);
    }

    // neg, not, mul, div, idiv, inc, dec
    static const struct { const char *name; uint8_t opcode8, opcode; int ext; } unary[] = {
        {"not", 0xF6, 0xF7, 2}, {"neg", 0xF6, 0xF7, 3}, {"mul", 0xF6, 0xF7, 4}, {"div", 0xF6, 0xF7, 6},
        {"idiv", 0xF6, 0xF7, 7}, {"inc", 0xFE, 0xFF, 0}, {"dec", 0xFE, 0xFF, 1},
    };
    for (size_t i = 0; i < sizeof(unary) / sizeof(unary[0]); i++) {
        if (strcmp(m, unary[i].name) != 0) continue;
        if (n != 1 || !is_rm(a)) return false;
        int width = asm_width(a, NULL);
        if (width == 0) return false;
        return enc_gpr(e, width, unary[i].opcode8, unary[i].opcode, unary[i].ext, false, a);
    }

    // shl, shr, sar by cl or by a number
    static const struct { const char *name; int ext; } shifts[] = { {"rol", 0}, {"ror", 1}, {"shl", 4}, {"sal", 4}, {"shr", 5}, {"sar", 7} };
    for (size_t i = 0; i < sizeof(shifts) / sizeof(shifts[0]); i++) {
        if (strcmp(m, shifts[i].name) != 0) continue;
        if (n != 2 || !is_rm(a)) return false;
        int width = asm_width(a, NULL);
        if (width == 0) return false;
        if (b->kind == ASM_REG && b->reg == 1 && b->width == 1) return enc_gpr(e, width, 0xD2, 0xD3, shifts[i].ext, false, a);
        if (b->kind == ASM_IMM && b->imm == 1) return enc_gpr(e, width, 0xD0, 0xD1, shifts[i].ext, false, a);
        if (b->kind == ASM_IMM) {
            if (!enc_gpr(e, width, 0xC0, 0xC1, shifts[i].ext, false, a)) return false;
            enc_imm(e, b->imm, 1);
            enc_finish(e);
            return true;
        }
        return false;
    }

    // bt, bts, btr, btc with a number
    static const struct { const char *name; int ext; } bits[] = { {"bt", 4}, {"bts", 5}, {"btr", 6}, {"btc", 7} };
    for (size_t i = 0; i < sizeof(bits) / sizeof(bits[0]); i++) {
        if (strcmp(m, bits[i].name) != 0) continue;
        if (n != 2 || !is_rm(a) || b->kind != ASM_IMM) return false;
        int width = asm_width(a, NULL);
        uint8_t op[2] = { 0x0F, 0xBA };
        if (!enc_modrm(e, width == 2 ? 0x66 : 0, width == 8, op, 2, bits[i].ext, false, a)) return false;
        enc_imm(e, b->imm, 1);
        enc_finish(e);
        return true;
    }

    // setcc
    if (strncmp(m, "set", 3) == 0 && n == 1 && is_rm(a)) {
        int cc = asm_condition(m + 3);
        if (cc < 0) return false;
        uint8_t op[2] = { 0x0F, (uint8_t)(0x90 + cc) };
        return enc_modrm(e, 0, false, op, 2, 0, false, a);
    }

    if (strcmp(m, "push") == 0 && n == 1 && is_gpr(a) && a->width == 8) {
        if (a->reg & 8) enc_byte(e, 0x41);
        enc_byte(e, (uint8_t)(0x50 + (a->reg & 7)));
        return true;
    }
    if (strcmp(m, "pop") == 0 && n == 1 && is_gpr(a) && a->width == 8) {
        if (a->reg & 8) enc_byte(e, 0x41);
        enc_byte(e, (uint8_t)(0x58 + (a->reg & 7)));
        return true;
    }

    if (strcmp(m, "call") == 0 && n == 1) {
        if (is_rm(a)) {
            uint8_t op = 0xFF;
            return enc_modrm(e, 0, false, &op, 1, 2, false, a);
        }
        if (a->kind == ASM_LABEL) {
            enc_byte(e, 0xE8);
            e->relocs[e->reloc_count++] = (AsmReloc){ .offset = e->size, .kind = ASM_RELOC_REL32, .symbol = asm_symbol(e->obj, a->symbol) };
            enc_imm(e, 0, 4);
            return true;
        }
        return false;
    }

    // SSE moves
    if ((strcmp(m, "movsd") == 0 || strcmp(m, "movss") == 0) && n == 2) {
        int prefix = m[4] == 'd' ? 0xF2 : 0xF3;
        if (is_xmm(a) && is_xm(b)) { uint8_t op[2] = { 0x0F, 0x10 }; return enc_modrm(e, prefix, false, op, 2, a->reg, false, b); }
        if (is_mem(a) && is_xmm(b)) { uint8_t op[2] = { 0x0F, 0x11 }; return enc_modrm(e, prefix, false, op, 2, b->reg, false, a); }
        return false;
    }
    if ((strcmp(m, "movaps") == 0 || strcmp(m, "movups") == 0) && n == 2) {
        uint8_t load = m[3] == 'a' ? 0x28 : 0x10;
        if (is_xmm(a) && is_xm(b)) { uint8_t op[2] = { 0x0F, load }; return enc_modrm(e, 0, false, op, 2, a->reg, false, b); }
        if (is_mem(a) && is_xmm(b)) { uint8_t op[2] = { 0x0F, (uint8_t)(load + 1) }; return enc_modrm(e, 0, false, op, 2, b->reg, false, a); }
        return false;
    }
    if ((strcmp(m, "movq") == 0 || strcmp(m, "movd") == 0) && n == 2) {
        bool wide = m[3] == 'q';
        if (is_xmm(a) && is_rm(b)) { uint8_t op[2] = { 0x0F, 0x6E }; return enc_modrm(e, 0x66, wide, op, 2, a->reg, false, b); }
        if (is_rm(a) && is_xmm(b)) { uint8_t op[2] = { 0x0F, 0x7E }; return enc_modrm(e, 0x66, wide, op, 2, b->reg, false, a); }
        return false;
    }

    // SSE arithmetic, compares and conversions between floats
    for (size_t i = 0; i < sizeof(asm_sse_arith) / sizeof(asm_sse_arith[0]); i++) {
        if (strcmp(m, asm_sse_arith[i].name) != 0) continue;
        if (n != 2 || !is_xmm(a) || !is_xm(b)) return false;
        uint8_t op[2] = { 0x0F, asm_sse_arith[i].opcode };
        if (!enc_modrm(e, asm_sse_arith[i].prefix, false, op, 2, a->reg, false, b)) return false;
        return true;
    }

    // Conversions between integers and floats
    if ((strcmp(m, "cvtsi2sd") == 0 || strcmp(m, "cvtsi2ss") == 0) && n == 2 && is_xmm(a) && is_rm(b)) {
        uint8_t op[2] = { 0x0F, 0x2A };
        return enc_modrm(e, m[7] == 'd' ? 0xF2 : 0xF3, asm_width(b, NULL) == 8, op, 2, a->reg, false, b);
    }
    if ((strcmp(m, "cvttsd2si") == 0 || strcmp(m, "cvttss2si") == 0) && n == 2 && is_gpr(a) && is_xm(b)) {
        uint8_t op[2] = { 0x0F, 0x2C };
        return enc_modrm(e, m[5] == 'd' ? 0xF2 : 0xF3, a->width == 8, op, 2, a->reg, false, b);
    }

    return false;
}

//
// Assembling a text
//

typedef struct Assembler {
    AsmObject *obj;
    int        section;
    char       scope[160];   // The last label that doesn't start with '.'

    // The text section is made of items, as jumps get their size at the end
    AsmItem   *items;
    int        item_count;
    int        item_capacity;
    AsmBytes   item_bytes;
    AsmReloc  *pending;      // The relocations of the items, with offsets in item_bytes
    int        pending_count;
    int        pending_capacity;
    int       *label_item;   // For each symbol: the item a text label is before, or -1
    int        label_capacity;
} Assembler;

static void asm_add_item(Assembler *as, AsmItem item) {
    if (as->item_count == as->item_capacity) {
        as->item_capacity = as->item_capacity ? as->item_capacity * 2 : 4096;
        as->items = realloc(as->items, as->item_capacity * sizeof(AsmItem));
    }
    as->items[as->item_count++] = item;
}

static void asm_add_pending(Assembler *as, AsmReloc reloc) {
    if (as->pending_count == as->pending_capacity) {
        as->pending_capacity = as->pending_capacity ? as->pending_capacity * 2 : 4096;
        as->pending = realloc(as->pending, as->pending_capacity * sizeof(AsmReloc));
    }
    as->pending[as->pending_count++] = reloc;
}

static void asm_set_label_item(Assembler *as, int symbol, int item) {
    if (symbol >= as->label_capacity) {
        int capacity = as->label_capacity ? as->label_capacity : 1024;
        while (capacity <= symbol) capacity *= 2;
        as->label_item = realloc(as->label_item, capacity * sizeof(int));
        for (int i = as->label_capacity; i < capacity; i++) as->label_item[i] = -1;
        as->label_capacity = capacity;
    }
    as->label_item[symbol] = item;
}

static bool asm_fail(Assembler *as, int line, const char *message, const char *text) {
    snprintf(as->obj->error, sizeof(as->obj->error), "line %d: %s: %s", line, message, text);
    return false;
}

static void asm_align(AsmBytes *bytes, int align, uint8_t fill) {
    while (align > 1 && bytes->size % align != 0) asm_put_byte(bytes, fill);
}

static bool asm_data_line(Assembler *as, const char *directive, const char *rest, int line, const char *text) {
    AsmBytes *bytes = &as->obj->sections[as->section];
    if (strcmp(directive, "align") == 0) {
        int align = atoi(rest);
        if (align > as->obj->alignment[as->section]) as->obj->alignment[as->section] = align;
        if (as->section == ASM_TEXT) return asm_fail(as, line, "align in the text section isn't supported", text);
        asm_align(bytes, align, 0);
        return true;
    }
    if (strcmp(directive, "times") == 0) {
        // times N db 0
        int count = atoi(rest);
        const char *db = strstr(rest, "db");
        if (!db) return asm_fail(as, line, "times only takes db", text);
        int value = atoi(db + 2);
        for (int i = 0; i < count; i++) asm_put_byte(bytes, (uint8_t)value);
        return true;
    }

    int size = strcmp(directive, "db") == 0 ? 1 : strcmp(directive, "dw") == 0 ? 2 : strcmp(directive, "dd") == 0 ? 4 : 8;
    const char *p = rest;
    while (*p) {
        p = asm_skip_spaces(p);
        const char *end = p;
        while (*end && *end != ',') end++;
        int length = (int)(end - p);
        while (length > 0 && (p[length - 1] == ' ' || p[length - 1] == '\t' || p[length - 1] == '\n' || p[length - 1] == '\r')) length--;
        if (length > 0) {
            int64_t value;
            if (asm_parse_number(p, length, &value)) {
                for (int i = 0; i < size; i++) asm_put_byte(bytes, (uint8_t)(value >> (8 * i)));
            } else if (size == 8) {
                // The address of a symbol
                char name[160];
                asm_symbol_name(p, length, as->scope, name, sizeof(name));
                asm_add_reloc(as->obj, (AsmReloc){ .section = as->section, .offset = bytes->size, .kind = ASM_RELOC_ABS64, .symbol = asm_symbol(as->obj, name) });
                for (int i = 0; i < 8; i++) asm_put_byte(bytes, 0);
            } else {
                return asm_fail(as, line, "a value that isn't a number", text);
            }
        }
        p = *end ? end + 1 : end;
    }
    return true;
}

static bool asm_instruction(Assembler *as, const char *mnemonic, const char *rest, int line, const char *text) {
    if (as->section != ASM_TEXT) return asm_fail(as, line, "an instruction outside of the text section", text);

    AsmOperand ops[3];
    int n = 0;
    const char *p = asm_skip_spaces(rest);
    while (*p && *p != ';' && n < 3) {
        const char *end = p;
        int depth = 0;
        while (*end && *end != ';' && !(*end == ',' && depth == 0)) {
            if (*end == '[') depth++;
            if (*end == ']') depth--;
            end++;
        }
        int length = (int)(end - p);
        while (length > 0 && (p[length - 1] == '\n' || p[length - 1] == '\r')) length--;
        if (!asm_parse_operand(p, length, &ops[n], as->scope)) return asm_fail(as, line, "an operand that the assembler doesn't know", text);
        n++;
        p = *end == ',' ? asm_skip_spaces(end + 1) : end;
    }

    // Jumps to labels are sized later
    bool is_jmp = strcmp(mnemonic, "jmp") == 0;
    int  cc = mnemonic[0] == 'j' && !is_jmp ? asm_condition(mnemonic + 1) : -1;
    if ((is_jmp || cc >= 0) && n == 1 && ops[0].kind == ASM_LABEL) {
        asm_add_item(as, (AsmItem){ .start = as->item_bytes.size, .is_jump = true, .condition = is_jmp ? -1 : cc,
                                    .target = asm_symbol(as->obj, ops[0].symbol), .is_short = true, .line = line });
        return true;
    }

    AsmEncoder e = { .obj = as->obj, .rip_field = -1 };
    if (!asm_encode(&e, mnemonic, ops, n)) return asm_fail(as, line, "an instruction that the assembler doesn't know", text);
    enc_finish(&e);

    AsmItem item = { .start = as->item_bytes.size, .size = e.size, .condition = -1, .reloc_start = as->pending_count, .reloc_count = e.reloc_count, .line = line };
    for (int i = 0; i < e.reloc_count; i++) {
        AsmReloc r = e.relocs[i];
        r.offset += as->item_bytes.size;
        asm_add_pending(as, r);
    }
    asm_put(&as->item_bytes, e.bytes, e.size);
    asm_add_item(as, item);
    return true;
}

// Gives the jumps their sizes, starting short and making the ones that don't reach long until nothing changes
static void asm_layout(Assembler *as, int *offsets) {
    bool changed = true;
    while (changed) {
        changed = false;
        int offset = 0;
        for (int i = 0; i < as->item_count; i++) {
            AsmItem *item = &as->items[i];
            offsets[i] = offset;
            if (item->is_jump) item->size = item->is_short ? 2 : (item->condition < 0 ? 5 : 6);
            offset += item->size;
        }
        offsets[as->item_count] = offset;

        for (int i = 0; i < as->item_count; i++) {
            AsmItem *item = &as->items[i];
            if (!item->is_jump || !item->is_short) continue;
            int target = item->target < as->label_capacity ? as->label_item[item->target] : -1;
            if (target < 0) { item->is_short = false; changed = true; continue; }
            int64_t displacement = offsets[target] - (offsets[i] + 2);
            if (!fits_i8(displacement)) { item->is_short = false; changed = true; }
        }
    }
}

static bool asm_finish_text(Assembler *as) {
    int *offsets = malloc((as->item_count + 1) * sizeof(int));
    asm_layout(as, offsets);

    AsmBytes *text = &as->obj->sections[ASM_TEXT];
    for (int i = 0; i < as->item_count; i++) {
        AsmItem *item = &as->items[i];
        if (item->is_jump) {
            int target = item->target < as->label_capacity ? as->label_item[item->target] : -1;
            if (target < 0) {
                snprintf(as->obj->error, sizeof(as->obj->error), "line %d: a jump to '%s', which isn't a label of the code", item->line, as->obj->symbols[item->target].name);
                free(offsets);
                return false;
            }
            int64_t displacement = offsets[target] - (offsets[i] + item->size);
            if (item->is_short) {
                asm_put_byte(text, item->condition < 0 ? 0xEB : (uint8_t)(0x70 + item->condition));
                asm_put_byte(text, (uint8_t)displacement);
            } else {
                if (item->condition < 0) asm_put_byte(text, 0xE9);
                else { asm_put_byte(text, 0x0F); asm_put_byte(text, (uint8_t)(0x80 + item->condition)); }
                int32_t d = (int32_t)displacement;
                asm_put(text, &d, 4);
            }
            continue;
        }
        int start = text->size;
        asm_put(text, &as->item_bytes.data[item->start], item->size);
        for (int r = 0; r < item->reloc_count; r++) {
            AsmReloc reloc = as->pending[item->reloc_start + r];
            reloc.section = ASM_TEXT;
            reloc.offset  = start + (reloc.offset - item->start);
            asm_add_reloc(as->obj, reloc);
        }
    }

    // The labels of the text get their offsets
    for (int s = 0; s < as->obj->symbol_count && s < as->label_capacity; s++) {
        if (as->label_item[s] >= 0) {
            as->obj->symbols[s].section = ASM_TEXT;
            as->obj->symbols[s].offset  = offsets[as->label_item[s]];
        }
    }
    free(offsets);
    return true;
}

// Assembles the text that x64_emit.c writes. On an error, obj->error says what and where
bool x64_assemble(const char *source, int length, AsmObject *obj) {
    memset(obj, 0, sizeof(*obj));
    for (int s = 0; s < ASM_SECTION_COUNT; s++) obj->alignment[s] = s == ASM_TEXT ? 16 : 8;

    Assembler as = { .obj = obj, .section = ASM_TEXT };
    int line_number = 0;
    const char *p = source;
    const char *end_of_source = source + length;
    char line[1024];

    while (p < end_of_source) {
        const char *eol = memchr(p, '\n', end_of_source - p);
        if (!eol) eol = end_of_source;
        int line_length = (int)(eol - p);
        line_number++;
        if (line_length >= (int)sizeof(line)) {
            snprintf(obj->error, sizeof(obj->error), "line %d is too long", line_number);
            return false;
        }
        memcpy(line, p, line_length);
        line[line_length] = 0;
        p = eol + 1;

        // Comments
        char *comment = NULL;
        bool in_string = false;
        for (char *c = line; *c; c++) {
            if (*c == '"' || *c == '\'') in_string = !in_string;
            if (*c == ';' && !in_string) { comment = c; break; }
        }
        if (comment) *comment = 0;
        int n = (int)strlen(line);
        while (n > 0 && (line[n - 1] == ' ' || line[n - 1] == '\t' || line[n - 1] == '\r')) line[--n] = 0;
        const char *text = asm_skip_spaces(line);
        if (!*text) continue;

        // A label
        if (line[0] != ' ' && line[0] != '\t' && line[n - 1] == ':') {
            char name[160];
            asm_symbol_name(line, n - 1, as.scope, name, sizeof(name));
            if (line[0] != '.') snprintf(as.scope, sizeof(as.scope), "%s", name);
            int symbol = asm_symbol(obj, name);
            if (obj->symbols[symbol].section >= 0 || (symbol < as.label_capacity && as.label_item[symbol] >= 0)) {
                return asm_fail(&as, line_number, "a label that is already defined", name);
            }
            if (as.section == ASM_TEXT) {
                asm_set_label_item(&as, symbol, as.item_count);
            } else {
                obj->symbols[symbol].section = as.section;
                obj->symbols[symbol].offset  = obj->sections[as.section].size;
            }
            continue;
        }

        // The first word and the rest
        char word[32];
        int w = 0;
        while (text[w] && text[w] != ' ' && text[w] != '\t' && w < (int)sizeof(word) - 1) { word[w] = text[w]; w++; }
        word[w] = 0;
        const char *rest = asm_skip_spaces(text + w);

        if (strcmp(word, "bits") == 0 || strcmp(word, "default") == 0) continue;
        if (strcmp(word, "section") == 0) {
            int s = -1;
            for (int i = 0; i < ASM_SECTION_COUNT; i++) if (strcmp(rest, asm_section_names[i]) == 0) s = i;
            if (strcmp(rest, ".rodata") == 0) s = ASM_RDATA;
            if (s < 0) return asm_fail(&as, line_number, "a section that the assembler doesn't know", rest);
            as.section = s;
            continue;
        }
        if (strcmp(word, "extern") == 0 || strcmp(word, "global") == 0) {
            char name[160];
            asm_symbol_name(rest, (int)strlen(rest), NULL, name, sizeof(name));
            int symbol = asm_symbol(obj, name);
            if (word[0] == 'e') obj->symbols[symbol].is_extern = true;
            else                obj->symbols[symbol].is_global = true;
            continue;
        }
        if (strcmp(word, "db") == 0 || strcmp(word, "dw") == 0 || strcmp(word, "dd") == 0 || strcmp(word, "dq") == 0 ||
            strcmp(word, "times") == 0 || strcmp(word, "align") == 0) {
            if (!asm_data_line(&as, word, rest, line_number, text)) return false;
            continue;
        }
        if (!asm_instruction(&as, word, rest, line_number, text)) return false;
    }

    if (!asm_finish_text(&as)) return false;

    // Every symbol that is used is defined, or is an extern
    for (int s = 0; s < obj->symbol_count; s++) {
        AsmSymbol *symbol = &obj->symbols[s];
        if (symbol->section < 0 && !symbol->is_extern) {
            snprintf(obj->error, sizeof(obj->error), "'%s' is used, but it isn't defined or an extern", symbol->name);
            return false;
        }
    }

    free(as.items);
    free(as.item_bytes.data);
    free(as.pending);
    free(as.label_item);
    return true;
}
