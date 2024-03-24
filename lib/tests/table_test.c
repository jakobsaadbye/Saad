#include "typer.c"
#include "../table.c"

bool identifier_is_equal(const void *key, const void *item) {
    const char    *name  = (const char *)   (key);
    AstIdentifier *ident = (AstIdentifier *)(item);
    return strcmp(name, ident->name) == 0;
}

int main(void)
{
    HashTable table = hash_table_make(128, identifier_is_equal);

    AstIdentifier var_a = {0};
    var_a.name = "ac";
    var_a.type = TYPE_INTEGER;

    AstIdentifier var_b = {0};
    var_b.name = "bb";
    var_b.type = TYPE_STRING;

    hash_table_add(&table, var_a.name, &var_a);
    hash_table_add(&table, var_b.name, &var_b);

    AstIdentifier *a = (AstIdentifier *)(hash_table_get(&table, "ac"));
    if (a == NULL) {
        printf("Whoopsi doopsie, undeclared variable a!\n");
        return -1;
    }
    AstIdentifier *b = (AstIdentifier *)(hash_table_get(&table, "bb"));
    if (b == NULL) {
        printf("Whoopsi doopsie! undeclared variable b\n");
        return -1;
    }

    printf("'%s' is of type %s\n", a->name, type_kind_to_str(a->type));
    printf("'%s' is of type %s\n", b->name, type_kind_to_str(b->type));

    return 0;
}
