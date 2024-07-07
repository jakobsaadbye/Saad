#include "lib/table.c"

#define SYMBOL_HASH_TABLE_LENGTH 8

typedef struct Scope Scope;

typedef struct Scope {
    Scope *parent;
    int next;       // Next child to visit on a call to dive()
    DynamicArray children;
    HashTable symbols;

    size_t bytes_allocated; // Number of bytes allocated in this scope
} Scope;

typedef struct SymbolTable {
    Scope *global_scope;
    Scope *current_scope;
} SymbolTable;