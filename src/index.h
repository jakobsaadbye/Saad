#ifndef INDEX_H
#define INDEX_H

//
// The index of a program, for editors: where every name is declared, and where it is used. 'saad index' writes it as JSON,
// see docs/lsp.md. The typer adds a use with index_*_use() where it finds out what a name refers to
//

#include "ast.h"

typedef struct IndexSymbol {
    AstFile    *file;
    Pos         name_start;  // The name in the declaration
    Pos         name_end;
    Pos         start;       // The whole declaration e.g the function with its body
    Pos         end;
    char       *name;
    const char *kind;        // "function", "method", "struct", "union", "enum", "enumMember", "field", "variant", "constant",
                             // "variable", "global", "parameter", "typeAlias" or "typeParameter"
    char       *detail;      // e.g the signature of a function or the type of a variable
    int         container;   // The symbol it is in e.g the struct of a member, or -1
} IndexSymbol;

typedef struct IndexReference {
    AstFile *file;
    Pos      start;
    Pos      end;
    int      symbol;
} IndexReference;

typedef struct Index {
    bool         enabled;    // Only 'saad index' records anything
    bool         with_std;   // Also write the uses in the standard library, and its declarations that the program doesn't use: 'saad index --std'
    char        *std_path;   // Absolute path of the standard library
    DynamicArray symbols;    // of IndexSymbol
    DynamicArray references; // of IndexReference
} Index;

extern Index program_index;

void index_identifier_use(Ast *site, AstIdentifier *ident);
void index_name_use(Ast *site, Pos start, Pos end, AstIdentifier *ident);
void index_enumerator_use(Ast *site, AstEnumerator *etor);
int  index_identifier(AstIdentifier *ident);
void index_namespace_use(Ast *site, Pos start, Pos end, Namespace *ns, const char *spelled);

#endif
