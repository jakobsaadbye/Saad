#include "arena.h"

Chunk *make_new_chunk(Chunk *old_chunk, size_t capacity) {
    Chunk *new_chunk = malloc(sizeof(Chunk));

    new_chunk->data = (unsigned char*) malloc(capacity);
    new_chunk->cursor = 0;
    new_chunk->capacity = capacity;
    new_chunk->prev = old_chunk;

    return new_chunk;
}

Arena arena_init(size_t capacity) {
    Arena arena = {0};
    arena.current_chunk = make_new_chunk(NULL, capacity);

    return arena;
}

void *arena_allocate(Arena *arena, size_t size) {
    Chunk *current_chunk = arena->current_chunk;
    if ((current_chunk->cursor + size) < current_chunk->capacity) {
        void *ptr = &current_chunk->data[current_chunk->cursor];
        current_chunk->cursor += size;
        memset(ptr, 0, size);
        return ptr;
    }

    size_t new_capacity = current_chunk->capacity * 2;
    while (new_capacity < current_chunk->cursor + size) {
        new_capacity *= 2;
    }

    Chunk *new_chunk = make_new_chunk(current_chunk, new_capacity);
    arena->current_chunk = new_chunk;

    return arena_allocate(arena, size);
}

void arena_free_all(Arena *arena) {
    Chunk *chunk = arena->current_chunk;
    while(chunk) {
        free(chunk->data);
        Chunk *temp = chunk;
        chunk = chunk->prev;
        free(temp);
    }
}