#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define da_append(da, item) _da_append(da, &item);

typedef struct DynamicArray {
    int item_size;
    unsigned char *items;
    int count;
    int capacity;
} DynamicArray;

DynamicArray da_init(unsigned int init_cap, unsigned int item_size);
void _da_append(DynamicArray *da, void *item);
void da_free(DynamicArray *da);
void da_remove(DynamicArray *da, int index);

#endif 

#ifdef DYNAMIC_ARRAY_IMPLEMENTATION

DynamicArray da_init(unsigned int init_cap, unsigned int item_size) {
    DynamicArray da = {0};
    da.capacity = init_cap;
    da.item_size = item_size;
    da.count = 0;
    da.items = (unsigned char *) malloc(item_size * init_cap);
    if (da.items == NULL) {
        printf("Buy more ram!\n");
        exit(1);
    }
    memset(da.items, 0, init_cap);

    return da;
}

void _da_append(DynamicArray *da, void *item) {
    if (da->count + 1 > da->capacity) {
        // Reallocate memory
        da->items = realloc(da->items, da->capacity * 2 * da->item_size);
        if (da->items == NULL) {
            printf("Buy more ram!\n");
            exit(1);
        }
        da->capacity *= 2;
    }

    void *dst = da->items + (da->count * da->item_size);
    memcpy(dst, item, da->item_size);
    da->count += 1;
}

void da_remove(DynamicArray *da, int index) {
    if (index == -1) index = da->count - 1; // Special case to get last element
    if (index >= da->count || index < 0) return;


    // Shift over all the bytes by one item size
    unsigned char *ptr   = da->items + (index * da->item_size);

    if (index == da->count - 1) {
        // Removing the last element, we can simply NULL it without doing any shifting on the rest of the array
        memset(ptr, 0, da->item_size);
        da->count -= 1;
    } else {
        int after_index = index + 1;
        unsigned char *after = da->items + (after_index * da->item_size);
        memcpy(ptr, after, (da->count - after_index) * da->item_size);
        unsigned char *last_item = da->items + ((da->count - 1) * da->item_size);
        memset(last_item, 0, da->item_size);
        da->count -= 1;
    }
}

void da_free(DynamicArray *da) {
    free(da->items);
    da->count = 0;
    da->capacity = 0;
    da->item_size = 0;
    da->items = NULL;
}

#endif
