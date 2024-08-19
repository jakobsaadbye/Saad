#include <stdio.h>

int main(int argc, char const *argv[])
{
    unsigned char x = 255;
    x += 2;

    printf("x=%llu", x);
    
    return 0;
}
