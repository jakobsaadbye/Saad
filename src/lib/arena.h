#ifndef ARENA_H
#define ARENA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Chunk Chunk;
typedef struct Chunk {
    unsigned char *data;
    size_t cursor;
    size_t capacity;
    Chunk *prev;
} Chunk;

typedef struct Arena {
    Chunk *current_chunk;
} Arena;

Chunk *make_new_chunk(Chunk *old_chunk, size_t capacity);
Arena arena_init(size_t capacity);
void *arena_allocate(Arena *arena, size_t size);
void arena_free_all(Arena *arena);

#endif