#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdarg.h>
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

#endif