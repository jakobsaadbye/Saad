#ifndef TABLE_H
#define TABLE_H

#include "stdbool.h"
#include <stdlib.h>

typedef struct Node Node;

typedef struct Node {
    Node *next;
    void *data;
} Node;

typedef struct LinkedList {
    size_t length;
    Node *head;
    Node *tail;
} LinkedList;

typedef struct HashTable {
    LinkedList *items;
    size_t      item_size;
    size_t      table_length;
    size_t      num_of_items;

    bool (*cmp_func)(const void* key, const void* item);
} HashTable;

HashTable  hash_table_init(size_t table_length, size_t item_size, bool (*cmp_func)(const void* key, const void* item));
void       hash_table_add(HashTable *table, const char *key, void *item);
void      *hash_table_get(HashTable *table, const char *key);
int        hash(HashTable *table, const char *key);
LinkedList linked_list_make(void);
void       linked_list_add(LinkedList *ll, void *item, size_t item_size);
Node      *linked_list_get(LinkedList *ll, size_t index);
void      *linked_list_find(LinkedList *ll, const char *key, bool (*cmp_func)(const void* key, const void* item));

#endif