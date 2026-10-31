//////////////////////////////////
// Runtime support file that is linked with the user program executable
//
// The type descriptors of reflection (see runtime.sd) are constants that the compiler generates, so they are not here
//////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct RawString {
    char   *data;
    int64_t len;
} RawString;

int runtime_compare_strings(RawString *str1, RawString *str2);
void runtime_builtin_assert(bool condition, RawString filename, uint64_t linenumber);

int runtime_compare_strings(RawString *str1, RawString *str2) {
    if (str1->len == str2->len) {
        for (int i = 0; i < str1->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return 0;
    }

    if (str1->len < str2->len) {
        for (int i = 0; i < str1->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return -1;
    } else {
        for (int i = 0; i < str2->len; i++) {
            char c1 = str1->data[i];
            char c2 = str2->data[i];

            if (c1 == c2) continue;
            if (c1 < c2) return -1;
            if (c1 > c2) return +1;
        }

        return +1;

    }
}

void runtime_builtin_assert(bool condition, RawString filename, uint64_t linenumber) {
    if (!condition) {
        printf("%s:%zu Assertion failed!\n", filename.data, linenumber);
        exit(EXIT_FAILURE);
    }
}