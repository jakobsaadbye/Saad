#include <stdio.h>
#include <stdbool.h>

typedef struct Node {
    Node *next;
    int data;
} Node;

int main() {
    Node n1 = {0};
    n1.data = 69;

    Node *ptr0 = &n1;
    Node **ptr1 = &ptr0;

    printf("%d", ptr1->data);
    return 0;
}
