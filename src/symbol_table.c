#include <assert.h>
#include "ast.c"

typedef struct Symbol {
    AstType type;
    char *name;
    union {
        AstIdentifier   *identifier;
        AstFunctionDefn *function_defn;
        AstStruct       *struct_defn;
        AstDeclaration  *struct_member;
        AstEnum         *enum_defn;
    } as;
} Symbol;

bool compare_symbol(const void *key, const void *item) {
    Symbol *symbol = (Symbol *)(item);
    return strcmp((const char *)(key), symbol->name) == 0;
}

SymbolTable symbol_table_init() {
    SymbolTable st = {0};
    Scope *global_scope = (Scope *)(malloc(sizeof(Scope)));
    global_scope->parent    = NULL;
    global_scope->children  = da_init(2, sizeof(Scope));
    global_scope->symbols   = hash_table_init(SYMBOL_HASH_TABLE_LENGTH, sizeof(Symbol), compare_symbol);
    global_scope->next      = 0;
    st.current_scope = global_scope;
    st.global_scope  = global_scope;

    return st;
}

void enter_scope(SymbolTable *st) {
    int next_child = st->current_scope->next;
    Scope *child   = &((Scope *)(st->current_scope->children.items))[next_child];
    assert(child != NULL);

    st->current_scope->next += 1;
    st->current_scope        = child;
}

void exit_scope(SymbolTable *st) {
    Scope *parent = st->current_scope->parent;
    assert(parent != NULL);

    st->current_scope = parent;
}

void symbol_table_reset_helper(SymbolTable *st, Scope *current) {
    current->next = 0;

    for (unsigned int i = 0; i < current->children.count; i++) {
        Scope *next = &((Scope *)(current->children.items))[i];
        assert(next != NULL);
        symbol_table_reset_helper(st, next);
    }
}

void symbol_table_reset(SymbolTable *st) {
    symbol_table_reset_helper(st, st->current_scope);
}

void open_scope(SymbolTable *st) {
    Scope *parent = st->current_scope;
    Scope child    = {0};
    child.parent   = parent;
    child.children = da_init(2, sizeof(Scope));
    child.symbols  = hash_table_init(SYMBOL_HASH_TABLE_LENGTH, sizeof(Symbol), compare_symbol);

    da_append(&parent->children, child);
    st->current_scope = &((Scope *)(parent->children.items))[parent->children.count - 1];
}

void close_scope(SymbolTable *st) {
    st->current_scope = st->current_scope->parent;
}

Symbol *symbol_lookup(SymbolTable *st, char *name) {
    Scope *searching_scope = st->current_scope;
    Symbol *symbol = NULL;
    while (true) {
        symbol = (Symbol *) hash_table_get(&searching_scope->symbols, name);
        if (symbol) break;
        
        searching_scope = searching_scope->parent;
        if (searching_scope == NULL) {
            // We've hit global scope without any success. Just return in shame
            return NULL;
        }
    }

    return symbol;
}

DynamicArray symbol_get_symbols(SymbolTable *st) {
    DynamicArray symbols = hash_table_get_items(&st->current_scope->symbols);
    return symbols;
}

Symbol *symbol_add(SymbolTable *st, Symbol symbol) {
    Symbol *existing = (Symbol *)(hash_table_get(&st->current_scope->symbols, symbol.name));
    if (existing) {
        return existing;
    }

    hash_table_add(&st->current_scope->symbols, symbol.name, &symbol);
    return NULL;
}

Symbol *symbol_add_identifier(SymbolTable *st, AstIdentifier *identifier) {
    Symbol symbol = {0};
    symbol.type = AST_IDENTIFIER;
    symbol.name = identifier->name;
    symbol.as.identifier = identifier;

    return symbol_add(st, symbol);
}

Symbol *symbol_add_function_defn(SymbolTable *st, AstFunctionDefn *func_defn) {
    Symbol symbol = {0};
    symbol.type = AST_FUNCTION_DEFN;
    symbol.name = func_defn->identifier->name;
    symbol.as.function_defn = func_defn;

    return symbol_add(st, symbol);
}

Symbol *symbol_add_enum(SymbolTable *st, AstEnum *enum_defn) {
    Symbol symbol = {0};
    symbol.type = AST_ENUM;
    symbol.name = enum_defn->identifier->name;
    symbol.as.enum_defn = enum_defn;

    return symbol_add(st, symbol);
}

Symbol *symbol_add_struct(SymbolTable *st, AstStruct *struct_defn) {
    Symbol symbol = {0};
    symbol.type = AST_STRUCT;
    symbol.name = struct_defn->identifier->name;
    symbol.as.struct_defn = struct_defn;

    return symbol_add(st, symbol);
}

Symbol *symbol_add_struct_member(SymbolTable *st, AstDeclaration *member) {
    Symbol symbol = {0};
    symbol.type = AST_DECLARATION;
    symbol.name = member->identifier->name;
    symbol.as.struct_member = member;

    return symbol_add(st, symbol);
}