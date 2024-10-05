#include <stdio.h>
#include "../dynamic_array.c"

int main()
{
    DynamicArray numbers = da_init(2, sizeof(int));

    for (int i = 0; i < 100; i++) {
        da_append(&numbers, i);
    }

    for (int i = 0; i < numbers.count; i++) {
        int n = ((int *)(numbers.items))[i];
        printf("%d\n", n);
    }

    return 0;
}
