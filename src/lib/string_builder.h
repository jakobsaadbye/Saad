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


StringBuilder sb_init(size_t init_capicity);
void sb_append(StringBuilder *sb, const char *string, ...);
void sb_free(StringBuilder *sb);
char *sb_to_string(StringBuilder *sb);

StringBuilder sb_init(size_t init_capicity);
StringBuilder sb_init(size_t init_capicity);
void sb_append(StringBuilder *sb, const char *template, ...);
void sb_copy(StringBuilder *sb, char *src, int length);
void sb_copy_string(StringBuilder *sb, char *src);
void sb_free(StringBuilder *sb);
char *sb_to_string(StringBuilder *sb);

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

void sb_append(StringBuilder *sb, const char *template, ...) {
    va_list args;
    va_start(args, template);

    char *string = (char *)(malloc(MAX_STRING_LEN));
    int   string_len = vsprintf(string, template, args);
    va_end(args);

    if ((sb->cursor + string_len) < sb->capacity) {
        char *dst = &sb->buffer[sb->cursor];
        memcpy(dst, string, string_len);
        sb->cursor += string_len;

        free(string);
        return;
    } else {
        size_t new_capacity = sb->capacity;
        while (new_capacity <= sb->cursor + string_len) {
            new_capacity = new_capacity * 2;
        }

        char *new_buffer = (char *)(realloc(sb->buffer, new_capacity));
        if (new_buffer == NULL) {
            printf("%s:%d: error: Buy more ram!", __FILE__, __LINE__);
            exit(1);
        }

        sb->buffer   = new_buffer;
        sb->capacity = new_capacity;

        memset(&sb->buffer[sb->cursor], '\0', new_capacity - sb->cursor);

        // Append the string
        char *dst = &sb->buffer[sb->cursor];
        memcpy(dst, string, string_len);
        sb->cursor += string_len;

        free(string);
    }
}

// Copies length amount of characters from src to the builder
void sb_copy(StringBuilder *sb, char *src, int length) {
    if (length == 0) return;
    
    if ((sb->cursor + length) < sb->capacity) {
        char *dst = &sb->buffer[sb->cursor];
        memcpy(dst, src, length);
        sb->cursor += length;

        return;
    } else {
        size_t new_capacity = sb->capacity;
        while (new_capacity <= sb->cursor + length) {
            new_capacity = new_capacity * 2;
        }

        char *new_buffer = (char *)(realloc(sb->buffer, new_capacity));
        if (new_buffer == NULL) {
            printf("%s:%d: error: Buy more ram!", __FILE__, __LINE__);
            exit(1);
        }

        sb->buffer   = new_buffer;
        sb->capacity = new_capacity;

        memset(&sb->buffer[sb->cursor], '\0', new_capacity - sb->cursor);

        char *dst = &sb->buffer[sb->cursor];
        memcpy(dst, src, length);
        sb->cursor += length;
    }
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