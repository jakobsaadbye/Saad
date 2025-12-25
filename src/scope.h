#ifndef SCOPE_H
#define SCOPE_H

#include "parser.h"

AstBlock *new_block(Parser *parser, BlockKind kind);
void      close_block(Parser *parser);

AstIdentifier  *find_identifier_in_scope(AstBlock *scope, char *ident_name);
AstIdentifier  *lookup_from_scope(Parser *parser, AstBlock *scope, char *ident_name);
AstIdentifier  *add_identifier_to_scope(Parser *parser, AstBlock *scope, AstIdentifier *ident);
AstIdentifier  *add_declaration_to_scope(Parser *parser, AstBlock *scope, AstDeclaration *decl);

bool is_a_before_b (Ast *a, Ast *b);

#endif
