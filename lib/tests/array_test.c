#include <stdio.h>
#include "../array.c"

int main()
{
    DynamicArray numbers = da_init(2, sizeof(int));

    for (unsigned int i = 0; i < 100; i++) {
        da_append(&numbers, i);
    }

    for (unsigned int i = 0; i < numbers.count; i++) {
        int n = ((int *)(numbers.items))[i];
        printf("%d\n", n);
    }

    return 0;
}
