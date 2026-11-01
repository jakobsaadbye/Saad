//////////////////////////////////////////////////////////////////////////////////////////
//
// Writes what the assembler (x64_asm.c) made into a file:
//
//   * coff_write()  a COFF object file (.obj) for Windows, like 'nasm -fwin64' writes. For comparing with nasm
//   * elf_write()   an ELF object file (.o), for running the programs on Linux (tools/linux)
//   * pe_write()    a Windows program (.exe) that imports the C runtime and the other DLLs it uses. No linker is needed
//
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct OutBytes {
    uint8_t *data;
    size_t   size;
    size_t   capacity;
} OutBytes;

static void out_reserve(OutBytes *o, size_t size) {
    if (o->size + size <= o->capacity) return;
    size_t capacity = o->capacity ? o->capacity * 2 : 65536;
    while (capacity < o->size + size) capacity *= 2;
    o->data = realloc(o->data, capacity);
    o->capacity = capacity;
}

static void out_put(OutBytes *o, const void *data, size_t size) {
    out_reserve(o, size);
    memcpy(&o->data[o->size], data, size);
    o->size += size;
}

static void out_u8(OutBytes *o, uint8_t v)   { out_put(o, &v, 1); }
static void out_u16(OutBytes *o, uint16_t v) { out_put(o, &v, 2); }
static void out_u32(OutBytes *o, uint32_t v) { out_put(o, &v, 4); }
static void out_u64(OutBytes *o, uint64_t v) { out_put(o, &v, 8); }
static void out_zeros(OutBytes *o, size_t n) { out_reserve(o, n); memset(&o->data[o->size], 0, n); o->size += n; }
static void out_pad(OutBytes *o, size_t align) { while (o->size % align) out_u8(o, 0); }

static void put_u32_at(uint8_t *at, uint32_t v) { memcpy(at, &v, 4); }
static void put_u64_at(uint8_t *at, uint64_t v) { memcpy(at, &v, 8); }
static void put_u16_at(uint8_t *at, uint16_t v) { memcpy(at, &v, 2); }

static bool write_file_bytes(const char *path, OutBytes *o) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(o->data, 1, o->size, f) == o->size;
    fclose(f);
    return ok;
}

// A rel32 to a symbol of the same section is known without a linker, as the distance doesn't change
static bool reloc_is_resolved(AsmObject *obj, AsmReloc *r) {
    AsmSymbol *s = &obj->symbols[r->symbol];
    return r->kind == ASM_RELOC_REL32 && s->section == r->section;
}

static void resolve_local_relocs(AsmObject *obj) {
    for (int i = 0; i < obj->reloc_count; i++) {
        AsmReloc *r = &obj->relocs[i];
        if (!reloc_is_resolved(obj, r)) continue;
        AsmSymbol *s = &obj->symbols[r->symbol];
        int64_t value = s->offset + r->addend - (r->offset + 4 + r->trailing);
        put_u32_at(&obj->sections[r->section].data[r->offset], (uint32_t)(int32_t)value);
    }
}

static int log2_of(int n) {
    int l = 0;
    while ((1 << l) < n) l++;
    return l;
}

//
// COFF
//

bool coff_write(AsmObject *obj, const char *path) {
    resolve_local_relocs(obj);

    // Symbols: the sections, then the globals and the externs. Other labels are reached through their section
    OutBytes symbols = {0}, strings = {0};
    out_u32(&strings, 0); // Its size, set at the end
    int *symbol_index = calloc(obj->symbol_count, sizeof(int));
    int count = 0;

    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        char name[8] = {0};
        memcpy(name, asm_section_names[s], strlen(asm_section_names[s]));
        out_put(&symbols, name, 8);
        out_u32(&symbols, 0);
        out_u16(&symbols, (uint16_t)(s + 1));
        out_u16(&symbols, 0);
        out_u8(&symbols, 3); // static
        out_u8(&symbols, 0);
        count++;
    }
    for (int i = 0; i < obj->symbol_count; i++) {
        AsmSymbol *s = &obj->symbols[i];
        symbol_index[i] = -1;
        if (!s->is_global && !s->is_extern) continue;
        size_t length = strlen(s->name);
        if (length <= 8) {
            char name[8] = {0};
            memcpy(name, s->name, length);
            out_put(&symbols, name, 8);
        } else {
            out_u32(&symbols, 0);
            out_u32(&symbols, (uint32_t)strings.size);
            out_put(&strings, s->name, length + 1);
        }
        out_u32(&symbols, s->section >= 0 ? (uint32_t)s->offset : 0);
        out_u16(&symbols, (uint16_t)(s->section >= 0 ? s->section + 1 : 0));
        out_u16(&symbols, 0x20); // function
        out_u8(&symbols, 2);     // external
        out_u8(&symbols, 0);
        symbol_index[i] = count++;
    }
    put_u32_at(strings.data, (uint32_t)strings.size);

    // The relocations of each section
    OutBytes relocs[ASM_SECTION_COUNT] = {0};
    int reloc_count[ASM_SECTION_COUNT] = {0};
    for (int i = 0; i < obj->reloc_count; i++) {
        AsmReloc *r = &obj->relocs[i];
        if (reloc_is_resolved(obj, r)) continue;
        AsmSymbol *s = &obj->symbols[r->symbol];
        int64_t addend = r->addend;
        int target = symbol_index[r->symbol];
        if (target < 0) {
            target = s->section;      // The symbol of its section
            addend += s->offset;
        }
        uint8_t *field = &obj->sections[r->section].data[r->offset];
        uint16_t type;
        if (r->kind == ASM_RELOC_ABS64) { type = 1; put_u64_at(field, (uint64_t)addend); }
        else                            { type = (uint16_t)(4 + r->trailing); put_u32_at(field, (uint32_t)(int32_t)addend); }
        out_u32(&relocs[r->section], (uint32_t)r->offset);
        out_u32(&relocs[r->section], (uint32_t)target);
        out_u16(&relocs[r->section], type);
        reloc_count[r->section]++;
    }

    OutBytes o = {0};
    uint32_t header_size = 20 + 40 * ASM_SECTION_COUNT;
    uint32_t at = header_size;
    uint32_t data_at[ASM_SECTION_COUNT], reloc_at[ASM_SECTION_COUNT];
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        data_at[s] = at;  at += obj->sections[s].size;
        reloc_at[s] = at; at += (uint32_t)relocs[s].size;
    }
    uint32_t symbols_at = at;

    out_u16(&o, 0x8664);
    out_u16(&o, ASM_SECTION_COUNT);
    out_u32(&o, 0);
    out_u32(&o, symbols_at);
    out_u32(&o, (uint32_t)count);
    out_u16(&o, 0);
    out_u16(&o, 0);

    static const uint32_t characteristics[ASM_SECTION_COUNT] = { 0x60000020, 0xC0000040, 0x40000040 };
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        char name[8] = {0};
        memcpy(name, asm_section_names[s], strlen(asm_section_names[s]));
        out_put(&o, name, 8);
        out_u32(&o, 0);
        out_u32(&o, 0);
        out_u32(&o, (uint32_t)obj->sections[s].size);
        out_u32(&o, obj->sections[s].size ? data_at[s] : 0);
        out_u32(&o, reloc_count[s] ? reloc_at[s] : 0);
        out_u32(&o, 0);
        out_u16(&o, (uint16_t)reloc_count[s]);
        out_u16(&o, 0);
        out_u32(&o, characteristics[s] | ((uint32_t)(log2_of(obj->alignment[s]) + 1) << 20));
    }
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        if (obj->sections[s].size) out_put(&o, obj->sections[s].data, obj->sections[s].size);
        out_put(&o, relocs[s].data, relocs[s].size);
    }
    out_put(&o, symbols.data, symbols.size);
    out_put(&o, strings.data, strings.size);

    bool ok = write_file_bytes(path, &o);
    free(o.data); free(symbols.data); free(strings.data); free(symbol_index);
    for (int s = 0; s < ASM_SECTION_COUNT; s++) free(relocs[s].data);
    return ok;
}

//
// ELF, for running the programs on Linux. The names of the externs and globals get a prefix, like
// 'nasm --prefix ms_' does, so tools/linux/shim.c can give them the Win64 calling convention
//

bool elf_write(AsmObject *obj, const char *path, const char *prefix) {
    resolve_local_relocs(obj);
    if (!prefix) prefix = "";

    // Sections: 0 null, 1 .text, 2 .data, 3 .rodata, 4-6 .rela.*, 7 .symtab, 8 .strtab, 9 .shstrtab
    static const char *names[] = { "", ".text", ".data", ".rodata", ".rela.text", ".rela.data", ".rela.rodata", ".symtab", ".strtab", ".shstrtab" };
    OutBytes shstrtab = {0};
    uint32_t name_at[10];
    for (int i = 0; i < 10; i++) { name_at[i] = (uint32_t)shstrtab.size; out_put(&shstrtab, names[i], strlen(names[i]) + 1); }

    // Symbols: null, the 3 sections (local), then the globals and externs
    OutBytes symtab = {0}, strtab = {0};
    out_u8(&strtab, 0);
    out_zeros(&symtab, 24);
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        out_u32(&symtab, 0);
        out_u8(&symtab, 3);              // STB_LOCAL, STT_SECTION
        out_u8(&symtab, 0);
        out_u16(&symtab, (uint16_t)(s + 1));
        out_u64(&symtab, 0);
        out_u64(&symtab, 0);
    }
    int first_global = 4;
    int *symbol_index = calloc(obj->symbol_count, sizeof(int));
    int count = 4;
    for (int i = 0; i < obj->symbol_count; i++) {
        AsmSymbol *s = &obj->symbols[i];
        symbol_index[i] = -1;
        if (!s->is_global && !s->is_extern) continue;
        out_u32(&symtab, (uint32_t)strtab.size);
        out_put(&strtab, prefix, strlen(prefix));
        out_put(&strtab, s->name, strlen(s->name) + 1);
        out_u8(&symtab, s->section >= 0 && s->section == ASM_TEXT ? 0x12 : 0x10); // STB_GLOBAL, STT_FUNC or STT_NOTYPE
        out_u8(&symtab, 0);
        out_u16(&symtab, (uint16_t)(s->section >= 0 ? s->section + 1 : 0));
        out_u64(&symtab, s->section >= 0 ? (uint64_t)s->offset : 0);
        out_u64(&symtab, 0);
        symbol_index[i] = count++;
    }

    OutBytes rela[ASM_SECTION_COUNT] = {0};
    for (int i = 0; i < obj->reloc_count; i++) {
        AsmReloc *r = &obj->relocs[i];
        if (reloc_is_resolved(obj, r)) continue;
        AsmSymbol *s = &obj->symbols[r->symbol];
        int64_t addend = r->addend;
        int target = symbol_index[r->symbol];
        if (target < 0) { target = 1 + s->section; addend += s->offset; }
        uint32_t type;
        if (r->kind == ASM_RELOC_ABS64) type = 1;                        // R_X86_64_64
        else { type = 2; addend -= 4 + r->trailing; }                    // R_X86_64_PC32
        out_u64(&rela[r->section], (uint64_t)r->offset);
        out_u64(&rela[r->section], ((uint64_t)target << 32) | type);
        out_u64(&rela[r->section], (uint64_t)addend);
    }

    OutBytes o = {0};
    out_zeros(&o, 64);
    uint64_t offsets[10] = {0}, sizes[10] = {0};
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        out_pad(&o, 16);
        offsets[1 + s] = o.size;
        sizes[1 + s] = obj->sections[s].size;
        out_put(&o, obj->sections[s].data, obj->sections[s].size);
    }
    for (int s = 0; s < ASM_SECTION_COUNT; s++) {
        out_pad(&o, 8);
        offsets[4 + s] = o.size;
        sizes[4 + s] = rela[s].size;
        out_put(&o, rela[s].data, rela[s].size);
    }
    out_pad(&o, 8);
    offsets[7] = o.size; sizes[7] = symtab.size; out_put(&o, symtab.data, symtab.size);
    offsets[8] = o.size; sizes[8] = strtab.size; out_put(&o, strtab.data, strtab.size);
    offsets[9] = o.size; sizes[9] = shstrtab.size; out_put(&o, shstrtab.data, shstrtab.size);
    out_pad(&o, 8);
    uint64_t section_headers = o.size;

    for (int i = 0; i < 10; i++) {
        uint32_t type = 0, link = 0, info = 0;
        uint64_t flags = 0, align = 1, entsize = 0;
        if (i >= 1 && i <= 3) { type = 1; align = 16; flags = i == 1 ? 6 : i == 2 ? 3 : 2; } // PROGBITS: AX, WA, A
        if (i >= 4 && i <= 6) { type = 4; link = 7; info = (uint32_t)(i - 3); align = 8; entsize = 24; flags = 0x40; } // RELA, INFO_LINK
        if (i == 7) { type = 2; link = 8; info = (uint32_t)first_global; align = 8; entsize = 24; }
        if (i == 8 || i == 9) type = 3;
        out_u32(&o, name_at[i]);
        out_u32(&o, type);
        out_u64(&o, flags);
        out_u64(&o, 0);
        out_u64(&o, i ? offsets[i] : 0);
        out_u64(&o, i ? sizes[i] : 0);
        out_u32(&o, link);
        out_u32(&o, info);
        out_u64(&o, i ? align : 0);
        out_u64(&o, entsize);
    }

    // The header
    uint8_t *h = o.data;
    memcpy(h, "\x7f" "ELF", 4);
    h[4] = 2; h[5] = 1; h[6] = 1;               // 64-bit, little endian, version 1
    put_u16_at(h + 16, 1);                      // ET_REL
    put_u16_at(h + 18, 62);                     // EM_X86_64
    put_u32_at(h + 20, 1);
    put_u64_at(h + 40, section_headers);
    put_u16_at(h + 52, 64);                     // e_ehsize
    put_u16_at(h + 58, 64);                     // e_shentsize
    put_u16_at(h + 60, 10);                     // e_shnum
    put_u16_at(h + 62, 9);                      // e_shstrndx

    bool ok = write_file_bytes(path, &o);
    free(o.data); free(symtab.data); free(strtab.data); free(shstrtab.data); free(symbol_index);
    for (int s = 0; s < ASM_SECTION_COUNT; s++) free(rela[s].data);
    return ok;
}

//
// PE: a Windows program (.exe). The code calls a function of a DLL through a small thunk, 'jmp [rip + entry]', where
// entry is its slot of the import address table, which Windows fills in when it loads the program. The entry point
// calls main and then exit() of the C runtime with what main returns, so the C runtime flushes its buffers
//

#define PE_IMAGE_BASE        0x140000000ULL
#define PE_SECTION_ALIGNMENT 0x1000
#define PE_FILE_ALIGNMENT    0x200

static uint32_t pe_align(uint32_t value, uint32_t align) { return (value + align - 1) / align * align; }

typedef struct PeImport {
    int         symbol;
    const char *library;
    uint32_t    thunk;      // Offset of its thunk in the text
    uint32_t    iat_entry;  // RVA of its slot in the import address table
    uint32_t    hint_name;  // RVA of its name
} PeImport;

// Writes the program. 'error' says what went wrong when it returns false
bool pe_write(AsmObject *obj, const char *path, char *error, int error_size) {
    int main_symbol = -1;
    for (int i = 0; i < obj->symbol_count; i++) {
        if (strcmp(obj->symbols[i].name, "main") == 0 && obj->symbols[i].section == ASM_TEXT) main_symbol = i;
    }
    if (main_symbol < 0) {
        snprintf(error, error_size, "The program has no main function");
        return false;
    }

    // The imports, grouped by library. The entry point needs exit() of the C runtime
    int exit_symbol = asm_symbol(obj, "exit");
    if (obj->symbols[exit_symbol].section < 0) obj->symbols[exit_symbol].is_extern = true;

    PeImport *imports = calloc(obj->symbol_count, sizeof(PeImport));
    int import_count = 0;
    const char *libraries[64];
    int library_count = 0;
    for (int i = 0; i < obj->symbol_count; i++) {
        AsmSymbol *s = &obj->symbols[i];
        if (s->section >= 0 || !s->is_extern) continue;
        const char *library = s->library ? s->library : "msvcrt.dll";
        bool known = false;
        for (int l = 0; l < library_count; l++) if (strcmp(libraries[l], library) == 0) known = true;
        if (!known) {
            if (library_count == 64) { snprintf(error, error_size, "The program uses more than 64 libraries"); free(imports); return false; }
            libraries[library_count++] = library;
        }
        imports[import_count++] = (PeImport){ .symbol = i, .library = library };
    }

    // .text: the code, the entry point, and a thunk for each import
    AsmBytes *code = &obj->sections[ASM_TEXT];
    OutBytes text = {0};
    out_put(&text, code->data, code->size);
    out_pad(&text, 16);
    uint32_t entry = (uint32_t)text.size;
    static const uint8_t entry_code[] = {
        0x48, 0x83, 0xEC, 0x28,         // sub rsp, 40
        0xE8, 0, 0, 0, 0,               // call main
        0x89, 0xC1,                     // mov ecx, eax
        0xE8, 0, 0, 0, 0,               // call exit
        0xCC,                           // int3
    };
    out_put(&text, entry_code, sizeof(entry_code));
    out_pad(&text, 8);
    for (int i = 0; i < import_count; i++) {
        imports[i].thunk = (uint32_t)text.size;
        static const uint8_t thunk[] = { 0xFF, 0x25, 0, 0, 0, 0, 0xCC, 0xCC }; // jmp [rip + entry]
        out_put(&text, thunk, sizeof(thunk));
    }

    // .rdata: the read only data, then the imports: their directory, the lookup and address tables, and the names
    OutBytes rdata = {0};
    out_put(&rdata, obj->sections[ASM_RDATA].data, obj->sections[ASM_RDATA].size);
    out_pad(&rdata, 16);

    uint32_t text_rva  = PE_SECTION_ALIGNMENT;
    uint32_t rdata_rva = text_rva + pe_align((uint32_t)text.size, PE_SECTION_ALIGNMENT);

    uint32_t directory = (uint32_t)rdata.size;
    out_zeros(&rdata, 20 * (library_count + 1));
    uint32_t *lookup_at = calloc(library_count, sizeof(uint32_t));
    uint32_t *address_at = calloc(library_count, sizeof(uint32_t));
    uint32_t iat_start = 0, iat_end = 0;
    for (int pass = 0; pass < 2; pass++) {
        // The lookup tables, then the address tables, which are the same until Windows fills in the second
        for (int l = 0; l < library_count; l++) {
            out_pad(&rdata, 8);
            if (pass == 0) lookup_at[l] = (uint32_t)rdata.size;
            else           address_at[l] = (uint32_t)rdata.size;
            if (pass == 1 && l == 0) iat_start = (uint32_t)rdata.size;
            for (int i = 0; i < import_count; i++) {
                if (strcmp(imports[i].library, libraries[l]) != 0) continue;
                if (pass == 1) imports[i].iat_entry = rdata_rva + (uint32_t)rdata.size;
                out_u64(&rdata, 0); // The name's RVA, set below
            }
            out_u64(&rdata, 0);
        }
    }
    iat_end = (uint32_t)rdata.size;
    for (int i = 0; i < import_count; i++) {
        out_pad(&rdata, 2);
        imports[i].hint_name = rdata_rva + (uint32_t)rdata.size;
        out_u16(&rdata, 0);
        const char *name = obj->symbols[imports[i].symbol].name;
        out_put(&rdata, name, strlen(name) + 1);
    }
    uint32_t *library_name = calloc(library_count, sizeof(uint32_t));
    for (int l = 0; l < library_count; l++) {
        library_name[l] = rdata_rva + (uint32_t)rdata.size;
        out_put(&rdata, libraries[l], strlen(libraries[l]) + 1);
    }
    // Fill in the tables and the directory
    for (int l = 0; l < library_count; l++) {
        uint32_t lookup = lookup_at[l], address = address_at[l];
        for (int i = 0; i < import_count; i++) {
            if (strcmp(imports[i].library, libraries[l]) != 0) continue;
            put_u64_at(&rdata.data[lookup], imports[i].hint_name);
            put_u64_at(&rdata.data[address], imports[i].hint_name);
            lookup += 8;
            address += 8;
        }
        uint8_t *d = &rdata.data[directory + 20 * l];
        put_u32_at(d + 0, rdata_rva + lookup_at[l]);
        put_u32_at(d + 12, library_name[l]);
        put_u32_at(d + 16, rdata_rva + address_at[l]);
    }

    uint32_t data_rva = rdata_rva + pe_align((uint32_t)rdata.size, PE_SECTION_ALIGNMENT);
    AsmBytes *data = &obj->sections[ASM_DATA];
    uint32_t data_size = data->size ? (uint32_t)data->size : 8;
    uint32_t image_size = data_rva + pe_align(data_size, PE_SECTION_ALIGNMENT);

    // Where each symbol is
    uint32_t section_rva[ASM_SECTION_COUNT] = { text_rva, data_rva, rdata_rva };
    uint32_t *symbol_rva = calloc(obj->symbol_count, sizeof(uint32_t));
    for (int i = 0; i < obj->symbol_count; i++) {
        AsmSymbol *s = &obj->symbols[i];
        if (s->section >= 0) symbol_rva[i] = section_rva[s->section] + (uint32_t)s->offset;
    }
    for (int i = 0; i < import_count; i++) symbol_rva[imports[i].symbol] = text_rva + imports[i].thunk;

    // The relocations
    uint8_t *section_data[ASM_SECTION_COUNT] = { text.data, data->data, rdata.data };
    for (int i = 0; i < obj->reloc_count; i++) {
        AsmReloc *r = &obj->relocs[i];
        uint8_t *field = &section_data[r->section][r->offset];
        uint64_t target = symbol_rva[r->symbol] + r->addend;
        if (r->kind == ASM_RELOC_ABS64) {
            put_u64_at(field, PE_IMAGE_BASE + target);
        } else {
            int64_t place = section_rva[r->section] + r->offset + 4 + r->trailing;
            put_u32_at(field, (uint32_t)(int32_t)((int64_t)target - place));
        }
    }
    put_u32_at(&text.data[entry + 5], symbol_rva[main_symbol] - (text_rva + entry + 9));
    put_u32_at(&text.data[entry + 12], symbol_rva[exit_symbol] - (text_rva + entry + 16));
    for (int i = 0; i < import_count; i++) {
        put_u32_at(&text.data[imports[i].thunk + 2], imports[i].iat_entry - (text_rva + imports[i].thunk + 6));
    }

    // The file: headers, then the sections
    int section_count = 3;
    uint32_t headers_size = pe_align(64 + 4 + 20 + 240 + 40 * section_count, PE_FILE_ALIGNMENT);
    uint32_t text_raw  = pe_align((uint32_t)text.size, PE_FILE_ALIGNMENT);
    uint32_t rdata_raw = pe_align((uint32_t)rdata.size, PE_FILE_ALIGNMENT);
    uint32_t data_raw  = pe_align(data_size, PE_FILE_ALIGNMENT);

    OutBytes o = {0};
    // DOS header: only 'MZ' and where the PE header is
    out_u16(&o, 0x5A4D);
    out_zeros(&o, 58);
    out_u32(&o, 64);
    // PE header
    out_put(&o, "PE\0\0", 4);
    out_u16(&o, 0x8664);                 // x64
    out_u16(&o, (uint16_t)section_count);
    out_u32(&o, 0);                      // Time stamp
    out_u32(&o, 0);
    out_u32(&o, 0);
    out_u16(&o, 240);                    // Size of the optional header
    out_u16(&o, 0x0022);                 // Executable, large address aware
    // Optional header (PE32+)
    out_u16(&o, 0x20B);
    out_u8(&o, 14); out_u8(&o, 0);       // Linker version
    out_u32(&o, text_raw);               // Size of code
    out_u32(&o, rdata_raw + data_raw);   // Size of initialized data
    out_u32(&o, 0);
    out_u32(&o, text_rva + entry);       // Entry point
    out_u32(&o, text_rva);               // Base of code
    out_u64(&o, PE_IMAGE_BASE);
    out_u32(&o, PE_SECTION_ALIGNMENT);
    out_u32(&o, PE_FILE_ALIGNMENT);
    out_u16(&o, 6); out_u16(&o, 0);      // OS version
    out_u16(&o, 0); out_u16(&o, 0);      // Image version
    out_u16(&o, 6); out_u16(&o, 0);      // Subsystem version
    out_u32(&o, 0);
    out_u32(&o, image_size);
    out_u32(&o, headers_size);
    out_u32(&o, 0);                      // Checksum
    out_u16(&o, 3);                      // Console program
    out_u16(&o, 0x8100);                 // NX compatible, terminal server aware. Not relocatable, so it has no .reloc
    out_u64(&o, 0x800000);               // Stack reserve (8 MB) and commit
    out_u64(&o, 0x1000);
    out_u64(&o, 0x100000);               // Heap reserve and commit
    out_u64(&o, 0x1000);
    out_u32(&o, 0);
    out_u32(&o, 16);                     // Data directories
    for (int d = 0; d < 16; d++) {
        if (d == 1)       { out_u32(&o, rdata_rva + directory); out_u32(&o, 20 * (library_count + 1)); }    // Imports
        else if (d == 12) { out_u32(&o, rdata_rva + iat_start); out_u32(&o, iat_end - iat_start); }          // Import address table
        else              { out_u32(&o, 0); out_u32(&o, 0); }
    }
    // Section headers
    struct { const char *name; uint32_t size, rva, raw, characteristics; } sections[3] = {
        { ".text",  (uint32_t)text.size,  text_rva,  text_raw,  0x60000020 },
        { ".rdata", (uint32_t)rdata.size, rdata_rva, rdata_raw, 0x40000040 },
        { ".data",  data_size,            data_rva,  data_raw,  0xC0000040 },
    };
    uint32_t file_at = headers_size;
    for (int s = 0; s < section_count; s++) {
        char name[8] = {0};
        memcpy(name, sections[s].name, strlen(sections[s].name));
        out_put(&o, name, 8);
        out_u32(&o, sections[s].size);
        out_u32(&o, sections[s].rva);
        out_u32(&o, sections[s].raw);
        out_u32(&o, file_at);
        out_u32(&o, 0); out_u32(&o, 0);
        out_u16(&o, 0); out_u16(&o, 0);
        out_u32(&o, sections[s].characteristics);
        file_at += sections[s].raw;
    }
    out_pad(&o, PE_FILE_ALIGNMENT);
    out_put(&o, text.data, text.size);    out_pad(&o, PE_FILE_ALIGNMENT);
    out_put(&o, rdata.data, rdata.size);  out_pad(&o, PE_FILE_ALIGNMENT);
    if (data->size) out_put(&o, data->data, data->size); else out_zeros(&o, 8);
    out_pad(&o, PE_FILE_ALIGNMENT);

    bool ok = write_file_bytes(path, &o);
    if (!ok) snprintf(error, error_size, "Couldn't write %s", path);
    free(o.data); free(text.data); free(rdata.data); free(imports); free(symbol_rva);
    free(lookup_at); free(address_at); free(library_name);
    return ok;
}
