#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lib/arena.c"
#include "lib/string.c"

#define MAX_TOKENS 1024
#define MAX_DIGITS 32
#define IDENT_NAME_BUFFER_SIZE (size_t)1024

#define XXX assert(false && "Not implemented")


#define COLOR_RESET  "\x1B[0m"
#define COLOR_RED    "\x1B[31m"
#define COLOR_YELLOW "\x1B[33m"
#define COLOR_ICE    "\x1B[36m"
#define COLOR_WHITE  "\x1B[37m"
#define COLOR_WHITE_BOLD  "\x1B[1;37m"


static const char *LABEL_NONE    = COLOR_ICE    "none"   COLOR_RESET;
static const char *LABEL_ERROR   = COLOR_RED    "error"   COLOR_RESET;
static const char *LABEL_WARNING = COLOR_YELLOW "warning" COLOR_RESET;

typedef enum TokenType {
    // Mentally insert ASCII values here ...

    TOKEN_INTEGER    = 128,
    TOKEN_FLOAT      = 129,
    TOKEN_STRING     = 130,
    TOKEN_IDENTIFIER = 131,
    TOKEN_BOOLEAN    = 132,
    TOKEN_FALSE      = 133,
    TOKEN_TRUE       = 134,

    TOKEN_LOGICAL_OR    = 140,
    TOKEN_LOGICAL_AND   = 141,
    TOKEN_LESS_EQUAL    = 142,
    TOKEN_GREATER_EQUAL = 143,
    TOKEN_DOUBLE_EQUAL  = 144,
    TOKEN_NOT_EQUAL     = 145,

    TOKEN_DOUBLE_COLON = 150,
    TOKEN_COLON_EQUAL  = 151,


    TOKEN_PRINT        = 160,
    TOKEN_IF           = 161,
    TOKEN_ELSE         = 162,

    TOKEN_TYPE_INT     = 180,
    TOKEN_TYPE_FLOAT   = 181,
    TOKEN_TYPE_STRING  = 182,
    TOKEN_TYPE_BOOL    = 183,
    
    TOKEN_END          = 254
} TokenType;

typedef union As_value {
    bool   boolean;
    int    integer;
    double floating;
    struct {
        char *data;
        int   length;
    } string;
    struct {
        char *name;
        int   length;    
    } identifier;
} As_value;

typedef struct Line {
    char *str;
    int start;
    int end;
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

    Token tokens[MAX_TOKENS];
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
void eat_multiple_characters(Lexer *lexer, int chars_to_eat);
char peek_char(Lexer *lexer, int lookahead);
char peek_next_char(Lexer *lexer);
void push_token(Lexer *lexer, Token token);
Pos get_current_position(Lexer *lexer);
char *token_type_to_str(TokenType token_type);
void make_keyword(Lexer *lexer, TokenType kw_type, Pos start);
void make_literal(Lexer *lexer, TokenType token_type, Pos start);
void make_identifier(Lexer *lexer, Pos start);
void make_single_character_token(Lexer *lexer, TokenType token_type);
void make_token(Lexer *lexer, TokenType token_type, Pos start);
KeywordMatch matches_keyword(Lexer *lexer);
TokenType matches_double_character_operator(char current, char next);
bool ends_literal(char c);
bool starts_identifier(char c);
bool proceeds_identifier(char c);
bool is_single_character_operator(char c);
bool is_binary_operator(TokenType op);
bool is_unary_operator(TokenType op);
bool is_digit(char c);
void report_syntax_error(Lexer *lexer, const char *message, ...);
void report_error_helper(Lexer *lexer, const char* label, Pos start, Pos end, const char *message, va_list args);
Line get_input_line(char *input_str, int start_pos);
void free_line(Line line);
const char *label_color(const char *label);

Lexer lexer_init(char *input_str, const char *file_path) {
    Lexer lexer = {0};
    lexer.input_str = input_str;
    lexer.file_path = file_path;
    lexer.identifier_names = arena_make(1024);

    return lexer;
}

char *token_type_to_str(TokenType token_type) {
    if (token_type >= 0 && token_type <= 127) {
        char *name_str = (char *)(malloc(8));
        sprintf(name_str, "%c", (char)(token_type));
        return name_str;
    }
    switch (token_type) {
        case TOKEN_INTEGER:       return "INTEGER";
        case TOKEN_FLOAT:         return "FLOAT";
        case TOKEN_STRING:        return "STRING";
        case TOKEN_IDENTIFIER:    return "IDENTIFIER";
        case TOKEN_BOOLEAN:       return "BOOLEAN";
        case TOKEN_FALSE:         return "FALSE";
        case TOKEN_TRUE:          return "TRUE";
        case TOKEN_DOUBLE_COLON:  return "DOUBLE_COLON";
        case TOKEN_COLON_EQUAL:   return "COLON_EQUAL";
        case TOKEN_LOGICAL_OR:    return "LOGICAL_OR";
        case TOKEN_LOGICAL_AND:   return "LOGICAL_AND";
        case TOKEN_LESS_EQUAL:    return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_DOUBLE_EQUAL:  return "DOUBLE_EQUAL";
        case TOKEN_NOT_EQUAL:     return "NOT_EQUAL";
        case TOKEN_PRINT:         return "PRINT";
        case TOKEN_IF:            return "IF";
        case TOKEN_ELSE:          return "ELSE";
        case TOKEN_TYPE_INT:      return "TYPE_INT";
        case TOKEN_TYPE_FLOAT:    return "TYPE_FLOAT";
        case TOKEN_TYPE_STRING:   return "TYPE_STRING";
        case TOKEN_TYPE_BOOL:     return "TYPE_BOOL";
        case TOKEN_END:           return "END";
    }

    printf("%s:%d: compiler-error: Unknown token type '%d'\n", __FILE__, __LINE__, token_type);
    return NULL;
}

bool lex(Lexer *lexer) {
    while (peek_next_char(lexer) != '\0') {
        char c = peek_next_char(lexer);

        if (c == ' ') {
            eat_character(lexer);
            continue;
        }

        if (c == '\n') {
            eat_character(lexer);
            lexer->line_number += 1;
            lexer->col          = 0;
            continue;
        }

        if (is_digit(c)) {
            Pos pos_start = get_current_position(lexer);
            eat_character(lexer);
            char next = peek_next_char(lexer);
            while (is_digit(next)) {
                eat_character(lexer);
                next = peek_next_char(lexer);
            }
            if (next == '.') {
                eat_character(lexer);
                next = peek_next_char(lexer);
                while (is_digit(next)) {
                    eat_character(lexer);
                    next = peek_next_char(lexer);
                }

                if (!ends_literal(next)) {
                    report_syntax_error(lexer, "Invalid float literal. Probably a missing ;");
                    return false;
                }
                make_literal(lexer, TOKEN_FLOAT, pos_start);
            } else {
                if (!ends_literal(next)) {
                    report_syntax_error(lexer, "Invalid integer literal. Probably a missing ;");
                    return false;
                }
                make_literal(lexer, TOKEN_INTEGER, pos_start);
            }

            continue;
        }

        if (c == '"') {
            Pos pos_start = get_current_position(lexer);
            eat_character(lexer);
            char next = peek_next_char(lexer);
            while (next != '"') {
                eat_character(lexer);
                next = peek_next_char(lexer);
            }
            eat_character(lexer);

            make_literal(lexer, TOKEN_STRING, pos_start);
            continue;
        }

        {
            // Keywords
            Pos pos_start = get_current_position(lexer);
            KeywordMatch keyword = matches_keyword(lexer);
            if (keyword.matched) {
                eat_multiple_characters(lexer, keyword.length);

                if (keyword.token == TOKEN_TRUE || keyword.token == TOKEN_FALSE) {
                    make_literal(lexer, keyword.token, pos_start);
                } else {
                    make_keyword(lexer, keyword.token, pos_start);
                }
                continue;
            }
            // Fallthrough
        }

        if (starts_identifier(c)) {
            Pos pos_start = get_current_position(lexer);
            eat_character(lexer);

            char next = peek_next_char(lexer);
            while (proceeds_identifier(next)) {
                eat_character(lexer);
                next = peek_next_char(lexer);
            }

            make_identifier(lexer, pos_start);

            continue;
        }

        {
            Pos start = get_current_position(lexer);
            char next = peek_char(lexer, 1);
            TokenType op = matches_double_character_operator(c, next);
            if (op != 0) {
                eat_character(lexer);
                eat_character(lexer);
                make_token(lexer, op, start);
                continue;
            } else {
                // Fallthrough
            }
        }

        if (is_single_character_operator(c)) {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }

        if (c == '=') {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }

        if (c == ';') {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }

        if (c == '{') {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }
        if (c == '}') {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }

        if (c == ':') {
            Pos pos_start = get_current_position(lexer);
            eat_character(lexer);

            char next = peek_next_char(lexer);
            if (next == ':') {
                eat_character(lexer);
                make_token(lexer, TOKEN_DOUBLE_COLON, pos_start);
                continue;
            }
            if (next == '=') {
                eat_character(lexer);
                make_token(lexer, TOKEN_COLON_EQUAL, pos_start);
                continue;
            }

            make_single_character_token(lexer, (TokenType)(':'));
            continue;
        }

        if (c == '(' || c == ')') {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        } 

        report_syntax_error(lexer, "Unknown character");
        return false;
    }

    Pos here = get_current_position(lexer);
    push_token(lexer, (Token){.type = TOKEN_END, .start = here, .end = here});
    return true;
}

Pos get_current_position(Lexer *lexer) {
    return (Pos) {
        .input_idx = lexer->char_idx,
        .line      = lexer->line_number,
        .col       = lexer->col
    };
}

void make_keyword(Lexer *lexer, TokenType kw_type, Pos start) {
    Token token = {0};
    token.type  = kw_type;
    token.start = start;
    token.end   = get_current_position(lexer);

    push_token(lexer, token);
}

KeywordMatch matches_keyword(Lexer *lexer) {
    int i = 0;
    while (true) {
        char c = peek_char(lexer, i);
        if (c == ' ')  break;
        if (c == ';')  break;
        if (c == '(')  break;
        if (c == ')')  break;
        if (c == '\0') break;

        i++;
    }
    
    int keyword_start = lexer->char_idx;
    int keyword_len   = i;

    char *src   = &lexer->input_str[keyword_start];
    char *text  = (char *)(malloc(keyword_len + 1));
    memset(text, '\0', keyword_len + 1);
    memcpy(text, src, keyword_len);

    if (keyword_len == 2) {
        if (strcmp(text, "if") == 0) return (KeywordMatch){.token = TOKEN_IF, .length = keyword_len, .matched = true };
    }
    if (keyword_len == 3) {
        if (strcmp(text, "int") == 0) return (KeywordMatch){.token = TOKEN_TYPE_INT, .length = keyword_len, .matched = true };
    }
    if (keyword_len == 4) {
        if (strcmp(text, "bool") == 0) return (KeywordMatch){.token = TOKEN_TYPE_BOOL, .length = keyword_len, .matched = true };
        if (strcmp(text, "true") == 0) return (KeywordMatch){.token = TOKEN_TRUE, .length = keyword_len, .matched = true };
        if (strcmp(text, "else") == 0) return (KeywordMatch){.token = TOKEN_ELSE, .length = keyword_len, .matched = true };
    }
    if (keyword_len == 5) {
        if (strcmp(text, "float") == 0)  return (KeywordMatch){.token = TOKEN_TYPE_FLOAT, .length = keyword_len, .matched = true };
        if (strcmp(text, "print") == 0)  return (KeywordMatch){.token = TOKEN_PRINT, .length = keyword_len, .matched = true };
        if (strcmp(text, "false") == 0) return (KeywordMatch){.token = TOKEN_FALSE, .length = keyword_len, .matched = true };
    }
    if (keyword_len == 6) {
        if (strcmp(text, "string") == 0) return (KeywordMatch){.token = TOKEN_TYPE_STRING, .length = keyword_len, .matched = true };
    }

    return (KeywordMatch){.matched = false};
}

void make_identifier(Lexer *lexer, Pos pos_start) {
    Pos pos_end = get_current_position(lexer);

    // @Cleanup - this is a bit meh, a lot of work, just to zero terminate the
    //            identifier ...
    int   ident_length = pos_end.input_idx - pos_start.input_idx;
    char *temp_buffer  = (char *)(malloc(ident_length + 1));
    memset(temp_buffer, '\0', ident_length + 1);
    memcpy(temp_buffer, &lexer->input_str[pos_start.input_idx], ident_length);
    char *ident_name   = (char *)(arena_allocate(&lexer->identifier_names, ident_length + 1));
    memcpy(ident_name, temp_buffer, ident_length + 1);
    free(temp_buffer);

    Token token = {0};
    token.type  = TOKEN_IDENTIFIER;
    token.as_value = (As_value) {
        .identifier = {
            .name   = ident_name,
            .length = ident_length
        }
    };
    token.start = pos_start;
    token.end = pos_end;

    push_token(lexer, token);
}

void make_token(Lexer *lexer, TokenType token_type, Pos pos_start) {
    Token token = {0};
    token.type  = token_type;
    token.start = pos_start;
    token.end   = get_current_position(lexer);

    push_token(lexer, token);
}

void make_single_character_token(Lexer *lexer, TokenType token_type) {
    Pos start = get_current_position(lexer);
    start.col       -= 1;
    start.input_idx -= 1;
    make_token(lexer, token_type, start);
}

void push_token(Lexer *lexer, Token token) {
    lexer->tokens[lexer->token_index_cursor] = token;
    lexer->token_index_cursor++;
}

void make_literal(Lexer *lexer, TokenType token_type, Pos pos_start) {
    Pos pos_end = get_current_position(lexer);

    Token result = {0};
    switch(token_type) {
        case TOKEN_INTEGER: {
            const char *src = &lexer->input_str[pos_start.input_idx];
            char val_str[MAX_DIGITS];
            memset(val_str, '\0', MAX_DIGITS);

            strncpy(val_str, src, pos_end.input_idx - pos_start.input_idx);
            int val = atoi(val_str);
            
            Token token = {0};
            token.type  = token_type;
            token.start = pos_start;
            token.end   = pos_end;
            token.as_value.integer = val;

            result = token;
            break;
        }
        case TOKEN_FLOAT : {
            const char *src = &lexer->input_str[pos_start.input_idx];
            char val_str[MAX_DIGITS];
            memset(val_str, '\0', MAX_DIGITS);

            strncpy(val_str, src, pos_end.input_idx - pos_start.input_idx);
            double val = atof(val_str);
            
            Token token = {0};
            token.type  = token_type;
            token.start = pos_start;
            token.end   = pos_end;
            token.as_value.floating = val;

            result = token;
            break;
        }
        case TOKEN_STRING: {
            // For the token start and end position we save it including the quotes,
            // but don't include them for the value of the string.
            const char *src = &lexer->input_str[pos_start.input_idx + 1];
            int str_len = pos_end.input_idx - pos_start.input_idx - 2;
            char *buffer = (char *)(malloc(str_len + 1));
            memset(buffer, '\0', str_len + 1);
            memcpy(buffer, src, str_len);
 
            Token token = {0};
            token.type  = token_type;
            token.start = pos_start;
            token.end   = pos_end;
            token.as_value = (As_value) {
                .string = {
                    .data   = buffer,
                    .length = str_len
                }
            };

            result = token;
            break;
        }
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            Token token = {0};
            token.type  = TOKEN_BOOLEAN;
            token.start = pos_start;
            token.end   = pos_end;
            token.as_value = (As_value) {
                .boolean = token_type == TOKEN_TRUE ? true : false
            };

            result = token;
            break;
        }
        default:
            printf("Error: Invalid token type with id '%d' \n", token_type);
            exit(1);
    }

    push_token(lexer, result);
}

void eat_multiple_characters(Lexer *lexer, int chars_to_eat) {
    for (int i = 0; i < chars_to_eat; i++) {
        eat_character(lexer);
    }
}

void eat_character(Lexer *lexer) {
    lexer->col++;
    lexer->char_idx++;
}

char peek_char(Lexer *lexer, int lookahead) {
    return lexer->input_str[lexer->char_idx + lookahead];
}

char peek_next_char(Lexer *lexer) {
    return peek_char(lexer, 0);
}

bool ends_literal(char c) {
    if (c == ' ')  return true;
    if (c == ';')  return true;
    if (c == ')')  return true;
    return false;
}

bool starts_identifier(char c) {
    if (c == '_') return true;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return false;
}

bool proceeds_identifier(char c) {
    if (c == '_')             return true;
    if (c >= '0' && c <= '9') return true;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return false;
}

bool is_unary_operator(TokenType op) {
    if (op == '!') return true;
    return false;
}

bool is_binary_operator(TokenType op) {
    if (op == '+') return true;
    if (op == '-') return true;
    if (op == '*') return true;
    if (op == '/') return true;
    if (op == '%') return true;
    if (op == '^') return true;

    if (op == '<')                 return true;
    if (op == '>')                 return true;
    if (op == TOKEN_LOGICAL_OR)    return true;
    if (op == TOKEN_LOGICAL_AND)   return true;
    if (op == TOKEN_LESS_EQUAL)    return true;
    if (op == TOKEN_GREATER_EQUAL) return true;
    if (op == TOKEN_DOUBLE_EQUAL)  return true;
    if (op == TOKEN_NOT_EQUAL)     return true;
    return false;
}

TokenType matches_double_character_operator(char current, char next) {
    if (current == '&' && next == '&') return TOKEN_LOGICAL_AND;
    if (current == '|' && next == '|') return TOKEN_LOGICAL_OR;
    if (current == '<' && next == '=') return TOKEN_LESS_EQUAL;
    if (current == '>' && next == '=') return TOKEN_GREATER_EQUAL;
    if (current == '=' && next == '=') return TOKEN_DOUBLE_EQUAL;
    if (current == '!' && next == '=') return TOKEN_NOT_EQUAL;

    return (TokenType)(0);
}

bool is_single_character_operator(char c) {
    if (c == '+') return true;
    if (c == '-') return true;
    if (c == '*') return true;
    if (c == '/') return true;
    if (c == '%') return true;
    if (c == '^') return true;

    if (c == '<') return true;
    if (c == '>') return true;
    if (c == '!') return true;
    return false;
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

void report_syntax_error(Lexer *lexer, const char *message, ...) {
    va_list args;
    va_start(args, message);

    Pos here = get_current_position(lexer);
    report_error_helper(lexer, LABEL_ERROR, here, here, message, args);

    va_end(args);
}

#define MAX_ERROR_LEN 512

void report_error_helper(Lexer *lexer, const char* label, Pos start, Pos end, const char *template, va_list args) {
    char message[MAX_ERROR_LEN];
    vsnprintf(message, MAX_ERROR_LEN, template, args);

    Line err_line = get_input_line(lexer->input_str, start.input_idx);

    // Header
    if (label != LABEL_NONE) {
        printf("\n" COLOR_WHITE_BOLD "%s:%d:%d " COLOR_RESET "%s" ": %s\n\n", lexer->file_path, start.line + 1, start.col + 1, label, message);
    } else {
        printf("\n" COLOR_WHITE_BOLD "%s:%d:%d " COLOR_RESET ": %s\n\n", lexer->file_path, start.line + 1, start.col + 1, message);
    }

    // Code line
    char *start_segment = malloc(start.col + 1);
    memset(start_segment, '\0', start.col + 1);
    memcpy(start_segment, &lexer->input_str[err_line.start], start.col);

    int error_len = end.col - start.col;
    char *error_segment = malloc(error_len + 1);
    memset(error_segment, '\0', error_len + 1);
    memcpy(error_segment, &lexer->input_str[start.input_idx], error_len);

    int end_segment_len = err_line.end - end.input_idx;
    char *end_segment = malloc(end_segment_len + 1);
    memset(end_segment, '\0', end_segment_len + 1);
    memcpy(end_segment, &lexer->input_str[end.input_idx], end_segment_len);


    printf("   %s" "%s" "%s" COLOR_RESET "%s\n", start_segment, label_color(label), error_segment, end_segment);

    // Underline coloring
    for (int i = 0; i < start.col + 3; i++) printf(" ");
    printf("%s^", label_color(label));
    for (int i = start.col; i < end.col - 1; i++) printf("~");
    printf(COLOR_RESET);
    printf("\n");

    free_line(err_line);
}

Line get_input_line(char *input_str, int start_pos) {
    // Seek for the next and previous line breaks to get the line where the error happened
    int line_end_pos = start_pos;
    char *p1 = &input_str[start_pos];
    while ((*p1 != '\n') && (*p1 != '\0')) {
        p1++;
        line_end_pos++;
    }

    int line_start_pos = start_pos;
    char *p2 = &input_str[start_pos];
    bool found_new_line = false;
    while (true) {
        p2--;
        line_start_pos--;
        if (line_start_pos <= 0) {
            line_start_pos = 0;
            break;
        }
        if (*p2 == '\n') {
            found_new_line = true;
            break;
        }
    }
    if (found_new_line) {
        line_start_pos += 1;
    }

    char *start  = &input_str[line_start_pos];
    int line_len = line_end_pos - line_start_pos;
    char *line   = (char *)(malloc(line_len + 1));
    memset(line, '\0', line_len + 1);
    memcpy(line, start, line_len);

    return (Line) {
        .str   = line,
        .start = line_start_pos,
        .end   = line_end_pos
    };
}

void free_line(Line line) {
    free(line.str);
}

const char *label_color(const char *label) {
    if (label == LABEL_NONE)    return COLOR_ICE;
    if (label == LABEL_ERROR)   return COLOR_RED;
    if (label == LABEL_WARNING) return COLOR_YELLOW;

    return COLOR_WHITE;
}
