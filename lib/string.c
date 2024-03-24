#include <stdarg.h>
#include "stdlib.h"
#include "strings.h"

#define MAX_STRING_LEN 2048

typedef struct StringBuilder {
    char   *buffer;
    size_t  cursor;
    size_t  capacity;
} StringBuilder;


StringBuilder string_builder_make(size_t capicity);
void sb_append(StringBuilder *sb, const char *string, ...);
void sb_free(StringBuilder *sb);
char *sb_to_string(StringBuilder *sb);


StringBuilder string_builder_make(size_t capicity) {
    char *buffer = (char *)(malloc(capicity));
    memset(buffer, '\0', capicity);

    return (StringBuilder) {
        .buffer  = buffer,
        .cursor   = 0,
        .capacity = capicity
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

void sb_free(StringBuilder *sb) {
    free(sb->buffer);
}

char *sb_to_string(StringBuilder *sb) {
    return sb->buffer;
}