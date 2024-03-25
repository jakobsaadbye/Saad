#include "stdbool.h"
#include "string.h"
#include "stdlib.h"

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
LinkedList linked_list_make();
void       linked_list_add(LinkedList *ll, void *item, size_t item_size);
void      *linked_list_find(LinkedList *ll, const char *key, bool (*cmp_func)(const void* key, const void* item));

HashTable hash_table_init(size_t table_length, size_t item_size, bool (*cmp_func)(const void* key, const void* item)) {
    HashTable table    = {0};
    table.cmp_func     = cmp_func;
    table.table_length = table_length;
    table.item_size    = item_size;
    table.items        = (LinkedList *)(malloc(sizeof(LinkedList) * table_length));
    for (size_t i = 0; i < table_length; i++) {
        table.items[i] = linked_list_make();
    }
    return table;
}

void hash_table_add(HashTable *table, const char *key, void *item) {
    #ifdef DEBUG
    printf("Added '%s' to identifier table\n", key);
    #endif

    int index = hash(table, key);

    LinkedList *ll = &table->items[index];
    linked_list_add(ll, item, table->item_size);
    table->num_of_items += 1;
}

void *hash_table_get(HashTable *table, const char *key) {
    int index = hash(table, key);
    
    LinkedList *ll = &table->items[index];
    if (ll->length == 0) return NULL;

    return linked_list_find(ll, key, table->cmp_func);
}

int hash(HashTable *table, const char *key) {
    const char *c = key;
    int sum = 0;
    while (*c != '\0') {
        sum += (int)(*c);
        c++;
    }

    return sum % table->table_length;
}

LinkedList linked_list_make() {
    LinkedList ll = {0};
    ll.head = NULL;
    ll.tail = NULL;
    ll.length = 0;
    return ll;
}

void linked_list_add(LinkedList *ll, void *item, size_t item_size) {
    if (ll->head == NULL) {
        Node *node =  (Node *)(malloc(sizeof(Node)));
        node->next = NULL;
        node->data = malloc(item_size);
        memcpy(node->data, item, item_size);

        ll->head = node;
        ll->tail = node;
        ll->length++;
        return;
    }

    Node *node     = (Node *)(malloc(sizeof(Node)));
    node->next     = NULL;
    node->data = malloc(item_size);
    memcpy(node->data, item, item_size);

    ll->tail->next = node;
    ll->tail       = node;
    ll->length++;
}

void *linked_list_find(LinkedList *ll, const char *key, bool (*cmp_func)(const void* key, const void* item)) {
    Node *node = ll->head;
    bool found = cmp_func(key, node->data);
    if (found) return node->data;
    else {
        while (true) {
            if (node->next == NULL) return NULL;    // We didn't find the thing

            node  = node->next;
            found = cmp_func(key, node->data);
            if (found) break;
        }
        return node->data;
    }
}







