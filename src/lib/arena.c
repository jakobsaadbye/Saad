#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Arena {
    unsigned char *data;
    size_t cursor;
    size_t capacity;
} Arena;

Arena arena_make(size_t capacity) {
    unsigned char *data = (unsigned char*) malloc(capacity);
    
    Arena arena    = {0};
    arena.data     = data;
    arena.capacity = capacity;
    arena.cursor   = 0;

    return arena;
}

void *arena_allocate(Arena *arena, size_t size) {
    if (arena->cursor + size < arena->capacity) {
        void *ptr = &arena->data[arena->cursor];
        arena->cursor += size;
        memset(ptr, 0, size);
        return ptr;
    }

    return NULL;
}

void arena_free(Arena *arena) {
    free(arena->data);
}