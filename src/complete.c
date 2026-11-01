//
// 'saad complete main.sd --at file:line:character' writes, as JSON, what can be written at a position: the names for
// completion, and the signature of the call the position is in, for signature help. See docs/lsp.md.
//
// The program is checked first. As the parser and the typer go on after errors, the code around the position is checked
// even when the line that is being written isn't whole. Then the text before the position says what is wanted:
//
//   * 'a.b.na'  the members of the value, type or namespace before the last '.', e.g fields and methods, the values of
//               an enum, the variants of a union or the declarations of a namespace
//   * 'na'      the names in scope at the position, and the keywords
//   * 'f(a, '   the signature of f, and which parameter is being written
//
// Included by compiler.c after index.c, whose kind_of(), detail_of() and doc_comment() it uses
//

#include <ctype.h>
#include <limits.h>

typedef struct Completion {
    bool  enabled;
    char *path;       // The file, as given
    int   line;       // From 0
    int   character;  // From 0, a byte of the line

    // The answer
    StringBuilder items;      // JSON objects, separated by commas
    int           item_count;
    char         *signature;  // A JSON object, or NULL
} Completion;

Completion program_completion;

#define MAX_SEGMENTS 16

// A part of a chain like 'game.players[0].name()': a name, called or indexed
typedef struct Segment {
    char name[128];
    bool call;
    bool index;
} Segment;

// What a chain is: a namespace, a type by its name (e.g Shape. or Status.), or a value of a type
typedef struct Target {
    Namespace     *ns;
    AstIdentifier *type_name;
    Type          *type;
} Target;

static bool is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

// The offset of a line and a byte of it in the text
static int offset_of(const char *text, int line, int character) {
    int offset = 0;
    for (int l = 0; l < line && text[offset]; offset++) {
        if (text[offset] == '\n') l++;
    }
    for (int c = 0; c < character && text[offset] && text[offset] != '\n'; c++) offset++;
    return offset;
}

// The file of the program with the path
static AstFile *completion_file(DynamicArray files, const char *path) {
    char *absolute = get_absolute_path(path);
    for (int i = 0; i < files.count; i++) {
        AstFile *file = ((AstFile **)files.items)[i];
        if (absolute && same_path(file->absolute_path, absolute)) return file;
        if (same_path(file->absolute_path, path)) return file;
    }
    return NULL;
}

// The innermost block at the offset, or the scope of the file
static AstBlock *scope_at(AstFile *file, int offset) {
    AstBlock *best = file->scope;
    int best_size = INT_MAX;
    for (int i = 0; i < file->blocks.count; i++) {
        AstBlock *block = ((AstBlock **)file->blocks.items)[i];
        int start = block->head.start.input_idx;
        int end   = block->head.end.input_idx;
        if (end <= start || offset <= start || offset > end) continue;
        if (end - start < best_size) {
            best = block;
            best_size = end - start;
        }
    }
    return best;
}

// Reads a chain backwards from just before 'end', e.g 'a.b(1).c' of 'a.b(1).c.'. Returns the number of segments, or -1
static int read_chain(const char *text, int end, Segment *segments) {
    Segment reversed[MAX_SEGMENTS];
    int count = 0;
    int at = end;
    while (count < MAX_SEGMENTS) {
        Segment segment = {0};

        // Calls and indexes after the name, e.g the (1) of b(1)
        while (at > 0 && (text[at - 1] == ')' || text[at - 1] == ']')) {
            char close = text[at - 1];
            char open  = close == ')' ? '(' : '[';
            int depth = 0;
            int p = at - 1;
            for (; p >= 0; p--) {
                if (text[p] == close) depth++;
                if (text[p] == open && --depth == 0) break;
            }
            if (p < 0) return -1;
            if (close == ')') segment.call = true; else segment.index = true;
            at = p;
        }

        int name_end = at;
        while (at > 0 && is_name_char(text[at - 1])) at--;
        int length = name_end - at;
        if (length == 0 || length >= (int)sizeof(segment.name) || isdigit((unsigned char)text[at])) return -1;
        memcpy(segment.name, &text[at], length);
        reversed[count++] = segment;

        // Another part before it
        if (at > 1 && text[at - 1] == '.' && text[at - 2] != '.') {
            at -= 1;
            continue;
        }
        break;
    }
    for (int i = 0; i < count; i++) segments[i] = reversed[count - 1 - i];
    return count;
}

// The type a value of the type has after a call or an index, e.g the return type of a function
static Type *after_call_or_index(Typer *typer, Type *type, Segment *segment) {
    if (type == NULL) return NULL;
    if (segment->call) {
        if (type->kind != TYPE_FUNCTION) return NULL;
        AstFunctionDefn *func = ((TypeFunction *)type)->node;
        if (func->return_types.count == 0) return NULL;
        type = ((Type **)func->return_types.items)[0];
    }
    if (segment->index) {
        if (type->kind == TYPE_POINTER) type = ((TypePointer *)type)->pointer_to;
        if (type->kind == TYPE_ARRAY) return ((TypeArray *)type)->elem_type;
        if (type->kind == TYPE_STRING) return primitive_type(PRIMITIVE_U8);
        if (type->kind == TYPE_STRUCT) {
            AstStruct *node = ((TypeStruct *)type)->node;
            AstIdentifier *index = get_struct_method(node, "index");
            if (index && index->type && index->type->kind == TYPE_FUNCTION) {
                Segment call = { .call = true };
                return after_call_or_index(typer, index->type, &call);
            }
            // An instance whose index() isn't made yet, e.g List<T>: its elements are where 'data' points
            AstIdentifier *data = get_struct_member(node, "data");
            if (data && data->type && data->type->kind == TYPE_POINTER) return ((TypePointer *)data->type)->pointer_to;
        }
        return NULL;
    }
    return type;
}

static Type *value_type_of(Type *type) {
    if (type && type->kind == TYPE_POINTER) return ((TypePointer *)type)->pointer_to;
    return type;
}

// The member of a value of the type, e.g a field or a method
static AstIdentifier *member_of(Typer *typer, Type *type, const char *name) {
    type = value_type_of(type);
    if (type == NULL) return NULL;
    if (type->kind == TYPE_STRUCT) {
        AstStruct *node = ((TypeStruct *)type)->node;
        AstIdentifier *member = get_struct_member(node, (char *)name);
        if (!member) member = get_struct_method(node, (char *)name);
        // The methods of an instance are made when they are used, so one that isn't is a method of the generic struct
        if (!member && node->generic_struct) member = get_struct_method(node->generic_struct, (char *)name);
        if (member) return member;
    }
    TypeStruct *holder = method_holder_of(typer, type);
    if (holder && holder->node) {
        AstIdentifier *member = get_struct_member(holder->node, (char *)name);
        if (!member) member = get_struct_method(holder->node, (char *)name);
        return member;
    }
    return NULL;
}

// The position completion is asked for, so names declared below it in a function are not seen
static AstFile *completing_file;
static int      completing_offset;

// A name in scope at the position: in a function only the ones declared above it
static AstIdentifier *lookup_visible(AstBlock *scope, const char *name) {
    for (AstBlock *block = scope; block; block = block->parent) {
        bool is_file = completing_file && block == completing_file->scope;
        for (int i = 0; i < block->identifiers.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)block->identifiers.items)[i];
            if (strcmp(ident->name, name) != 0) continue;
            if (!is_file && ident->head.file == completing_file && ident->head.start.input_idx >= completing_offset) continue;
            return ident;
        }
        if (is_file) break;
    }
    return NULL;
}

// A private member of a struct of another namespace can't be used
static bool can_use(AstIdentifier *ident) {
    if (!(ident->flags & IDENTIFIER_IS_PRIVATE)) return true;
    AstFile *declared_in = ident->head.file;
    if (!declared_in || !completing_file) return true;
    if (declared_in == completing_file) return true;
    return declared_in->ns && declared_in->ns == completing_file->ns;
}

// What a declaration is when it is written, e.g a value of its type, or a type by its name
static bool target_of_identifier(Typer *typer, AstIdentifier *ident, Segment *segment, Target *target) {
    *target = (Target){0};
    if (ident->flags & IDENTIFIER_IS_NAMESPACE) {
        target->ns = ident->ns;
        return target->ns != NULL;
    }
    if (ident->flags & (IDENTIFIER_IS_NAME_OF_STRUCT | IDENTIFIER_IS_NAME_OF_ENUM | IDENTIFIER_IS_TYPE_ALIAS)) {
        if (segment->call || segment->index) return false;
        target->type_name = ident;
        return true;
    }
    target->type = after_call_or_index(typer, ident->type, segment);
    return target->type != NULL;
}

// The type that a type name names, e.g the struct of 'Shape' or the type of an alias
static Type *named_type(AstIdentifier *name) {
    return name->type;
}

// Follows the chain to what it is. 'skip_last' leaves out the last segment, e.g for the function of a call
static bool resolve_chain(Typer *typer, AstBlock *scope, Segment *segments, int count, Target *target) {
    typer->current_scope = scope;
    typer->parser->current_scope = scope;
    int next = 0;

    // A namespace, which can have dots in its name, e.g game.physics
    AstIdentifier *first = lookup_visible(scope, segments[0].name);
    if (!first || (first->flags & IDENTIFIER_IS_NAMESPACE)) {
        char *names[MAX_SEGMENTS];
        int plain = 0;
        while (plain < count && !segments[plain].call && !segments[plain].index) {
            names[plain] = segments[plain].name;
            plain++;
        }
        for (int k = plain; k >= 1; k--) {
            Namespace *ns = namespace_of_names(typer, names, k);
            if (ns) {
                *target = (Target){ .ns = ns };
                next = k;
                break;
            }
        }
        if (next == 0) {
            if (!first) return false;
            if (!target_of_identifier(typer, first, &segments[0], target)) return false;
            next = 1;
        }
    } else {
        if (!target_of_identifier(typer, first, &segments[0], target)) return false;
        next = 1;
    }

    for (int i = next; i < count; i++) {
        Segment *segment = &segments[i];
        if (target->ns) {
            AstIdentifier *ident = find_namespace_declaration(target->ns, segment->name);
            if (!ident || !target_of_identifier(typer, ident, segment, target)) return false;
            continue;
        }
        if (target->type_name) {
            Type *type = named_type(target->type_name);
            if (type && type->kind == TYPE_STRUCT) {
                AstStruct *node = ((TypeStruct *)type)->node;
                AstIdentifier *member = node->static_members ? find_identifier_in_scope(node->static_members, segment->name) : NULL;
                if (!member && node->is_union && get_struct_member(node, segment->name)) {
                    *target = (Target){ .type = type }; // e.g Shape.empty is a Shape
                    continue;
                }
                if (!member) member = get_struct_method(node, segment->name);
                if (!member || !target_of_identifier(typer, member, segment, target)) return false;
                continue;
            }
            if (type && type->kind == TYPE_ENUM) {
                *target = (Target){ .type = type }; // e.g Status.open is a Status
                continue;
            }
            return false;
        }
        AstIdentifier *member = member_of(typer, target->type, segment->name);
        if (!member || !member->type) return false;
        target->type = after_call_or_index(typer, member->type, segment);
        if (!target->type) return false;
    }
    return true;
}

//
// The items
//

static void add_item_inserting(const char *name, const char *kind, const char *detail, const char *doc, const char *insert);

static void add_item(const char *name, const char *kind, const char *detail, const char *doc) {
    add_item_inserting(name, kind, detail, doc, NULL);
}

// 'insert' is the text that is written instead of the name, e.g "radius: " for a field of a struct literal
static void add_item_inserting(const char *name, const char *kind, const char *detail, const char *doc, const char *insert) {
    StringBuilder *sb = &program_completion.items;
    if (program_completion.item_count > 0) sb_append(sb, ",");
    sb_append(sb, "\n  {\"label\": ");
    json_write_string(sb, name);
    sb_append(sb, ", \"kind\": ");
    json_write_string(sb, kind);
    sb_append(sb, ", \"detail\": ");
    json_write_string(sb, detail ? detail : "");
    if (doc && doc[0]) {
        sb_append(sb, ", \"doc\": ");
        json_write_string(sb, doc);
    }
    if (insert) {
        sb_append(sb, ", \"insert\": ");
        json_write_string(sb, insert);
    }
    sb_append(sb, "}");
    program_completion.item_count++;
}

// A name the program has written itself, not one the compiler made, e.g '$tag'
static bool is_visible_name(const char *name) {
    return name && name[0] && name[0] != '$' && name[0] != '_';
}

static void add_identifier(AstIdentifier *ident) {
    AstIdentifier *declared = ident->imported_from ? ident->imported_from : ident;
    if (!is_visible_name(ident->name) || (declared->flags & IDENTIFIER_IS_POISONED) || !can_use(declared)) return;

    const char *kind = (declared->flags & IDENTIFIER_IS_NAMESPACE) ? "namespace" : kind_of(declared);
    char *detail = (declared->flags & IDENTIFIER_IS_NAMESPACE) ? "namespace" : detail_of(declared, kind);
    char *doc = declared->head.file ? doc_comment(declared->head.file, declared->head.start.line) : NULL;
    add_item(ident->name, kind, detail, doc);
}

static bool already_added(DynamicArray *names, const char *name) {
    for (int i = 0; i < names->count; i++) {
        if (strcmp(((char **)names->items)[i], name) == 0) return true;
    }
    da_append(names, name);
    return false;
}

static void add_scope_identifiers(AstBlock *block, DynamicArray *seen) {
    if (!block) return;
    for (int i = 0; i < block->identifiers.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)block->identifiers.items)[i];
        if (!already_added(seen, ident->name)) add_identifier(ident);
    }
}

// The members of a value of the type: fields and methods. A slice or an array has count, data and the methods on slices
static void add_members(Typer *typer, Type *type) {
    DynamicArray seen = da_init(32, sizeof(char *));
    type = value_type_of(type);
    if (type == NULL) return;

    if (type->kind == TYPE_STRUCT) {
        AstStruct *node = ((TypeStruct *)type)->node;
        if (!node->is_union) add_scope_identifiers(node->members, &seen); // The variants of a union are read with a switch
        else if (!already_added(&seen, "kind")) add_item("kind", "field", "the variant it holds", NULL);
        add_scope_identifiers(node->methods, &seen);
        if (node->generic_struct) add_scope_identifiers(node->generic_struct->methods, &seen); // The ones that aren't made yet
    }
    if (type->kind == TYPE_ARRAY) {
        if (!already_added(&seen, "count")) add_item("count", "field", "i64", "The number of elements");
        if (!already_added(&seen, "data")) add_item("data", "field", "pointer to the first element", NULL);
        for (int i = 0; i < typer->slice_methods.count; i++) {
            AstFunctionDefn *method = ((AstFunctionDefn **)typer->slice_methods.items)[i];
            if (method->identifier && !already_added(&seen, method->identifier->name)) add_identifier(method->identifier);
        }
    }
    TypeStruct *holder = type->kind == TYPE_STRUCT ? NULL : method_holder_of(typer, type);
    if (holder && holder->node) {
        add_scope_identifiers(holder->node->members, &seen);
        add_scope_identifiers(holder->node->methods, &seen);
    }
}

// Writing a type by its name, e.g the values of an enum, the variants of a union or the constants of a struct
static void add_static_members(AstIdentifier *type_name) {
    DynamicArray seen = da_init(32, sizeof(char *));
    Type *type = named_type(type_name);
    if (type == NULL) return;
    if (type->kind == TYPE_ENUM) {
        AstEnum *node = ((TypeEnum *)type)->node;
        for (int i = 0; i < node->enumerators.count; i++) {
            AstEnumerator *etor = ((AstEnumerator **)node->enumerators.items)[i];
            char value[32];
            snprintf(value, sizeof(value), "%d", etor->value);
            add_item(etor->name, "enumMember", value, NULL);
        }
        return;
    }
    if (type->kind == TYPE_STRUCT) {
        AstStruct *node = ((TypeStruct *)type)->node;
        if (node->is_union) {
            for (int i = 0; i < node->members->identifiers.count; i++) {
                AstIdentifier *variant = ((AstIdentifier **)node->members->identifiers.items)[i];
                if (is_visible_name(variant->name) && !already_added(&seen, variant->name)) add_item(variant->name, "variant", variant->type ? type_to_str(variant->type) : "", NULL);
            }
        }
        if (node->static_members) add_scope_identifiers(node->static_members, &seen);
    }
}

// The public declarations of a namespace, and the private ones for a file of the same namespace
static void add_namespace_declarations(Namespace *ns, AstFile *from) {
    DynamicArray seen = da_init(64, sizeof(char *));
    for (int i = 0; i < ns->files.count; i++) {
        AstFile *file = ((AstFile **)ns->files.items)[i];
        for (int j = 0; j < file->scope->identifiers.count; j++) {
            AstIdentifier *ident = ((AstIdentifier **)file->scope->identifiers.items)[j];
            if (!is_declared_by(ident, file)) continue;
            if ((ident->flags & IDENTIFIER_IS_PRIVATE) && from->ns != ns) continue;
            if (!already_added(&seen, ident->name)) add_identifier(ident);
        }
    }
}

static const char *keywords[] = {
    "if", "else", "for", "while", "in", "switch", "case", "return", "break", "continue", "struct", "union", "enum",
    "method", "import", "use", "namespace", "extern", "as", "new", "assert", "typeof", "sizeof", "true", "false", "null",
    "int", "uint", "float", "bool", "string", "any", "void", "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "f32", "f64",
};

// The names in scope at the position: locals declared above it, parameters, the file's declarations and imports
static void add_names_in_scope(AstFile *file, AstBlock *scope, int offset) {
    DynamicArray seen = da_init(128, sizeof(char *));
    for (AstBlock *block = scope; block; block = block->parent) {
        bool is_file = block == file->scope;
        for (int i = 0; i < block->identifiers.count; i++) {
            AstIdentifier *ident = ((AstIdentifier **)block->identifiers.items)[i];
            // In a function, only what is declared above. A parameter is declared before its body
            if (!is_file && ident->head.file == file && ident->head.start.input_idx >= offset) continue;
            if (!already_added(&seen, ident->name)) add_identifier(ident);
        }
        if (is_file) break;
    }
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (!already_added(&seen, keywords[i])) add_item(keywords[i], "keyword", "", NULL);
    }
}

//
// Signature help
//

// If the closing quote at p ends a character literal, e.g 'a' or '\n', where it starts. Otherwise p
static int start_of_character(const char *text, int p) {
    if (text[p] != '\'') return p;
    if (p >= 2 && text[p - 2] == '\'' && text[p - 1] != '\\') return p - 2;                      // 'a'
    if (p >= 3 && text[p - 3] == '\'' && text[p - 2] == '\\') return p - 3;                       // '\n'
    if (p >= 5 && text[p - 5] == '\'' && text[p - 4] == '\\' && text[p - 3] == 'x') return p - 5;  // '\x41'
    return p;
}

// The '(' of the call the offset is in, and which argument it is at. -1 if it isn't in one
static int open_paren_of_call(const char *text, int offset, int *argument) {
    int depth = 0;
    *argument = 0;
    for (int p = offset - 1; p >= 0; p--) {
        p = start_of_character(text, p);
        if (text[p] == '\'') continue;
        char c = text[p];
        if (c == '"') {  // Skips a string backwards
            for (p--; p >= 0 && !(text[p] == '"' && (p == 0 || text[p - 1] != '\\')); p--) {}
            continue;
        }
        if (c == ')' || c == ']' || c == '}') depth++;
        else if (c == '[' || c == '{') {
            if (depth == 0) return -1; // e.g in a block, not a call
            depth--;
        }
        else if (c == '(') {
            if (depth == 0) return p;
            depth--;
        }
        else if (c == ',' && depth == 0) (*argument)++;
        else if (c == ';' && depth == 0) return -1;
    }
    return -1;
}

static void write_signature(AstIdentifier *ident, AstFunctionDefn *func, bool skip_receiver, int argument) {
    StringBuilder label  = sb_init(128);
    StringBuilder params = sb_init(128);
    sb_append(&label, "%s(", ident->name);
    sb_append(&params, "[");

    int first = (skip_receiver && func->is_method) ? 1 : 0;
    int shown = 0;
    for (int i = first; i < func->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func->parameters.items)[i];
        StringBuilder one = sb_init(64);
        sb_append(&one, "%s", parameter_text(param));
        if (shown > 0) {
            sb_append(&label, ", ");
            sb_append(&params, ", ");
        }
        sb_append(&label, "%s", one.buffer);
        json_write_string(&params, one.buffer);
        shown++;
    }
    sb_append(&label, ")");
    for (int i = 0; i < func->return_types.count; i++) {
        Type *type = ((Type **)func->return_types.items)[i];
        if (type->kind == TYPE_VOID) continue;
        sb_append(&label, i == 0 ? " -> %s" : ", %s", type_to_str(type));
    }
    sb_append(&params, "]");

    // A variadic parameter takes the rest of the arguments
    int active = argument;
    for (int i = first; i < func->parameters.count; i++) {
        AstIdentifier *param = ((AstIdentifier **)func->parameters.items)[i];
        if (param->type && param->type->kind == TYPE_VARIADIC && active > i - first) active = i - first;
    }
    if (shown > 0 && active >= shown) active = shown - 1;

    AstIdentifier *declared = ident->imported_from ? ident->imported_from : ident;
    char *doc = declared->head.file ? doc_comment(declared->head.file, declared->head.start.line) : NULL;

    StringBuilder sb = sb_init(256);
    sb_append(&sb, "{\"label\": ");
    json_write_string(&sb, label.buffer);
    sb_append(&sb, ", \"parameters\": %s, \"active\": %d", params.buffer, active);
    if (doc && doc[0]) {
        sb_append(&sb, ", \"doc\": ");
        json_write_string(&sb, doc);
    }
    sb_append(&sb, "}");
    program_completion.signature = sb.buffer;
}

// The call the offset is in: its function, and the argument at the offset. 'on_value' is set for a method called on a
// value, whose receiver isn't an argument
typedef struct Call {
    AstIdentifier   *ident;
    AstFunctionDefn *func;
    bool             on_value;
    int              argument;
} Call;

static bool call_at(Typer *typer, AstFile *file, AstBlock *scope, int offset, Call *call) {
    *call = (Call){0};
    int open = open_paren_of_call(file->text, offset, &call->argument);
    if (open < 0) return false;

    int end = open;
    while (end > 0 && (file->text[end - 1] == ' ' || file->text[end - 1] == '\t')) end--;
    Segment segments[MAX_SEGMENTS];
    int count = read_chain(file->text, end, segments);
    if (count <= 0) return false;

    // The function is the last part. The parts before it are what it is called on
    AstBlock *call_scope = scope_at(file, open);
    if (call_scope) scope = call_scope;
    AstIdentifier *function = NULL;
    if (count == 1) {
        function = lookup_visible(scope, segments[0].name);
    } else {
        Target target;
        if (!resolve_chain(typer, scope, segments, count - 1, &target)) return false;
        const char *name = segments[count - 1].name;
        if (target.ns) function = find_namespace_declaration(target.ns, (char *)name);
        else if (target.type_name && named_type(target.type_name) && named_type(target.type_name)->kind == TYPE_STRUCT) {
            function = get_struct_method(((TypeStruct *)named_type(target.type_name))->node, (char *)name);
        }
        else if (target.type) {
            function = member_of(typer, target.type, name);
            call->on_value = true;
            // A method on slices is a function, and the slice is its first argument
            if (!function && value_type_of(target.type)->kind == TYPE_ARRAY) {
                for (int i = 0; i < typer->slice_methods.count; i++) {
                    AstFunctionDefn *method = ((AstFunctionDefn **)typer->slice_methods.items)[i];
                    if (method->identifier && strcmp(method->identifier->name, name) == 0) {
                        call->ident = method->identifier;
                        call->func  = method;
                        return true;
                    }
                }
            }
        }
    }
    if (!function || !function->type || function->type->kind != TYPE_FUNCTION) return false;
    call->ident = function;
    call->func  = ((TypeFunction *)function->type)->node;
    return true;
}

static void complete_signature(Typer *typer, AstFile *file, AstBlock *scope, int offset) {
    Call call;
    if (call_at(typer, file, scope, offset, &call)) write_signature(call.ident, call.func, call.on_value, call.argument);
}

// The type of the parameter that the argument at the offset is given to, or NULL
static Type *parameter_type_at(Typer *typer, AstFile *file, AstBlock *scope, int offset) {
    Call call;
    if (!call_at(typer, file, scope, offset, &call)) return NULL;
    int index = call.argument + ((call.on_value && call.func->is_method) ? 1 : 0);
    if (index >= call.func->parameters.count) index = call.func->parameters.count - 1; // A variadic parameter
    if (index < 0) return NULL;
    AstIdentifier *param = ((AstIdentifier **)call.func->parameters.items)[index];
    return param->type;
}

//
// Struct literals, e.g 'Point{x: 1, |}': the fields that aren't written yet, and the type of the field a value is for
//

typedef struct Literal {
    AstStruct *node;       // The struct of the literal
    int        open;       // Its '{'
    char       field[128]; // The field whose value is at the offset, e.g 'x' of 'Point{x: |'. Empty when a name is written
    bool       in_value;   // After 'name:', so a value is written, not the name of a field
} Literal;

static void skip_spaces_back(const char *text, int *at) {
    while (*at > 0 && isspace((unsigned char)text[*at - 1])) (*at)--;
}

// The struct literal the offset is directly in, not in a call or an array of it
static bool literal_at(Typer *typer, AstFile *file, int offset, Literal *literal) {
    const char *text = file->text;
    *literal = (Literal){0};
    int depth = 0;
    int element = -1; // The start of the element at the offset: after its ',' or the '{'
    int open = -1;
    for (int p = offset - 1; p >= 0; p--) {
        p = start_of_character(text, p);
        if (text[p] == '\'') continue;
        char c = text[p];
        if (c == '"') {
            for (p--; p >= 0 && !(text[p] == '"' && (p == 0 || text[p - 1] != '\\')); p--) {}
            continue;
        }
        if (c == ')' || c == ']' || c == '}') depth++;
        else if (c == '(' || c == '[') {
            if (depth == 0) return false;
            depth--;
        }
        else if (c == '{') {
            if (depth == 0) { open = p; break; }
            depth--;
        }
        else if (c == ',' && depth == 0 && element < 0) element = p + 1;
        else if (c == ';' && depth == 0) return false;
    }
    if (open < 0) return false;
    if (element < 0) element = open + 1;

    // The type before the '{', e.g Point, geometry.Vec2 or Pair<int>
    int end = open;
    skip_spaces_back(text, &end);
    if (end > 0 && text[end - 1] == '>') {
        int angle = 0;
        for (end--; end >= 0; end--) {
            if (text[end] == '>') angle++;
            if (text[end] == '<' && --angle == 0) break;
        }
        if (end < 0) return false;
    }
    Segment segments[MAX_SEGMENTS];
    int count = read_chain(text, end, segments);
    if (count <= 0) return false;
    Target target;
    if (!resolve_chain(typer, scope_at(file, open), segments, count, &target) || !target.type_name) return false;
    Type *type = named_type(target.type_name);
    if (!type || type->kind != TYPE_STRUCT) return false;
    literal->node = ((TypeStruct *)type)->node;
    literal->open = open;

    // 'name: value' or just a name
    int colon = -1;
    for (int p = element; p < offset; p++) {
        if (text[p] == ':' && text[p + 1] != ':' && text[p + 1] != '=') { colon = p; break; }
    }
    if (colon >= 0) {
        int name_end = colon;
        skip_spaces_back(text, &name_end);
        int name_start = name_end;
        while (name_start > element && is_name_char(text[name_start - 1])) name_start--;
        int length = name_end - name_start;
        if (length > 0 && length < (int)sizeof(literal->field)) memcpy(literal->field, &text[name_start], length);
        literal->in_value = true;
    } else {
        for (int p = element; p < offset; p++) {
            if (!isspace((unsigned char)text[p]) && !is_name_char(text[p])) return false; // e.g 'Vec2{1, 2 + |', which has no names
        }
    }
    return true;
}

// If the field is written in the literal already, e.g the 'x' of 'Point{x: 1, |'
static bool field_is_written(const char *text, int open, int offset, const char *name) {
    int length = strlen(name);
    int depth = 0;
    for (int p = open + 1; p + length < offset; p++) {
        char c = text[p];
        if (c == '(' || c == '[' || c == '{') depth++;
        if (c == ')' || c == ']' || c == '}') depth--;
        if (depth != 0 || strncmp(&text[p], name, length) != 0) continue;
        if (p > 0 && is_name_char(text[p - 1])) continue;
        int after = p + length;
        while (after < offset && isspace((unsigned char)text[after])) after++;
        if (text[after] == ':') return true;
    }
    return false;
}

static void add_literal_fields(Literal *literal, const char *text, int offset) {
    AstStruct *node = literal->node;
    for (int i = 0; i < node->members->identifiers.count; i++) {
        AstIdentifier *member = ((AstIdentifier **)node->members->identifiers.items)[i];
        if (!is_visible_name(member->name) || (member->flags & IDENTIFIER_IS_STRUCT_METHOD)) continue;
        if (member->type && member->type->kind == TYPE_FUNCTION) continue;
        if (field_is_written(text, literal->open, offset, member->name)) continue;
        char insert[160];
        snprintf(insert, sizeof(insert), "%s: ", member->name);
        AstFile *declared_in = member->head.file;
        char *doc = declared_in ? doc_comment(declared_in, member->head.start.line) : NULL;
        add_item_inserting(member->name, node->is_union ? "variant" : "field", member->type ? type_to_str(member->type) : "", doc, insert);
    }
}

//
// A '.' with nothing before it, e.g 'status == .open'. Its type comes from where it is written
//

static bool ends_with_word(const char *text, int end, const char *word) {
    int length = strlen(word);
    if (end < length || strncmp(&text[end - length], word, length) != 0) return false;
    return end == length || !is_name_char(text[end - length - 1]);
}

// The value type of a chain that ends just before 'end', e.g the 'status' of 'status == '
static Type *type_of_chain_before(Typer *typer, AstFile *file, AstBlock *scope, int end) {
    skip_spaces_back(file->text, &end);
    Segment segments[MAX_SEGMENTS];
    int count = read_chain(file->text, end, segments);
    if (count <= 0) return NULL;
    Target target;
    if (!resolve_chain(typer, scope, segments, count, &target)) return NULL;
    if (target.type_name) return NULL;
    return target.type;
}

// The type that the '.' at 'dot' is a value of, or NULL
static Type *expected_type_at(Typer *typer, AstFile *file, AstBlock *scope, int dot) {
    const char *text = file->text;
    int at = dot;
    skip_spaces_back(text, &at);
    if (at == 0) return NULL;
    char c = text[at - 1];
    char before = at > 1 ? text[at - 2] : 0;

    // a == .open, a != .open
    if (c == '=' && (before == '=' || before == '!')) return type_of_chain_before(typer, file, scope, at - 2);

    // a = .open, a: Status = .open, but not a := .open, which has no type to go by
    if (c == '=' && before != ':' && before != '<' && before != '>' && before != '=') {
        int end = at - 1;
        skip_spaces_back(text, &end);
        Segment segments[MAX_SEGMENTS];
        int count = read_chain(text, end, segments);
        if (count <= 0) return NULL;
        Target target;
        if (!resolve_chain(typer, scope, segments, count, &target)) return NULL;
        if (target.type_name) return named_type(target.type_name); // The type of a declaration, e.g 'Status' of 'a: Status ='
        return target.type;
    }

    // case .open, in the switch on a value of the type
    if (ends_with_word(text, at, "case")) {
        int depth = 0;
        for (int p = at - 5; p >= 0; p--) {
            if (text[p] == '}') depth++;
            if (text[p] == '{' && depth-- == 0) {
                int end = p;
                skip_spaces_back(text, &end);
                return type_of_chain_before(typer, file, scope_at(file, p), end);
            }
        }
        return NULL;
    }

    // return .open
    if (ends_with_word(text, at, "return")) {
        AstFunctionDefn *func = scope ? scope->enclosing_function : NULL;
        if (!func || func->return_types.count == 0) return NULL;
        return ((Type **)func->return_types.items)[0];
    }

    // f(.open), f(a, .open)
    if (c == '(' || c == ',') {
        Literal literal;
        if (c == ',' && literal_at(typer, file, dot, &literal)) return NULL; // e.g 'Point{x: 1, .' isn't a value
        return parameter_type_at(typer, file, scope, dot);
    }

    // Point{kind: .open}
    if (c == ':') {
        Literal literal;
        if (!literal_at(typer, file, dot, &literal) || !literal.field[0]) return NULL;
        AstIdentifier *member = get_struct_member(literal.node, literal.field);
        return member ? member->type : NULL;
    }
    return NULL;
}

// The values that can be written as '.name' for the type: the values of an enum and the variants of a union
static void add_values_of(Type *type) {
    type = value_type_of(type);
    if (!type) return;
    if (type->kind == TYPE_ENUM) {
        AstEnum *node = ((TypeEnum *)type)->node;
        for (int i = 0; i < node->enumerators.count; i++) {
            AstEnumerator *etor = ((AstEnumerator **)node->enumerators.items)[i];
            char value[32];
            snprintf(value, sizeof(value), "%d", etor->value);
            add_item(etor->name, "enumMember", value, NULL);
        }
    }
    if (type->kind == TYPE_STRUCT && ((TypeStruct *)type)->node->is_union) {
        AstStruct *node = ((TypeStruct *)type)->node;
        for (int i = 0; i < node->members->identifiers.count; i++) {
            AstIdentifier *variant = ((AstIdentifier **)node->members->identifiers.items)[i];
            if (is_visible_name(variant->name)) add_item(variant->name, "variant", variant->type ? type_to_str(variant->type) : "", NULL);
        }
    }
}

// Called by compile_program() after the program is checked
void complete_program(Typer *typer, DynamicArray files) {
    program_completion.items = sb_init(4096);
    program_completion.item_count = 0;
    program_completion.signature = NULL;

    AstFile *file = completion_file(files, program_completion.path);
    if (!file || !file->text) return;

    int offset = offset_of(file->text, program_completion.line, program_completion.character);
    AstBlock *scope = scope_at(file, offset);
    completing_file   = file;
    completing_offset = offset;
    typer->current_file = file;
    typer->parser->current_file = file;

    complete_signature(typer, file, scope, offset);

    // The name being written, and what is before it
    int start = offset;
    while (start > 0 && is_name_char(file->text[start - 1])) start--;

    bool after_dot = start > 0 && file->text[start - 1] == '.' && !(start > 1 && file->text[start - 2] == '.');
    if (!after_dot) {
        // The name of a field in a struct literal, e.g 'Point{x: 1, |'
        Literal literal;
        if (literal_at(typer, file, start, &literal) && !literal.in_value) {
            add_literal_fields(&literal, file->text, start);
            return;
        }
        add_names_in_scope(file, scope, start);
        return;
    }

    // A '.' with nothing before it, e.g 'status == .open', is a value of the type that is wanted there
    char before_dot = start > 1 ? file->text[start - 2] : 0;
    if (!is_name_char(before_dot) && before_dot != ')' && before_dot != ']') {
        add_values_of(expected_type_at(typer, file, scope, start - 1));
        return;
    }

    Segment segments[MAX_SEGMENTS];
    int count = read_chain(file->text, start - 1, segments);
    if (count <= 0) return;

    Target target;
    if (!resolve_chain(typer, scope, segments, count, &target)) return;
    if (target.ns)             add_namespace_declarations(target.ns, file);
    else if (target.type_name) add_static_members(target.type_name);
    else                       add_members(typer, target.type);
}

void write_completion_json(FILE *out) {
    fprintf(out, "{\"version\": 1, \"items\": [%s\n], \"signature\": %s}\n",
        program_completion.items.buffer ? program_completion.items.buffer : "",
        program_completion.signature ? program_completion.signature : "null");
}
