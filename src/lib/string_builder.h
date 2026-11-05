#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdarg.h>
#include <stdio.h>
#include "stdlib.h"
#include "strings.h"

#define MAX_STRING_LEN 2048

typedef struct StringBuilder {
    char   *buffer;
    size_t  cursor;
    size_t  capacity;
} StringBuilder;



StringBuilder  sb_init(size_t init_capicity);
StringBuilder *sb_new(int init_capicity);
void           sb_append(StringBuilder *sb, const char *template, ...);
void           sb_vappend(StringBuilder *sb, const char *template, va_list args);
void           sb_copy(StringBuilder *sb, char *src, int length);
void           sb_copy_string(StringBuilder *sb, char *src);
void           sb_free(StringBuilder *sb);
char          *sb_to_string(StringBuilder *sb);



#endif // STRING_BUILDER_H


#ifdef STRING_BUILDER_IMPLEMENTATION

StringBuilder sb_init(size_t init_capicity) {
    char *buffer = (char *)(malloc(init_capicity));
    memset(buffer, '\0', init_capicity);

    return (StringBuilder) {
        .buffer  = buffer,
        .cursor   = 0,
        .capacity = init_capicity
    };
}

StringBuilder *sb_new(int init_capicity) {
    StringBuilder sb_temp = sb_init(init_capicity);
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    *sb = sb_temp;
    return sb;
}

// Makes room for length more characters and the 0 after them. The free part of the buffer is kept 0
static void sb_reserve(StringBuilder *sb, size_t length) {
    if (sb->cursor + length < sb->capacity) return;
    size_t new_capacity = sb->capacity;
    while (new_capacity <= sb->cursor + length) {
        new_capacity = new_capacity * 2;
    }

    char *new_buffer = (char *)(realloc(sb->buffer, new_capacity));
    if (new_buffer == NULL) {
        printf("%s:%d: error: Buy more ram!", __FILE__, __LINE__);
        exit(1);
    }
    memset(&new_buffer[sb->capacity], '\0', new_capacity - sb->capacity);
    sb->buffer   = new_buffer;
    sb->capacity = new_capacity;
}

void sb_vappend(StringBuilder *sb, const char *template, va_list args) {
    // Formats into the free part of the buffer, and only formats again when it didn't fit
    va_list again;
    va_copy(again, args);
    size_t free_space = sb->capacity - sb->cursor;
    int string_len = vsnprintf(&sb->buffer[sb->cursor], free_space, template, args);
    if (string_len >= 0 && (size_t)string_len >= free_space) {
        sb_reserve(sb, string_len);
        vsnprintf(&sb->buffer[sb->cursor], string_len + 1, template, again);
    }
    va_end(again);
    if (string_len > 0) sb->cursor += string_len;
}

void sb_append(StringBuilder *sb, const char *template, ...) {
    va_list args;
    va_start(args, template);
    sb_vappend(sb, template, args);
    va_end(args);
}

// Copies length amount of characters from src to the builder
void sb_copy(StringBuilder *sb, char *src, int length) {
    if (length <= 0) return;
    sb_reserve(sb, length);
    memcpy(&sb->buffer[sb->cursor], src, length);
    sb->cursor += length;
}

// Copies the null-terminated src string to the builder
void sb_copy_string(StringBuilder *sb, char *src) {
    int length = strlen(src);
    sb_copy(sb, src, length);
}

void sb_free(StringBuilder *sb) {
    free(sb->buffer);
}

char *sb_to_string(StringBuilder *sb) {
    return sb->buffer;
}

#endif // STRING_BUILDER_IMPLEMENTATION
