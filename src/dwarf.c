//////////////////////////////////////////////////////////////////////////////////////////
//
// Debug information of the x64 backend (saad build --debug): DWARF 4, which LLDB and GDB read on Windows and Linux,
// and the unwind tables of Windows (.pdata, .xdata). It is written into extra sections of the assembled AsmObject,
// after x64_assemble(), as then the addresses of the labels that x64_emit.c put in the code are known.
//
//   * .debug_line    the line of each place in the code, from the marks of x64_emit.c (a label where the line or the
//                    lexical scope changes)
//   * .debug_info    a subprogram per function, with its parameters and variables in their slots (rbp + offset),
//                    lexical blocks for the scopes, and the DWARF types of the Saad types
//   * .debug_ranges  the code of a lexical block, which can be in pieces
//   * .pdata, .xdata on Windows: every function starts with push rbp; mov rbp, rsp, so a debugger finds the caller
//                    through rbp
//
// An address is a relocation to a label, so pe_write() and elf_write() put in where the code is. An offset into
// another debug section is a section relative relocation to its start.
//
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct DwarfWriter {
    AsmObject       *obj;
    IrModule        *module;
    int              info_section, abbrev_section, line_section, ranges_section;
    int              info_start, abbrev_start, line_start, ranges_start;   // Symbols at the start of the sections
    OutBytes         info, abbrev, line, ranges;
    char           **files;          // The source files, the file i + 1 of the line table
    int              file_count;
    int              unit_low;       // Where the code of the unit starts in .text: the ranges are from there

    // The types: each is written once, after the functions. A reference to one is patched in at the end
    PtrMap           type_index;     // Type* -> index in types
    Type           **types;
    int             *type_offsets;   // Of its DIE in the unit, -1 until written
    int              type_count;
    struct { int at; int type; } *fixups;
    int              fixup_count;
} DwarfWriter;

enum {
    DW_ABBREV_COMPILE_UNIT = 1,
    DW_ABBREV_SUBPROGRAM,
    DW_ABBREV_PARAMETER,
    DW_ABBREV_VARIABLE,
    DW_ABBREV_LEXICAL_BLOCK,
    DW_ABBREV_BASE_TYPE,
    DW_ABBREV_POINTER,
    DW_ABBREV_VOID_POINTER,
    DW_ABBREV_STRUCT,
    DW_ABBREV_MEMBER,
    DW_ABBREV_ARRAY,
    DW_ABBREV_SUBRANGE,
    DW_ABBREV_ENUM,
    DW_ABBREV_ENUMERATOR,
    DW_ABBREV_SUBROUTINE,
    DW_ABBREV_SUBPROGRAM_EMPTY,   // Without variables
    DW_ABBREV_STRUCT_EMPTY,       // Without members
};

static void dwarf_uleb(OutBytes *o, uint64_t value) {
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value) byte |= 0x80;
        out_u8(o, byte);
    } while (value);
}

static void dwarf_sleb(OutBytes *o, int64_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if ((value == 0 && !(byte & 0x40)) || (value == -1 && (byte & 0x40))) more = false;
        else byte |= 0x80;
        out_u8(o, byte);
    }
}

static void dwarf_string(OutBytes *o, const char *text) {
    out_put(o, text, strlen(text) + 1);
}

// The AsmObject symbol of a debug label of a function, e.g main.Ldbg3
static int dwarf_label_symbol(DwarfWriter *w, IrFunction *func, int label) {
    char name[512];
    snprintf(name, sizeof(name), "%s.Ldbg%d", w->module->globals.items[func->global]->name, label);
    return asm_symbol(w->obj, name);
}

static int dwarf_label_offset(DwarfWriter *w, IrFunction *func, int label) {
    return w->obj->symbols[dwarf_label_symbol(w, func, label)].offset;
}

// An 8 byte address of a symbol in the section, e.g a label
static void dwarf_address(DwarfWriter *w, OutBytes *o, int section, int symbol) {
    AsmReloc reloc = { .section = section, .offset = (int) o->size, .kind = ASM_RELOC_ABS64, .symbol = symbol };
    asm_add_reloc(w->obj, reloc);
    out_u64(o, 0);
}

// A 4 byte offset into another debug section, whose start is the symbol
static void dwarf_section_offset(DwarfWriter *w, OutBytes *o, int section, int start_symbol, int offset) {
    AsmReloc reloc = { .section = section, .offset = (int) o->size, .kind = ASM_RELOC_SECREL32, .symbol = start_symbol, .addend = offset };
    asm_add_reloc(w->obj, reloc);
    out_u32(o, 0);
}

static int dwarf_file(DwarfWriter *w, const char *path) {
    for (int i = 0; i < w->file_count; i++) if (strcmp(w->files[i], path) == 0) return i + 1;
    w->files = realloc(w->files, (w->file_count + 1) * sizeof(char *));
    w->files[w->file_count++] = strdup(path);
    return w->file_count;
}

// A section of the debug information, with a symbol at its start
static int dwarf_section(DwarfWriter *w, const char *name, uint32_t flags, int *start) {
    int section = coff_extra_section(w->obj, name, flags);
    char symbol[64];
    snprintf(symbol, sizeof(symbol), "%s@start", name);
    *start = asm_symbol(w->obj, symbol);
    w->obj->symbols[*start].section = section;
    w->obj->symbols[*start].offset = 0;
    return section;
}

//
// Abbreviations: the shape of each kind of DIE
//

static void dwarf_abbrev(OutBytes *o, int code, int tag, bool children, const int *attributes) {
    dwarf_uleb(o, code);
    dwarf_uleb(o, tag);
    out_u8(o, children ? 1 : 0);
    for (int i = 0; attributes[i]; i += 2) {
        dwarf_uleb(o, attributes[i]);
        dwarf_uleb(o, attributes[i + 1]);
    }
    out_u8(o, 0);
    out_u8(o, 0);
}

// Attribute and form numbers of DWARF 4
#define DW_AT_name 0x03
#define DW_AT_byte_size 0x0b
#define DW_AT_stmt_list 0x10
#define DW_AT_low_pc 0x11
#define DW_AT_high_pc 0x12
#define DW_AT_language 0x13
#define DW_AT_comp_dir 0x1b
#define DW_AT_const_value 0x1c
#define DW_AT_producer 0x25
#define DW_AT_prototyped 0x27
#define DW_AT_count 0x37
#define DW_AT_data_member_location 0x38
#define DW_AT_decl_file 0x3a
#define DW_AT_decl_line 0x3b
#define DW_AT_encoding 0x3e
#define DW_AT_frame_base 0x40
#define DW_AT_location 0x02
#define DW_AT_type 0x49
#define DW_AT_ranges 0x55
#define DW_FORM_addr 0x01
#define DW_FORM_data2 0x05
#define DW_FORM_data4 0x06
#define DW_FORM_data8 0x07
#define DW_FORM_string 0x08
#define DW_FORM_data1 0x0b
#define DW_FORM_sdata 0x0d
#define DW_FORM_ref4 0x13
#define DW_FORM_sec_offset 0x17
#define DW_FORM_exprloc 0x18
#define DW_FORM_flag_present 0x19

static void dwarf_write_abbrevs(DwarfWriter *w) {
    OutBytes *o = &w->abbrev;
    dwarf_abbrev(o, DW_ABBREV_COMPILE_UNIT, 0x11, true, (int[]){ DW_AT_producer, DW_FORM_string, DW_AT_language, DW_FORM_data2,
        DW_AT_name, DW_FORM_string, DW_AT_comp_dir, DW_FORM_string, DW_AT_stmt_list, DW_FORM_sec_offset,
        DW_AT_low_pc, DW_FORM_addr, DW_AT_high_pc, DW_FORM_data8, 0 });
    dwarf_abbrev(o, DW_ABBREV_SUBPROGRAM, 0x2e, true, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_decl_file, DW_FORM_data2,
        DW_AT_decl_line, DW_FORM_data4, DW_AT_low_pc, DW_FORM_addr, DW_AT_high_pc, DW_FORM_data4, DW_AT_frame_base, DW_FORM_exprloc,
        DW_AT_prototyped, DW_FORM_flag_present, 0 });
    dwarf_abbrev(o, DW_ABBREV_PARAMETER, 0x05, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_decl_file, DW_FORM_data2,
        DW_AT_decl_line, DW_FORM_data4, DW_AT_type, DW_FORM_ref4, DW_AT_location, DW_FORM_exprloc, 0 });
    dwarf_abbrev(o, DW_ABBREV_VARIABLE, 0x34, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_decl_file, DW_FORM_data2,
        DW_AT_decl_line, DW_FORM_data4, DW_AT_type, DW_FORM_ref4, DW_AT_location, DW_FORM_exprloc, 0 });
    dwarf_abbrev(o, DW_ABBREV_LEXICAL_BLOCK, 0x0b, true, (int[]){ DW_AT_ranges, DW_FORM_sec_offset, 0 });
    dwarf_abbrev(o, DW_ABBREV_BASE_TYPE, 0x24, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_encoding, DW_FORM_data1,
        DW_AT_byte_size, DW_FORM_data1, 0 });
    dwarf_abbrev(o, DW_ABBREV_POINTER, 0x0f, false, (int[]){ DW_AT_byte_size, DW_FORM_data1, DW_AT_type, DW_FORM_ref4, 0 });
    dwarf_abbrev(o, DW_ABBREV_VOID_POINTER, 0x0f, false, (int[]){ DW_AT_byte_size, DW_FORM_data1, 0 });
    dwarf_abbrev(o, DW_ABBREV_STRUCT, 0x13, true, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_byte_size, DW_FORM_data4, 0 });
    dwarf_abbrev(o, DW_ABBREV_MEMBER, 0x0d, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_type, DW_FORM_ref4,
        DW_AT_data_member_location, DW_FORM_data4, 0 });
    dwarf_abbrev(o, DW_ABBREV_ARRAY, 0x01, true, (int[]){ DW_AT_type, DW_FORM_ref4, 0 });
    dwarf_abbrev(o, DW_ABBREV_SUBRANGE, 0x21, false, (int[]){ DW_AT_count, DW_FORM_data8, 0 });
    dwarf_abbrev(o, DW_ABBREV_ENUM, 0x04, true, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_byte_size, DW_FORM_data1,
        DW_AT_type, DW_FORM_ref4, 0 });
    dwarf_abbrev(o, DW_ABBREV_ENUMERATOR, 0x28, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_const_value, DW_FORM_sdata, 0 });
    dwarf_abbrev(o, DW_ABBREV_SUBROUTINE, 0x15, false, (int[]){ DW_AT_prototyped, DW_FORM_flag_present, 0 });
    dwarf_abbrev(o, DW_ABBREV_SUBPROGRAM_EMPTY, 0x2e, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_decl_file, DW_FORM_data2,
        DW_AT_decl_line, DW_FORM_data4, DW_AT_low_pc, DW_FORM_addr, DW_AT_high_pc, DW_FORM_data4, DW_AT_frame_base, DW_FORM_exprloc,
        DW_AT_prototyped, DW_FORM_flag_present, 0 });
    dwarf_abbrev(o, DW_ABBREV_STRUCT_EMPTY, 0x13, false, (int[]){ DW_AT_name, DW_FORM_string, DW_AT_byte_size, DW_FORM_data4, 0 });
    out_u8(o, 0);
}

//
// Types
//

// A reference to the DIE of a type (ref4), patched in when the types are written
static void dwarf_type_ref(DwarfWriter *w, Type *type) {
    int index = ptr_map_get(&w->type_index, type);
    if (index < 0) {
        index = w->type_count++;
        w->types = realloc(w->types, w->type_count * sizeof(Type *));
        w->type_offsets = realloc(w->type_offsets, w->type_count * sizeof(int));
        w->types[index] = type;
        w->type_offsets[index] = -1;
        ptr_map_put(&w->type_index, type, index);
    }
    w->fixups = realloc(w->fixups, (w->fixup_count + 1) * sizeof(*w->fixups));
    w->fixups[w->fixup_count].at = (int) w->info.size;
    w->fixups[w->fixup_count].type = index;
    w->fixup_count++;
    out_u32(&w->info, 0);
}

static void dwarf_base_type(DwarfWriter *w, const char *name, int encoding, int size) {
    out_u8(&w->info, DW_ABBREV_BASE_TYPE);
    dwarf_string(&w->info, name);
    out_u8(&w->info, (uint8_t) encoding);
    out_u8(&w->info, (uint8_t) size);
}

// A variant of a union without a value, e.g empty, has no member. Its name is a value of the $tag
static bool dwarf_is_member(AstIdentifier *member) {
    return member->type && !(member->flags & IDENTIFIER_IS_CONSTANT) && member->type->size > 0;
}

static void dwarf_struct(DwarfWriter *w, Type *type, TypeStruct *defn, const char *name) {
    int count = 0;
    if (defn && defn->node && defn->node->members) {
        DynamicArray members = defn->node->members->identifiers;
        for (int i = 0; i < members.count; i++) if (dwarf_is_member(da_get_deref(members, i))) count++;
    }
    out_u8(&w->info, count > 0 ? DW_ABBREV_STRUCT : DW_ABBREV_STRUCT_EMPTY);
    dwarf_string(&w->info, name);
    out_u32(&w->info, (uint32_t)(type->size > 0 ? type->size : 0));
    if (count == 0) return;
    if (defn && defn->node && defn->node->members) {
        DynamicArray members = defn->node->members->identifiers;
        for (int i = 0; i < members.count; i++) {
            AstIdentifier *member = da_get_deref(members, i);
            if (!dwarf_is_member(member)) continue;
            out_u8(&w->info, DW_ABBREV_MEMBER);
            dwarf_string(&w->info, member->name);
            dwarf_type_ref(w, member->type);
            out_u32(&w->info, (uint32_t) member->member_offset);
        }
    }
    out_u8(&w->info, 0);
}

// Something else is shown as its bytes
static void dwarf_bytes(DwarfWriter *w, Type *type) {
    out_u8(&w->info, DW_ABBREV_ARRAY);
    dwarf_type_ref(w, primitive_type(PRIMITIVE_U8));
    out_u8(&w->info, DW_ABBREV_SUBRANGE);
    out_u64(&w->info, (uint64_t)(type->size > 0 ? type->size : 0));
    out_u8(&w->info, 0);
}

// The DIE of a type, like llvm_di_type() of llvm_emit.c makes them
static void dwarf_type(DwarfWriter *w, Type *type) {
    char *name = type_to_str(type);
    if (!name) name = "?";
    int size = type->size > 0 ? type->size : 0;
    switch (type->kind) {
    case TYPE_BOOL:    dwarf_base_type(w, "bool", 0x02, 1); return;
    case TYPE_INTEGER: dwarf_base_type(w, name, is_signed_integer(type) ? 0x05 : 0x07, size ? size : 8); return;
    case TYPE_FLOAT:   dwarf_base_type(w, name, 0x04, size ? size : 8); return;
    case TYPE_POINTER: {
        Type *to = ((TypePointer *) type)->pointer_to;
        if (!to || to->kind == TYPE_VOID) {
            out_u8(&w->info, DW_ABBREV_VOID_POINTER);
            out_u8(&w->info, 8);
        } else {
            out_u8(&w->info, DW_ABBREV_POINTER);
            out_u8(&w->info, 8);
            dwarf_type_ref(w, to);
        }
        return;
    }
    case TYPE_FUNCTION: {
        // A pointer to a subroutine, which follows it
        out_u8(&w->info, DW_ABBREV_POINTER);
        out_u8(&w->info, 8);
        out_u32(&w->info, (uint32_t)(w->info.size + 4));    // The unit starts at 0
        out_u8(&w->info, DW_ABBREV_SUBROUTINE);
        return;
    }
    case TYPE_ENUM: {
        TypeEnum *enum_type = (TypeEnum *) type;
        out_u8(&w->info, DW_ABBREV_ENUM);
        dwarf_string(&w->info, name);
        out_u8(&w->info, (uint8_t)(size ? size : 4));
        dwarf_type_ref(w, enum_type->backing_type ? enum_type->backing_type : primitive_type(PRIMITIVE_I32));
        int count = enum_type->node ? enum_type->node->enumerators.count : 0;
        for (int i = 0; i < count; i++) {
            AstEnumerator *enumerator = da_get_deref(enum_type->node->enumerators, i);
            out_u8(&w->info, DW_ABBREV_ENUMERATOR);
            dwarf_string(&w->info, enumerator->name);
            dwarf_sleb(&w->info, enumerator->value);
        }
        out_u8(&w->info, 0);
        return;
    }
    case TYPE_ARRAY: {
        TypeArray *array = (TypeArray *) type;
        if (array->array_kind == ARRAY_FIXED) {
            out_u8(&w->info, DW_ABBREV_ARRAY);
            dwarf_type_ref(w, array->elem_type);
            out_u8(&w->info, DW_ABBREV_SUBRANGE);
            out_u64(&w->info, (uint64_t)(array->capacity > 0 ? array->capacity : 0));
            out_u8(&w->info, 0);
        } else {
            dwarf_struct(w, type, array->struct_defn, name);   // []T and [..]T: data, count, ...
        }
        return;
    }
    case TYPE_STRING: dwarf_struct(w, type, ((TypeString *) type)->struct_defn, "string"); return;
    case TYPE_ANY:    dwarf_struct(w, type, ((TypeAny *) type)->struct_defn, "any"); return;
    case TYPE_STRUCT: dwarf_struct(w, type, (TypeStruct *) type, name); return;
    case TYPE_VARIADIC: {
        TypeVariadic *variadic = (TypeVariadic *) type;
        if (variadic->slice) { dwarf_type(w, (Type *) variadic->slice); return; }
        break;
    }
    default:
        break;
    }
    dwarf_bytes(w, type);
}

//
// Functions
//

// The pieces of the code of a function that are of the scope or of a scope inside it, as offsets from the start of
// the function. Returns their count
static int dwarf_scope_ranges(DwarfWriter *w, X64DebugFunction *f, int scope, int *starts, int *ends, int capacity) {
    IrFunction *func = f->func;
    int function_offset = w->obj->symbols[asm_symbol(w->obj, w->module->globals.items[func->global]->name)].offset;
    int end = dwarf_label_offset(w, func, f->end_label) - function_offset;
    int count = 0;
    for (int i = 0; i < f->marks.count; i++) {
        // Whether the mark is in the scope: its scope or one of the scopes it is in
        bool inside = false;
        for (int s = f->marks.items[i].scope, guard = 0; s > 0 && guard < 1000; s = func->scopes.items[s].parent, guard++) {
            if (s == scope) { inside = true; break; }
            if (s >= func->scopes.count) break;
        }
        if (!inside) continue;
        int from = dwarf_label_offset(w, func, f->marks.items[i].label) - function_offset;
        int to = i + 1 < f->marks.count ? dwarf_label_offset(w, func, f->marks.items[i + 1].label) - function_offset : end;
        if (to <= from) continue;
        if (count > 0 && ends[count - 1] == from) ends[count - 1] = to;
        else if (count < capacity) { starts[count] = from; ends[count] = to; count++; }
    }
    return count;
}

static void dwarf_variable(DwarfWriter *w, X64DebugFunction *f, IrLocal *local, int file) {
    IrFunction *func = f->func;
    out_u8(&w->info, local->arg > 0 ? DW_ABBREV_PARAMETER : DW_ABBREV_VARIABLE);
    dwarf_string(&w->info, local->name);
    out_u16(&w->info, (uint16_t) file);
    out_u32(&w->info, (uint32_t)(local->line > 0 ? local->line : func->source_line));
    dwarf_type_ref(w, (Type *) local->type);
    // DW_OP_fbreg offset: the slot is at rbp + offset, and rbp is the frame base
    OutBytes expression = {0};
    out_u8(&expression, 0x91);
    dwarf_sleb(&expression, local->slot >= 0 && local->slot < func->slots.count ? f->slot_offsets[local->slot] : 0);
    dwarf_uleb(&w->info, expression.size);
    out_put(&w->info, expression.data, expression.size);
    free(expression.data);
}

// Whether a variable is in the scope or in a scope in it
static bool dwarf_scope_has_variables(IrFunction *func, int scope) {
    for (int i = 0; i < func->locals.count; i++) {
        for (int s = func->locals.items[i].scope, guard = 0; s >= 0 && s < func->scopes.count && guard < 1000; guard++) {
            if (s == scope) return true;
            if (s == 0) break;
            s = func->scopes.items[s].parent;
        }
    }
    return false;
}

// The variables of the scope, then the lexical blocks of the scopes in it that have variables
static void dwarf_scope(DwarfWriter *w, X64DebugFunction *f, int scope, int file) {
    IrFunction *func = f->func;
    for (int i = 0; i < func->locals.count; i++) {
        IrLocal *local = &func->locals.items[i];
        if (local->scope == scope && local->arg > 0) dwarf_variable(w, f, local, file);
    }
    for (int i = 0; i < func->locals.count; i++) {
        IrLocal *local = &func->locals.items[i];
        if (local->scope == scope && local->arg == 0) dwarf_variable(w, f, local, file);
    }
    for (int child = scope + 1; child < func->scopes.count; child++) {
        if (func->scopes.items[child].parent != scope || !dwarf_scope_has_variables(func, child)) continue;
        int starts[256], ends[256];
        int count = dwarf_scope_ranges(w, f, child, starts, ends, 256);
        if (count == 0) continue;   // No code
        // The pieces, as offsets from the start of the unit (its low_pc). Not with a base address entry, which LLDB
        // doesn't read
        int function_offset = w->obj->symbols[asm_symbol(w->obj, w->module->globals.items[func->global]->name)].offset;
        out_u8(&w->info, DW_ABBREV_LEXICAL_BLOCK);
        dwarf_section_offset(w, &w->info, w->info_section, w->ranges_start, (int) w->ranges.size);
        for (int r = 0; r < count; r++) {
            out_u64(&w->ranges, (uint64_t)(function_offset + starts[r] - w->unit_low));
            out_u64(&w->ranges, (uint64_t)(function_offset + ends[r] - w->unit_low));
        }
        out_u64(&w->ranges, 0);
        out_u64(&w->ranges, 0);
        dwarf_scope(w, f, child, file);
        out_u8(&w->info, 0);
    }
}

static void dwarf_subprogram(DwarfWriter *w, X64DebugFunction *f) {
    IrFunction *func = f->func;
    int symbol = asm_symbol(w->obj, w->module->globals.items[func->global]->name);
    int file = dwarf_file(w, func->source_file);
    bool has_variables = func->locals.count > 0;
    out_u8(&w->info, has_variables ? DW_ABBREV_SUBPROGRAM : DW_ABBREV_SUBPROGRAM_EMPTY);
    dwarf_string(&w->info, func->source_name ? func->source_name : func->name);
    out_u16(&w->info, (uint16_t) file);
    out_u32(&w->info, (uint32_t) func->source_line);
    dwarf_address(w, &w->info, w->info_section, symbol);
    out_u32(&w->info, (uint32_t)(dwarf_label_offset(w, func, f->end_label) - w->obj->symbols[symbol].offset));
    out_u8(&w->info, 1);       // Frame base: DW_OP_reg6 (rbp)
    out_u8(&w->info, 0x56);
    if (!has_variables) return;
    dwarf_scope(w, f, 0, file);
    out_u8(&w->info, 0);
}

// The line program of a function: its line at its start, then the line of each mark, from the end of the prologue on
static void dwarf_line_sequence(DwarfWriter *w, X64DebugFunction *f) {
    IrFunction *func = f->func;
    OutBytes *o = &w->line;
    int symbol = asm_symbol(w->obj, w->module->globals.items[func->global]->name);
    int start = w->obj->symbols[symbol].offset;

    out_u8(o, 0); dwarf_uleb(o, 9); out_u8(o, 0x02);     // DW_LNE_set_address
    dwarf_address(w, o, w->line_section, symbol);
    out_u8(o, 0x04); dwarf_uleb(o, dwarf_file(w, func->source_file));   // DW_LNS_set_file
    int line = 1, address = start;
    int function_line = func->source_line > 0 ? func->source_line : 1;
    out_u8(o, 0x03); dwarf_sleb(o, function_line - line); line = function_line;   // DW_LNS_advance_line
    out_u8(o, 0x01);                                                              // DW_LNS_copy

    int prologue_end = dwarf_label_offset(w, func, f->prologue_label);
    bool marked_prologue = false;
    for (int i = 0; i < f->marks.count; i++) {
        int at = dwarf_label_offset(w, func, f->marks.items[i].label);
        // Of marks at the same place, the last one is the line there
        if (i + 1 < f->marks.count && dwarf_label_offset(w, func, f->marks.items[i + 1].label) == at) continue;
        if (at < prologue_end) at = prologue_end;
        if (at > address) { out_u8(o, 0x02); dwarf_uleb(o, (uint64_t)(at - address)); address = at; }   // DW_LNS_advance_pc
        int mark_line = f->marks.items[i].line;
        if (mark_line != line) { out_u8(o, 0x03); dwarf_sleb(o, mark_line - line); line = mark_line; }
        if (!marked_prologue) { out_u8(o, 0x0a); marked_prologue = true; }       // DW_LNS_set_prologue_end
        out_u8(o, 0x01);
    }
    int end = dwarf_label_offset(w, func, f->end_label);
    if (end > address) { out_u8(o, 0x02); dwarf_uleb(o, (uint64_t)(end - address)); }
    out_u8(o, 0); dwarf_uleb(o, 1); out_u8(o, 0x01);     // DW_LNE_end_sequence
}

// The header of the line table, with the files, for a program (the sequences) of program_size bytes after it
static void dwarf_line_header(DwarfWriter *w, OutBytes *o, size_t program_size) {
    OutBytes header = {0};
    out_u8(&header, 1);          // minimum_instruction_length
    out_u8(&header, 1);          // maximum_operations_per_instruction
    out_u8(&header, 1);          // default_is_stmt
    out_u8(&header, (uint8_t)(int8_t) -5);  // line_base
    out_u8(&header, 14);         // line_range
    out_u8(&header, 13);         // opcode_base
    static const uint8_t lengths[12] = { 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1 };
    out_put(&header, lengths, 12);
    out_u8(&header, 0);          // No include directories: the files are absolute paths
    for (int i = 0; i < w->file_count; i++) {
        dwarf_string(&header, w->files[i]);
        dwarf_uleb(&header, 0);
        dwarf_uleb(&header, 0);
        dwarf_uleb(&header, 0);
    }
    out_u8(&header, 0);

    out_u32(o, (uint32_t)(2 + 4 + header.size + program_size));   // unit_length
    out_u16(o, 4);                                                 // version
    out_u32(o, (uint32_t) header.size);                            // header_length
    out_put(o, header.data, header.size);
    free(header.data);
}

// The unwind tables of Windows. Every function starts with push rbp (1 byte) and mov rbp, rsp (3 bytes), and rbp
// stays the frame, so all of them have the same unwind information: rbp is the frame register, and below it is the
// rbp of the caller and the return address
static void dwarf_windows_unwind(DwarfWriter *w, X64DebugFunction *functions, int count) {
    int xdata_start, pdata_start;
    int xdata = dwarf_section(w, ".xdata", 0x40000040, &xdata_start);
    int pdata = dwarf_section(w, ".pdata", 0x40000040, &pdata_start);
    AsmBytes *x = &w->obj->sections[xdata];
    static const uint8_t unwind_info[8] = {
        0x01,           // Version 1, no flags
        4,              // Size of the prologue
        2,              // Two unwind codes
        0x05,           // Frame register rbp, at offset 0
        4, 0x03,        // At 4: UWOP_SET_FPREG
        1, 0x50,        // At 1: UWOP_PUSH_NONVOL rbp
    };
    asm_put(x, unwind_info, 8);
    AsmBytes *p = &w->obj->sections[pdata];
    for (int i = 0; i < count; i++) {
        IrFunction *func = functions[i].func;
        int symbols[3] = {
            asm_symbol(w->obj, w->module->globals.items[func->global]->name),
            dwarf_label_symbol(w, func, functions[i].end_label),
            xdata_start,
        };
        for (int k = 0; k < 3; k++) {
            AsmReloc reloc = { .section = pdata, .offset = p->size, .kind = ASM_RELOC_RVA32, .symbol = symbols[k] };
            asm_add_reloc(w->obj, reloc);
            uint32_t zero = 0;
            asm_put(p, &zero, 4);
        }
    }
}

// Writes the debug information of the functions into the object. for_windows: also the unwind tables
void dwarf_write(AsmObject *obj, IrModule *module, X64DebugFunction *functions, int count, bool for_windows) {
    DwarfWriter w = {0};
    w.obj = obj;
    w.module = module;
    uint32_t flags = 0x42000040;   // Readable, discardable, initialized data
    w.abbrev_section = dwarf_section(&w, ".debug_abbrev", flags, &w.abbrev_start);
    w.info_section   = dwarf_section(&w, ".debug_info", flags, &w.info_start);
    w.line_section   = dwarf_section(&w, ".debug_line", flags, &w.line_start);
    w.ranges_section = dwarf_section(&w, ".debug_ranges", flags, &w.ranges_start);
    dwarf_write_abbrevs(&w);

    // The functions of the program (not the ones the compiler makes) and where their code is
    const char *main_file = NULL;
    int first = -1, low = INT_MAX, high = 0;
    for (int i = 0; i < count; i++) {
        IrFunction *func = functions[i].func;
        if (!func->source_file) continue;
        int start = w.obj->symbols[asm_symbol(obj, module->globals.items[func->global]->name)].offset;
        int end = dwarf_label_offset(&w, func, functions[i].end_label);
        if (start < low) { low = start; first = i; }
        if (end > high) high = end;
        if (func->is_entry_point || !main_file) main_file = func->source_file;
    }

    w.unit_low = first >= 0 ? low : 0;

    // .debug_info: the unit
    OutBytes *o = &w.info;
    out_u32(o, 0);                    // unit_length, set at the end
    out_u16(o, 4);                    // version
    dwarf_section_offset(&w, o, w.info_section, w.abbrev_start, 0);
    out_u8(o, 8);                     // address_size
    out_u8(o, DW_ABBREV_COMPILE_UNIT);
    dwarf_string(o, "saad");
    out_u16(o, 0x0c);                 // DW_LANG_C99, so debuggers know how to show the values
    char directory[1024] = ".";
    const char *name = main_file ? main_file : "main.sd";
    const char *slash = strrchr(name, '/');
    const char *backslash = strrchr(name, '\\');
    if (!slash || (backslash && backslash > slash)) slash = backslash;
    if (slash) snprintf(directory, sizeof(directory), "%.*s", (int)(slash - name), name);
    dwarf_string(o, name);
    dwarf_string(o, directory);
    dwarf_section_offset(&w, o, w.info_section, w.line_start, 0);
    if (first >= 0) dwarf_address(&w, o, w.info_section, asm_symbol(obj, module->globals.items[functions[first].func->global]->name));
    else            out_u64(o, 0);
    out_u64(o, first >= 0 ? (uint64_t)(high - low) : 0);

    // The subprograms, and the line sequences into w.line, which the header goes before
    int line_relocs_from = obj->reloc_count;
    for (int i = 0; i < count; i++) {
        if (!functions[i].func->source_file) continue;
        dwarf_subprogram(&w, &functions[i]);
        dwarf_line_sequence(&w, &functions[i]);
    }

    // The types, also the ones that the types refer to
    for (int i = 0; i < w.type_count; i++) {
        w.type_offsets[i] = (int) o->size;
        dwarf_type(&w, w.types[i]);
    }
    out_u8(o, 0);
    for (int i = 0; i < w.fixup_count; i++) put_u32_at(&o->data[w.fixups[i].at], (uint32_t) w.type_offsets[w.fixups[i].type]);
    put_u32_at(o->data, (uint32_t)(o->size - 4));

    // .debug_line: the header, then the sequences, whose relocations move with them
    OutBytes program = w.line;
    OutBytes line = {0};
    dwarf_line_header(&w, &line, program.size);
    for (int i = line_relocs_from; i < obj->reloc_count; i++) {
        if (obj->relocs[i].section == w.line_section) obj->relocs[i].offset += (int) line.size;
    }
    out_put(&line, program.data, program.size);
    w.line = line;

    // Into the sections
    asm_put(&obj->sections[w.abbrev_section], w.abbrev.data, (int) w.abbrev.size);
    asm_put(&obj->sections[w.info_section], w.info.data, (int) w.info.size);
    asm_put(&obj->sections[w.line_section], w.line.data, (int) w.line.size);
    asm_put(&obj->sections[w.ranges_section], w.ranges.data, (int) w.ranges.size);

    if (for_windows) dwarf_windows_unwind(&w, functions, count);

    free(w.abbrev.data); free(w.info.data); free(w.line.data); free(w.ranges.data); free(program.data);
    for (int i = 0; i < w.file_count; i++) free(w.files[i]);
    free(w.files); free(w.types); free(w.type_offsets); free(w.fixups); free(w.type_index.entries);
}
