#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "lib/arena.h"

#define MAX_TOKENS 1024
#define MAX_DIGITS 32
#define IDENT_NAME_BUFFER_SIZE (size_t)1024

#define XXX assert(false && "Not implemented")

#define U8_MAX  255U
#define U16_MAX 65535U
#define U32_MAX 4294967295UL
#define U64_MAX 18446744073709551615ULL
#define S8_MAX  127
#define S16_MAX 32767
#define S32_MAX 2147483647
#define S64_MAX 9223372036854775807

#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[91m"
#define COLOR_MAGENTA "\x1B[95m"
#define COLOR_ICE     "\x1B[96m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_WHITE_BOLD  "\x1B[1;37m"

extern const char *LABEL_NOTE;
extern const char *LABEL_ERROR;
extern const char *LABEL_WARNING;

typedef enum TokenType {
    TOKEN_NONE      = 0,

    // Mentally insert ASCII values here ...

    TOKEN_INTEGER    = 128,
    TOKEN_FLOAT      = 129,
    TOKEN_STRING     = 130,
    TOKEN_IDENTIFIER = 131,
    TOKEN_BOOLEAN    = 132,
    TOKEN_FALSE      = 133,
    TOKEN_TRUE       = 134,
    TOKEN_NIL        = 135,

    TOKEN_LOGICAL_OR    = 140,
    TOKEN_LOGICAL_AND   = 141,
    TOKEN_LESS_EQUAL    = 142,
    TOKEN_GREATER_EQUAL = 143,
    TOKEN_DOUBLE_EQUAL  = 144,
    TOKEN_NOT_EQUAL     = 145,

    TOKEN_PLUS_EQUAL   = 146,
    TOKEN_MINUS_EQUAL  = 147,
    TOKEN_TIMES_EQUAL  = 148,
    TOKEN_DIVIDE_EQUAL = 149,

    TOKEN_DOUBLE_COLON = 150,
    TOKEN_COLON_EQUAL  = 151,
    TOKEN_RIGHT_ARROW  = 152,
    TOKEN_DOUBLE_DOT   = 153,
    TOKEN_TRIPLE_DOT   = 154,
    TOKEN_IN           = 155,

    TOKEN_PRINT        = 160,
    TOKEN_ASSERT       = 161,
    TOKEN_IF           = 162,
    TOKEN_ELSE         = 163,
    TOKEN_RETURN       = 164,
    TOKEN_BREAK        = 165,
    TOKEN_CONTINUE     = 166,
    TOKEN_FOR          = 167,
    TOKEN_TYPEOF       = 168,

    TOKEN_STRUCT  = 180,
    TOKEN_ENUM    = 181,

    // @Important!!! - Order should follow PrimitiveKind order
    TOKEN_TYPE_INT = 185,
    TOKEN_TYPE_U8,
    TOKEN_TYPE_U16,
    TOKEN_TYPE_U32,
    TOKEN_TYPE_U64,
    TOKEN_TYPE_S8,
    TOKEN_TYPE_S16,
    TOKEN_TYPE_S32,
    TOKEN_TYPE_S64,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_F32,
    TOKEN_TYPE_F64,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_BOOL, 
    TOKEN_TYPE_VOID, // @Important!!! - void must be the final type token
    
    TOKEN_END = 254
} TokenType;

typedef enum ValueFlags {
    VALUE_IS_BIGGER_THAN_SIGNED_64_BIT_INTEGER = 1 << 0
} ValueFlags;

typedef struct As_value {
    ValueFlags flags;
    union {
        bool               boolean;
        long long          integer;
        double             floating;
        struct { char *data; int length; } string;
        struct { char *name; int length; } identifier;
    } value;
} As_value;

typedef struct Line {
    char *str;
    int start_idx;
    int end_idx;
} Line;

typedef struct Pos {
    int input_idx;
    int line;
    int col;
} Pos;

typedef struct Token {
    TokenType type;

    As_value as_value;

    Pos start;
    Pos end;
} Token;

typedef struct Lexer {
    char *input_str;
    const char *file_path;

    Token tokens[MAX_TOKENS]; // @Cleanup - Turn into a dynamic array
    unsigned int token_index_cursor;

    Arena identifier_names;

    int char_idx;
    int line_number;
    int col;
} Lexer;

typedef struct KeywordMatch {
    int       length;
    TokenType token;
    bool      matched;
} KeywordMatch;


Lexer lexer_init(char *input_str, const char *file_path);
bool lex(Lexer *lexer);
void eat_character(Lexer *lexer);
void eat_characters(Lexer *lexer, int chars_to_eat);
char peek_char(Lexer *lexer, int lookahead);
char peek_next_char(Lexer *lexer);
void push_token(Lexer *lexer, Token token);
Pos get_current_position(Lexer *lexer);
char *token_type_to_str(TokenType token_type);
void make_keyword(Lexer *lexer, TokenType kw_type, Pos start);
void make_literal_here(Lexer *lexer, TokenType token_type, Pos start);
void make_literal(Lexer *lexer, TokenType token_type, Pos pos_start, Pos pos_end);
void make_identifier(Lexer *lexer, Pos start);
void make_single_character_token(Lexer *lexer, TokenType token_type);
void make_token(Lexer *lexer, TokenType token_type, Pos start);
KeywordMatch is_keyword(Lexer *lexer);
bool ends_literal(char c);
bool starts_identifier(char c);
bool proceeds_identifier(char c);
bool is_single_character_token(char c);
TokenType is_double_character_token(Lexer *lexer);
TokenType is_triple_character_token(Lexer *lexer);
bool is_binary_operator(TokenType op);
bool is_unary_operator(TokenType op);
bool is_digit(char c);
void report_error_here(Lexer *lexer, const char *message, ...);
void report_error_helper(Lexer *lexer, const char* label, Pos start, Pos end, const char *message, va_list args);
Line get_line(char *input_str, int line_num);
void free_line(Line line);
const char *label_color(const char *label);

#endif