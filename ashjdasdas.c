#include <stdio.h>
#include <stdbool.h>

typedef struct WeirdlySizedStruct {
    bool a;
    bool b;
    bool c;
    bool d;
    bool e;
} WeirdlySizedStruct;

int main()
{
    WeirdlySizedStruct w = {0};

    printf("sizeof(WeirdlySizedStruct) = %zu\n", sizeof(WeirdlySizedStruct));

    return 0;
}
