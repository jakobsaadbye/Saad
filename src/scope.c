#include "scope.h"

int global_scope_counter = 0; // Only for debug purposes

AstBlock *new_block(Parser *parser, BlockKind kind) {
    AstBlock *scope    = ast_allocate(parser, sizeof(AstBlock));
    scope->head.kind   = AST_BLOCK;
    scope->head.file   = parser->current_file;
    scope->parent      = parser->current_scope;
    scope->kind        = kind;
    scope->statements  = da_init(4, sizeof(Ast *));
    scope->identifiers = da_init(4, sizeof(AstIdentifier *));
    scope->members     = da_init(4, sizeof(AstDeclaration *));
    scope->belongs_to_struct = NULL;
    scope->scope_number      = global_scope_counter++;

    parser->current_scope = scope;
    return scope;
}

void close_block(Parser *parser) {
    parser->current_scope = parser->current_scope->parent;
}

AstIdentifier *find_in_scope(AstBlock *scope, char *ident_name) {
    AstIdentifier *found = NULL;
    for (int i = 0; i < scope->identifiers.count; i++) {
        AstIdentifier *ident = ((AstIdentifier **)scope->identifiers.items)[i];

        if (strcmp(ident->name, ident_name) == 0) {
            found = ident;
            break;
        }
    }
    return found;
}

AstDeclaration *find_member_in_scope(AstBlock *scope, char *name) {
    for (int i = 0; i < scope->members.count; i++) {
        AstDeclaration *member = ((AstDeclaration **)scope->members.items)[i];
        if (strcmp(name, member->ident->name) == 0) {
            return member;
        }
    }

    return NULL;
}

bool is_a_before_b (Ast *a, Ast *b) {
    if (a->file != b->file) {
        // Consider them not to be equivalent and therefore none of them is "before" the other
        return false;
    }
    if (a->start.input_idx < b->start.input_idx) return true;
    return false;
}

bool is_declared_before_used(Ast *declared_at, Ast *used_at) {
    if (used_at == NULL || used_at->kind == AST_ERR) {
        return false;
    }

    if (is_a_before_b(used_at, declared_at)) {
        return false;
    }

    return true;
}

AstIdentifier *lookup_from_scope(Parser *parser, AstBlock *scope, char *ident_name) {
    (void)parser;

    AstBlock *searching_scope = scope;
    while (true) {
        if (searching_scope == NULL) return NULL;

        AstIdentifier *found = find_in_scope(searching_scope, ident_name);
        if (found) {
            return found;
        }

        searching_scope = searching_scope->parent;
    }
}

AstIdentifier *add_identifier_to_scope(Parser *parser, AstBlock *scope, AstIdentifier *ident) {
    AstIdentifier *existing = find_in_scope(scope, ident->name);
    if (existing) {
        if (!is_declared_before_used((Ast *)existing, (Ast *)ident)) {
            report_error_ast(parser, LABEL_ERROR, (Ast *)ident, "Identifier '%s' is being used before its being declared", ident->name);
            report_error_ast(parser, LABEL_NOTE, (Ast *)existing, "... first being declared here");
            return existing;
        }
        return existing;
    }

    da_append(&scope->identifiers, ident);

    return NULL;
}

AstDeclaration *add_declaration_to_scope(AstBlock *scope, AstDeclaration *decl) {
    AstDeclaration *existing = find_member_in_scope(scope, decl->ident->name);
    if (existing) {
        assert(existing);
        return existing;
    };

    decl->member_index = scope->members.count;
    da_append(&scope->members, decl);

    return NULL;
}