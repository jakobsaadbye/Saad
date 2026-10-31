//
// The index of a program, for editors. See index.h and docs/lsp.md
//
// A symbol is known by where it is declared (its file and the offset of its name), so the copies the compiler makes of
// a declaration, e.g in each instance of a generic or for an import, are the same symbol. A use is known by its file and
// range, so a use that is checked more than once, e.g in each instance of a generic function, is only there once
//

#include "index.h"

Index program_index;

// Open addressing: the index + 1 of an entry, 0 for an empty slot
typedef struct IndexTable {
    int     *slots;
    int      capacity;
    int      count;
} IndexTable;

static IndexTable symbol_table;
static IndexTable reference_table;

static uint64_t index_key(AstFile *file, int a, int b) {
    uint64_t h = (uint64_t)(uintptr_t)file * 1099511628211ull;
    h ^= (uint64_t)(uint32_t)a * 2654435761ull;
    h ^= (uint64_t)(uint32_t)b * 40503ull;
    return h ^ (h >> 29);
}

static void table_grow(IndexTable *table, DynamicArray *entries, uint64_t (*key_of)(void *)) {
    int capacity = table->capacity == 0 ? 1024 : table->capacity * 2;
    free(table->slots);
    table->slots    = calloc(capacity, sizeof(int));
    table->capacity = capacity;
    for (int i = 0; i < entries->count; i++) {
        void *entry = (char *)entries->items + i * entries->item_size;
        uint64_t slot = key_of(entry) & (capacity - 1);
        while (table->slots[slot]) slot = (slot + 1) & (capacity - 1);
        table->slots[slot] = i + 1;
    }
}

static uint64_t symbol_key(void *entry) {
    IndexSymbol *s = entry;
    return index_key(s->file, s->name_start.input_idx, 0);
}

static uint64_t reference_key(void *entry) {
    IndexReference *r = entry;
    return index_key(r->file, r->start.input_idx, r->end.input_idx);
}

static int find_symbol(AstFile *file, int offset) {
    if (symbol_table.capacity == 0) return -1;
    uint64_t slot = index_key(file, offset, 0) & (symbol_table.capacity - 1);
    while (symbol_table.slots[slot]) {
        IndexSymbol *s = &((IndexSymbol *)program_index.symbols.items)[symbol_table.slots[slot] - 1];
        if (s->file == file && s->name_start.input_idx == offset) return symbol_table.slots[slot] - 1;
        slot = (slot + 1) & (symbol_table.capacity - 1);
    }
    return -1;
}

static int add_symbol(IndexSymbol symbol) {
    if (program_index.symbols.items == NULL) {
        program_index.symbols    = da_init(1024, sizeof(IndexSymbol));
        program_index.references = da_init(4096, sizeof(IndexReference));
    }
    da_append(&program_index.symbols, symbol);
    if ((program_index.symbols.count + 1) * 10 > symbol_table.capacity * 7) {
        table_grow(&symbol_table, &program_index.symbols, symbol_key);
    } else {
        uint64_t slot = symbol_key(&symbol) & (symbol_table.capacity - 1);
        while (symbol_table.slots[slot]) slot = (slot + 1) & (symbol_table.capacity - 1);
        symbol_table.slots[slot] = program_index.symbols.count;
    }
    return program_index.symbols.count - 1;
}

static bool has_position(Ast *ast) {
    return ast && ast->file && ast->start.line > 0 && !(ast->flags & AST_FLAG_COMPILER_GENERATED);
}

// True if the text from start to end is the name, or has it in it (e.g math.sqrt for sqrt). Declarations and uses that the
// compiler makes itself, e.g the 'self' of a method in a struct at the position of 'method', don't spell their name
static bool spells(AstFile *file, Pos start, Pos end, const char *name, bool exactly) {
    if (file->text == NULL || name == NULL || end.input_idx < start.input_idx) return false;
    int length = end.input_idx - start.input_idx;
    int name_length = (int)strlen(name);
    const char *text = file->text + start.input_idx;
    if (exactly) return length == name_length && strncmp(text, name, name_length) == 0;
    for (int i = 0; i + name_length <= length; i++) {
        if (strncmp(text + i, name, name_length) == 0) return true;
    }
    return false;
}

// The identifier that declares the name: the one an import or a namespace copied, and the generic struct of an instance
static AstIdentifier *declaration_of(AstIdentifier *ident) {
    while (ident->imported_from) ident = ident->imported_from;
    if ((ident->flags & IDENTIFIER_IS_NAME_OF_STRUCT) && ident->type && ident->type->kind == TYPE_STRUCT) {
        AstStruct *node = ((TypeStruct *)ident->type)->node;
        if (node->generic_struct) return node->generic_struct->identifier;
    }
    return ident;
}

static AstFunctionDefn *function_of(AstIdentifier *ident) {
    if (ident->type && ident->type->kind == TYPE_FUNCTION) return ((TypeFunction *)ident->type)->node;
    if (ident->decl && ident->decl->values.count > 0) {
        AstExpr *value = ((AstExpr **)ident->decl->values.items)[0];
        if (value->head.kind == AST_FUNCTION_DEFN) return (AstFunctionDefn *)value;
    }
    return NULL;
}

static AstStruct *struct_of_member(AstIdentifier *member) {
    if (member->scope && member->scope->belongs_to_struct) return member->scope->belongs_to_struct;
    return NULL;
}

static const char *kind_of(AstIdentifier *ident) {
    IdentifierFlags flags = ident->flags;
    Type *type = ident->type;

    if (flags & IDENTIFIER_IS_NAME_OF_STRUCT) {
        return (type && type->kind == TYPE_STRUCT && ((TypeStruct *)type)->node->is_union) ? "union" : "struct";
    }
    if (flags & IDENTIFIER_IS_NAME_OF_ENUM) return "enum";

    AstFunctionDefn *func = (flags & IDENTIFIER_IS_CONSTANT) || (flags & IDENTIFIER_IS_STRUCT_METHOD) ? function_of(ident) : NULL;
    if (flags & IDENTIFIER_IS_STRUCT_METHOD) return "method";
    if (func) return func->is_method ? "method" : "function";

    if (flags & IDENTIFIER_IS_TYPE_ALIAS) return ident->decl ? "typeAlias" : "typeParameter";
    if (flags & IDENTIFIER_IS_STRUCT_MEMBER) {
        AstStruct *owner = struct_of_member(ident);
        return owner && owner->is_union ? "variant" : "field";
    }
    if (flags & IDENTIFIER_IS_GLOBAL)   return "global";
    if (flags & IDENTIFIER_IS_CONSTANT) return "constant";
    if (ident->decl && (ident->decl->flags & DECLARATION_IS_FUNCTION_PARAMETER)) return "parameter";
    return "variable";
}

// e.g "<T>(xs: []T, x: T) -> int"
static char *signature_of(AstFunctionDefn *func) {
    StringBuilder sb = sb_init(64);
    if (func->is_method) sb_append(&sb, "method ");
    if (func->type_params.count > 0) {
        sb_append(&sb, "<");
        for (int i = 0; i < func->type_params.count; i++) {
            if (i > 0) sb_append(&sb, ", ");
            sb_append(&sb, "%s", ((AstIdentifier **)func->type_params.items)[i]->name);
        }
        sb_append(&sb, ">");
    }
    sb_append(&sb, "(");
    for (int i = 0; i < func->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func->parameters.items)[i];
        if (i > 0) sb_append(&sb, ", ");
        sb_append(&sb, "%s: %s", param->name, param->type ? type_to_str(param->type) : "?");
    }
    sb_append(&sb, ")");

    bool returns = false;
    for (int i = 0; i < func->return_types.count; i++) {
        Type *type = ((Type **)func->return_types.items)[i];
        if (type->kind == TYPE_VOID) continue;
        sb_append(&sb, returns ? ", %s" : " -> %s", type_to_str(type));
        returns = true;
    }
    return sb_to_string(&sb);
}

static char *detail_of(AstIdentifier *ident, const char *kind) {
    AstFunctionDefn *func = function_of(ident);
    if (func && (strcmp(kind, "function") == 0 || strcmp(kind, "method") == 0)) return signature_of(func);
    if (strcmp(kind, "struct") == 0 || strcmp(kind, "union") == 0 || strcmp(kind, "enum") == 0) return strdup(kind);
    if (ident->type && ident->type->kind != TYPE_NAME) return type_to_str(ident->type);
    return strdup("");
}

int index_identifier(AstIdentifier *ident) {
    if (!program_index.enabled || ident == NULL) return -1;
    ident = declaration_of(ident);
    if (!has_position((Ast *)ident) || ident->name == NULL || ident->name[0] == '$') return -1;
    if (!spells(ident->head.file, ident->head.start, ident->head.end, ident->name, true)) return -1;
    if (ident->flags & (IDENTIFIER_IS_NAMESPACE | IDENTIFIER_IS_AMBIGUOUS)) return -1;

    int existing = find_symbol(ident->head.file, ident->head.start.input_idx);
    if (existing >= 0) return existing;

    IndexSymbol symbol = {0};
    symbol.file       = ident->head.file;
    symbol.name_start = ident->head.start;
    symbol.name_end   = ident->head.end;
    symbol.start      = ident->head.start;
    symbol.end        = ident->head.end;
    symbol.name       = ident->name;
    symbol.kind       = kind_of(ident);
    symbol.detail     = detail_of(ident, symbol.kind);
    symbol.container  = -1;

    // The whole declaration
    Ast *whole = NULL;
    if (ident->type && ident->type->kind == TYPE_STRUCT && (ident->flags & IDENTIFIER_IS_NAME_OF_STRUCT)) whole = (Ast *)((TypeStruct *)ident->type)->node;
    else if (ident->type && ident->type->kind == TYPE_ENUM && (ident->flags & IDENTIFIER_IS_NAME_OF_ENUM)) whole = (Ast *)((TypeEnum *)ident->type)->node;
    else if (ident->decl) whole = (Ast *)ident->decl;
    if (whole && whole->file == ident->head.file && whole->start.line > 0 && whole->end.input_idx >= ident->head.end.input_idx) {
        symbol.start = whole->start.input_idx <= ident->head.start.input_idx ? whole->start : ident->head.start;
        symbol.end   = whole->end;
    }

    int id = add_symbol(symbol);

    // What it is in: the struct of a member or of a method
    AstIdentifier *owner = NULL;
    if (ident->flags & IDENTIFIER_IS_STRUCT_MEMBER) {
        AstStruct *node = struct_of_member(ident);
        if (node) owner = node->generic_struct ? node->generic_struct->identifier : node->identifier;
    }
    AstFunctionDefn *func = function_of(ident);
    if (func && func->is_method && func->receiver_struct) owner = func->receiver_struct->identifier;
    if (owner) {
        int container = index_identifier(owner);
        ((IndexSymbol *)program_index.symbols.items)[id].container = container;
    }
    return id;
}

static int index_enumerator(AstEnumerator *etor) {
    if (!program_index.enabled || !has_position((Ast *)etor)) return -1;

    int existing = find_symbol(etor->head.file, etor->head.start.input_idx);
    if (existing >= 0) return existing;

    IndexSymbol symbol = {0};
    symbol.file       = etor->head.file;
    symbol.name_start = etor->head.start;
    symbol.name_end   = etor->head.end;
    symbol.start      = etor->head.start;
    symbol.end        = etor->head.end;
    symbol.name       = etor->name;
    symbol.kind       = "enumMember";
    symbol.container  = -1;

    StringBuilder sb = sb_init(16);
    sb_append(&sb, "%lld", (long long)etor->value);
    symbol.detail = sb_to_string(&sb);

    int id = add_symbol(symbol);
    if (etor->parent && !etor->parent->is_anonymous && etor->parent->identifier) {
        int container = index_identifier(etor->parent->identifier);
        ((IndexSymbol *)program_index.symbols.items)[id].container = container;
    }
    return id;
}

// 'spelled' is the name as it is written there, e.g the alias of an import
static void add_reference(AstFile *file, Pos start, Pos end, int symbol, const char *spelled) {
    if (symbol < 0 || file == NULL || start.line <= 0) return;
    if (!spells(file, start, end, spelled, false)) return;

    if (reference_table.capacity > 0) {
        uint64_t slot = index_key(file, start.input_idx, end.input_idx) & (reference_table.capacity - 1);
        while (reference_table.slots[slot]) {
            IndexReference *r = &((IndexReference *)program_index.references.items)[reference_table.slots[slot] - 1];
            if (r->file == file && r->start.input_idx == start.input_idx && r->end.input_idx == end.input_idx) return;
            slot = (slot + 1) & (reference_table.capacity - 1);
        }
    }

    IndexReference reference = { file, start, end, symbol };
    da_append(&program_index.references, reference);
    if ((program_index.references.count + 1) * 10 > reference_table.capacity * 7) {
        table_grow(&reference_table, &program_index.references, reference_key);
    } else {
        uint64_t slot = reference_key(&reference) & (reference_table.capacity - 1);
        while (reference_table.slots[slot]) slot = (slot + 1) & (reference_table.capacity - 1);
        reference_table.slots[slot] = program_index.references.count;
    }
}

void index_name_use(Ast *site, Pos start, Pos end, AstIdentifier *ident) {
    if (!program_index.enabled || !has_position(site) || ident == NULL) return;
    add_reference(site->file, start, end, index_identifier(ident), ident->name);
}

void index_identifier_use(Ast *site, AstIdentifier *ident) {
    if (site == NULL) return;
    index_name_use(site, site->start, site->end, ident);
}

void index_enumerator_use(Ast *site, AstEnumerator *etor) {
    if (!program_index.enabled || !has_position(site) || etor == NULL) return;
    add_reference(site->file, site->start, site->end, index_enumerator(etor), etor->name);
}

// A namespace is declared by the 'namespace a.b;' of its first file
static int index_namespace(Namespace *ns) {
    if (!program_index.enabled || ns == NULL) return -1;
    for (int i = 0; i < ns->files.count; i++) {
        AstFile *file = ((AstFile **)ns->files.items)[i];
        if (file->ns_name_start.line <= 0) continue;

        int existing = find_symbol(file, file->ns_name_start.input_idx);
        if (existing >= 0) return existing;

        IndexSymbol symbol = {0};
        symbol.file       = file;
        symbol.name_start = file->ns_name_start;
        symbol.name_end   = file->ns_name_end;
        symbol.start      = file->ns_name_start;
        symbol.end        = file->ns_name_end;
        symbol.name       = ns->name;
        symbol.kind       = "namespace";
        symbol.detail     = strdup("namespace");
        symbol.container  = -1;
        return add_symbol(symbol);
    }
    return -1;
}

void index_namespace_use(Ast *site, Pos start, Pos end, Namespace *ns, const char *spelled) {
    if (!program_index.enabled || site == NULL || site->file == NULL) return;
    add_reference(site->file, start, end, index_namespace(ns), spelled);
}

//
// The declarations of the files, also the ones that are not used, for the outline of a file
//

// The parameters of a function are parameters, not variables. The compiler doesn't mark their declarations, so it is done here
static void index_parameters(AstIdentifier *func_ident) {
    AstFunctionDefn *func = function_of(func_ident);
    if (func == NULL) return;
    for (int i = 0; i < func->parameters.count; i++) {
        int id = index_identifier(((AstIdentifier **)func->parameters.items)[i]);
        if (id >= 0) ((IndexSymbol *)program_index.symbols.items)[id].kind = "parameter";
    }
}

static void index_struct_declarations(AstIdentifier *struct_ident, AstStruct *node) {
    if (node == NULL) return;
    int owner = index_identifier(struct_ident);

    DynamicArray members = node->members->identifiers;
    for (int i = 0; i < members.count; i++) {
        int id = index_identifier(((AstIdentifier **)members.items)[i]);
        if (id < 0) continue;
        IndexSymbol *member = &((IndexSymbol *)program_index.symbols.items)[id];
        member->kind      = node->is_union ? "variant" : "field";
        member->container = owner;
    }
    DynamicArray statics = node->static_members->identifiers;
    for (int i = 0; i < statics.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)statics.items)[i];
        if (ident == struct_ident || (ident->flags & IDENTIFIER_IS_NAME_OF_STRUCT)) continue;
        int id = index_identifier(ident);
        if (id >= 0) ((IndexSymbol *)program_index.symbols.items)[id].container = owner;
    }
    DynamicArray methods = node->methods->identifiers;
    for (int i = 0; i < methods.count; i++) {
        AstIdentifier *method = ((AstIdentifier **)methods.items)[i];
        int id = index_identifier(method);
        if (id >= 0) ((IndexSymbol *)program_index.symbols.items)[id].container = owner;
        index_parameters(method);
    }
}

void index_declarations(DynamicArray files) {
    if (!program_index.enabled) return;
    for (int f = 0; f < files.count; f++) {
        AstFile *file = ((AstFile **)files.items)[f];
        if (file->ns) index_namespace(file->ns);
        DynamicArray idents = file->scope->identifiers;
        for (int i = 0; i < idents.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)idents.items)[i];
            if (ident->head.file != file || ident->imported_from) continue;
            if (ident->flags & (IDENTIFIER_IS_NAMESPACE | IDENTIFIER_IS_IMPORTED_NAME | IDENTIFIER_IS_AMBIGUOUS)) continue;

            index_identifier(ident);
            index_parameters(ident);
            if ((ident->flags & IDENTIFIER_IS_NAME_OF_STRUCT) && ident->type && ident->type->kind == TYPE_STRUCT) {
                index_struct_declarations(ident, ((TypeStruct *)ident->type)->node);
            }
            if ((ident->flags & IDENTIFIER_IS_NAME_OF_ENUM) && ident->type && ident->type->kind == TYPE_ENUM) {
                AstEnum *node = ((TypeEnum *)ident->type)->node;
                for (int e = 0; node && e < node->enumerators.count; e++) index_enumerator(((AstEnumerator **)node->enumerators.items)[e]);
            }
        }
    }
}

//
// JSON
//

// The '//' lines right above a line, without the slashes. Attributes between them and the declaration are skipped
static char *doc_comment(AstFile *file, int line) {
    if (file->text == NULL || line <= 1) return NULL;

    // The start of each line
    DynamicArray starts = da_init(256, sizeof(int));
    int zero = 0;
    da_append(&starts, zero);
    for (int i = 0; file->text[i]; i++) {
        if (file->text[i] == '\n') { int next = i + 1; da_append(&starts, next); }
    }

    StringBuilder sb = sb_init(64);
    int first = line - 1; // The line above, counting from 1
    int last  = first;
    bool found = false;
    for (int l = line - 1; l >= 1 && l <= starts.count; l--) {
        char *text = file->text + ((int *)starts.items)[l - 1];
        while (*text == ' ' || *text == '\t') text++;
        if (text[0] == '@' && !found) { last = l - 1; continue; }
        if (text[0] != '/' || text[1] != '/') break;
        first = l;
        found = true;
    }
    if (!found) { da_free(&starts); return NULL; }

    for (int l = first; l <= last; l++) {
        char *text = file->text + ((int *)starts.items)[l - 1];
        while (*text == ' ' || *text == '\t') text++;
        text += 2;
        if (*text == '/') continue; // e.g ////// separators
        if (*text == ' ') text++;
        char *end = text;
        while (*end && *end != '\n' && *end != '\r') end++;
        if (sb.cursor > 0) sb_append(&sb, "\n");
        sb_append(&sb, "%.*s", (int)(end - text), text);
    }
    da_free(&starts);
    if (sb.cursor == 0) return NULL;
    return sb_to_string(&sb);
}

static int file_id(DynamicArray *files, AstFile *file) {
    for (int i = 0; i < files->count; i++) {
        if (((AstFile **)files->items)[i] == file) return i;
    }
    da_append(files, file);
    return files->count - 1;
}

static void json_write_range(StringBuilder *sb, Pos start, Pos end) {
    sb_append(sb, "{\"start\": ");
    json_write_position(sb, start);
    sb_append(sb, ", \"end\": ");
    json_write_position(sb, end);
    sb_append(sb, "}");
}

static bool is_std_file(AstFile *file) {
    if (program_index.std_path == NULL) return false;
    char *absolute = get_absolute_path(file->absolute_path);
    size_t length  = strlen(program_index.std_path);
    bool is_std    = absolute && strncmp(absolute, program_index.std_path, length) == 0;
    free(absolute);
    return is_std;
}

// A symbol is written when it is declared in the program, or used by it. Then so is what it is in
static void keep_symbol(int *keep, int id) {
    while (id >= 0 && !keep[id]) {
        keep[id] = 1;
        id = ((IndexSymbol *)program_index.symbols.items)[id].container;
    }
}

void write_index_json(FILE *out, bool success) {
    StringBuilder sb = sb_init(1 << 16);
    DynamicArray files = da_init(16, sizeof(AstFile *));

    int symbol_count = program_index.symbols.count;
    int *keep   = calloc(symbol_count + 1, sizeof(int));
    int *new_id = calloc(symbol_count + 1, sizeof(int));
    bool *in_std = calloc(symbol_count + 1, sizeof(bool));
    for (int i = 0; i < symbol_count; i++) {
        IndexSymbol *s = &((IndexSymbol *)program_index.symbols.items)[i];
        in_std[i] = !program_index.with_std && is_std_file(s->file);
        if (!in_std[i]) keep_symbol(keep, i);
    }
    DynamicArray kept_references = da_init(1024, sizeof(IndexReference));
    for (int i = 0; i < program_index.references.count; i++) {
        IndexReference r = ((IndexReference *)program_index.references.items)[i];
        if (!program_index.with_std && is_std_file(r.file)) continue;
        keep_symbol(keep, r.symbol);
        da_append(&kept_references, r);
    }
    int next_id = 0;
    for (int i = 0; i < symbol_count; i++) new_id[i] = keep[i] ? next_id++ : -1;

    sb_append(&sb, "{\"version\": 1, \"success\": %s,\n\"symbols\": [", success ? "true" : "false");
    bool first_symbol = true;
    for (int i = 0; i < symbol_count; i++) {
        if (!keep[i]) continue;
        IndexSymbol *s = &((IndexSymbol *)program_index.symbols.items)[i];
        if (!first_symbol) sb_append(&sb, ",");
        first_symbol = false;
        sb_append(&sb, "\n  {\"id\": %d, \"name\": ", new_id[i]);
        json_write_string(&sb, s->name);
        sb_append(&sb, ", \"kind\": \"%s\", \"file\": %d, \"nameRange\": ", s->kind, file_id(&files, s->file));
        json_write_range(&sb, s->name_start, s->name_end);
        sb_append(&sb, ", \"range\": ");
        json_write_range(&sb, s->start, s->end);
        sb_append(&sb, ", \"detail\": ");
        json_write_string(&sb, s->detail ? s->detail : "");
        sb_append(&sb, ", \"container\": %d", s->container >= 0 ? new_id[s->container] : -1);
        bool can_have_doc = strcmp(s->kind, "variable") != 0 && strcmp(s->kind, "parameter") != 0;
        char *doc = can_have_doc ? doc_comment(s->file, s->start.line) : NULL;
        if (doc) {
            sb_append(&sb, ", \"doc\": ");
            json_write_string(&sb, doc);
        }
        sb_append(&sb, "}");
    }

    sb_append(&sb, "\n],\n\"references\": [");
    for (int i = 0; i < kept_references.count; i++) {
        IndexReference *r = &((IndexReference *)kept_references.items)[i];
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\n  {\"file\": %d, \"range\": ", file_id(&files, r->file));
        json_write_range(&sb, r->start, r->end);
        sb_append(&sb, ", \"symbol\": %d}", new_id[r->symbol]);
    }
    (void)in_std;

    sb_append(&sb, "\n],\n\"files\": [");
    for (int i = 0; i < files.count; i++) {
        AstFile *file = ((AstFile **)files.items)[i];
        char *absolute = get_absolute_path(file->absolute_path);
        if (i > 0) sb_append(&sb, ",");
        sb_append(&sb, "\n  ");
        json_write_string(&sb, absolute ? absolute : file->absolute_path);
    }

    sb_append(&sb, "\n],\n\"diagnostics\": ");
    write_diagnostics_array(&sb);
    sb_append(&sb, "\n}\n");

    fwrite(sb.buffer, 1, sb.cursor, out);
    fflush(out);
}
