#ifndef SCOPE_H
#define SCOPE_H

#include "parser.h"

AstBlock *new_block(Parser *parser, BlockKind kind);
void      close_block(Parser *parser);

AstIdentifier  *find_in_scope(AstBlock *scope, char *ident_name);
AstDeclaration *find_member_in_scope(AstBlock *scope, char *name);
AstIdentifier  *lookup_from_scope(Parser *parser, AstBlock *scope, char *ident_name, Ast *used_at);
AstIdentifier  *add_identifier_to_scope(Parser *parser, AstBlock *scope, AstIdentifier *ident);
AstDeclaration *add_declaration_to_scope(AstBlock *scope, AstDeclaration *decl);

#endif
