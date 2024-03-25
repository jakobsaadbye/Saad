#include <assert.h>
#include "ast.c"
#include "lib/table.c"

#define SYMBOL_HASH_TABLE_LENGTH 8

typedef struct Scope Scope;

typedef struct Scope {
    Scope *parent;
    DynamicArray children;
    HashTable symbols;
} Scope;

typedef struct SymbolTable {
    Scope *current_scope;
} SymbolTable;

typedef struct Symbol {
    AstType type;
    char *name;
    union as_value {
        AstIdentifier   *identifier;
        AstFunctionDefn *function_defn;
    } as_value;
} Symbol;

bool compare_symbol(const void *key, const void *item) {
    Symbol *symbol = (Symbol *)(item);
    return strcmp((const char *)(key), symbol->name) == 0;
}

SymbolTable symbol_table_init() {
    SymbolTable st = {0};
    Scope *global_scope = (Scope *)(malloc(sizeof(Scope)));;
    global_scope->parent = NULL;
    global_scope->children = da_init(2, sizeof(Scope));
    global_scope->symbols = hash_table_init(SYMBOL_HASH_TABLE_LENGTH, sizeof(Symbol), compare_symbol);
    st.current_scope = global_scope;

    return st;
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
    symbol.as_value.identifier = identifier;

    return symbol_add(st, symbol);
}