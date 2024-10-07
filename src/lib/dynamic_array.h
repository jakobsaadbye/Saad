#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#define da_append(da, item) _da_append(da, &item);

typedef struct DynamicArray {
    int item_size;
    unsigned char *items;
    int count;
    int capacity;
} DynamicArray;

DynamicArray da_init(unsigned int init_cap, unsigned int item_size);
void _da_append(DynamicArray *da, void *item);

#endif