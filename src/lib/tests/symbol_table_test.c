#include "symbol_table.c"

/* 
main :: ()  {
    a := 2.0;
    {
        b := 3; 
        {
            b := 5.0;
            print(b);   // Should find the b declared above as a float
        }
        {
            a := 5;
            print(a); // Should find the a declared above as an int
        }
        print(b); // Should find the b declared at the top an int
        print(a); // Should find the a declared at the outer most scope as a float
    }
    print(b) // This b should not exist!
}
*/

int main()
{
    SymbolTable st = symbol_table_init();

    open_scope(&st);
        AstIdentifier *a_float = (AstIdentifier *)(malloc(sizeof(AstIdentifier)));
        a_float->name = "a";
        a_float->type = TYPE_FLOAT;
        symbol_add_identifier(&st, a_float);

        open_scope(&st);
            AstIdentifier *b_int = (AstIdentifier *)(malloc(sizeof(AstIdentifier)));
            b_int->name = "b";
            b_int->type = TYPE_INTEGER;
            symbol_add_identifier(&st, b_int);

            open_scope(&st);
                AstIdentifier *b_float = (AstIdentifier *)(malloc(sizeof(AstIdentifier)));
                b_float->name = "b";
                b_float->type = TYPE_FLOAT;
                symbol_add_identifier(&st, b_float);

                Symbol *b = symbol_lookup(&st, "b");
                assert(b != NULL);
                assert(b->as_value.value.identifier->type == TYPE_FLOAT);
            close_scope(&st);

            open_scope(&st);
                AstIdentifier *a_int = (AstIdentifier *)(malloc(sizeof(AstIdentifier)));
                a_int->name = "a";
                a_int->type = TYPE_INTEGER;
                symbol_add_identifier(&st, a_int);

                Symbol *a = symbol_lookup(&st, "a");
                assert(a != NULL);
                assert(a->as_value.identifier->type == TYPE_INTEGER);
            close_scope(&st);
            b = symbol_lookup(&st, "b");
            assert(b != NULL);
            assert(b->as_value.identifier->type == TYPE_INTEGER);
            a = symbol_lookup(&st, "a");
            assert(a != NULL);
            assert(a->as_value.identifier->type == TYPE_FLOAT);
        close_scope(&st);

        b = symbol_lookup(&st, "b");
        assert(b == NULL);
    close_scope(&st);

    printf("Success!!!\n");

    return 0;
}
