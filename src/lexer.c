#include "lexer.h"

#define DYNAMIC_ARRAY_IMPLEMENTATION
#include "lib/dynamic_array.h"
#define ARENA_IMPLEMENTATION
#include "lib/arena.h"
#define FILE_IMPLEMENTATION
#include "lib/file.h"

#include <stdarg.h>

const char *LABEL_ERROR   = COLOR_RED     "error"   COLOR_RESET;
const char *LABEL_WARNING = COLOR_MAGENTA "warning" COLOR_RESET;
const char *LABEL_HINT    = COLOR_GREEN   "hint"    COLOR_RESET;
const char *LABEL_NOTE    = COLOR_ICE     "note"    COLOR_RESET;

Lexer lexer_init() {
    Lexer lexer = {0};

    // These two fields are set on a per file basis
    lexer.file_text = NULL;
    lexer.file_path = NULL;

    lexer.tokens = da_init(4096, sizeof(Token));
    lexer.identifier_names = arena_init(4096);

    lexer.char_idx    = 0;
    lexer.col         = 1;
    lexer.line_number = 1;

    return lexer;
}

char *token_type_to_str(TokenType token_type) {
    if (token_type >= 0 && token_type <= 127) {
        char *name_str = (char *)(malloc(8));
        sprintf(name_str, "%c", (char)(token_type));
        return name_str;
    }
    switch (token_type) {
        case TOKEN_NONE:          return "NONE";
        case TOKEN_LITERAL_INTEGER:       return "INTEGER";
        case TOKEN_LITERAL_FLOAT:         return "FLOAT";
        case TOKEN_LITERAL_STRING:        return "STRING";
        case TOKEN_LITERAL_IDENTIFIER:    return "IDENTIFIER";
        case TOKEN_LITERAL_BOOLEAN:       return "BOOLEAN";
        case TOKEN_LITERAL_FALSE:         return "FALSE";
        case TOKEN_LITERAL_TRUE:          return "TRUE";
        case TOKEN_LITERAL_NULL:          return "NULL";
        case TOKEN_BITWISE_SHIFT_LEFT:    return "<<";
        case TOKEN_BITWISE_SHIFT_RIGHT:   return ">>";
        case TOKEN_DOUBLE_COLON:  return "DOUBLE_COLON";
        case TOKEN_COLON_EQUAL:   return "COLON_EQUAL";
        case TOKEN_RIGHT_ARROW:   return "RIGHT_ARROW";
        case TOKEN_DOUBLE_DOT:    return "DOUBLE_DOT";
        case TOKEN_TRIPLE_DOT:    return "TRIPLE_DOT";
        case TOKEN_IN:            return "IN";
        case TOKEN_XX:            return "XX";
        case TOKEN_LOGICAL_OR:    return "LOGICAL_OR";
        case TOKEN_LOGICAL_AND:   return "LOGICAL_AND";
        case TOKEN_LESS_EQUAL:    return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL: return "GREATER_EQUAL";
        case TOKEN_DOUBLE_EQUAL:  return "DOUBLE_EQUAL";
        case TOKEN_NOT_EQUAL:     return "NOT_EQUAL";
        case TOKEN_PLUS_EQUAL:    return "PLUS_EQUAL";
        case TOKEN_MINUS_EQUAL:   return "MINUS_EQUAL";
        case TOKEN_TIMES_EQUAL:   return "TIMES_EQUAL";
        case TOKEN_DIVIDE_EQUAL:  return "DIVIDE_EQUAL";
        case TOKEN_PRINT:         return "PRINT";
        case TOKEN_ASSERT:        return "ASSERT";
        case TOKEN_TYPEOF:        return "TYPEOF";
        case TOKEN_SIZEOF:        return "SIZEOF";
        case TOKEN_RETURN:        return "RETURN";
        case TOKEN_BREAK:         return "BREAK";
        case TOKEN_CONTINUE:      return "CONTINUE";
        case TOKEN_FOR:           return "FOR";
        case TOKEN_WHILE:         return "WHILE";
        case TOKEN_IF:            return "IF";
        case TOKEN_ELSE:          return "ELSE";
        case TOKEN_CAST:          return "CAST";
        case TOKEN_NEW:           return "NEW";
        case TOKEN_IMPORT:        return "IMPORT";
        case TOKEN_TYPE_UINT:     return "TYPE_UINT";
        case TOKEN_TYPE_U8:       return "TYPE_U8";
        case TOKEN_TYPE_U16:      return "TYPE_U16";
        case TOKEN_TYPE_U32:      return "TYPE_U32";
        case TOKEN_TYPE_U64:      return "TYPE_U64";
        case TOKEN_TYPE_INT:      return "TYPE_INT";
        case TOKEN_TYPE_S8:       return "TYPE_S8";
        case TOKEN_TYPE_S16:      return "TYPE_S16";
        case TOKEN_TYPE_S32:      return "TYPE_S32";
        case TOKEN_TYPE_S64:      return "TYPE_S64";
        case TOKEN_TYPE_FLOAT:    return "TYPE_FLOAT";
        case TOKEN_TYPE_F32:      return "TYPE_F32";
        case TOKEN_TYPE_F64:      return "TYPE_F64";
        case TOKEN_TYPE_STRING:   return "TYPE_STRING";
        case TOKEN_TYPE_BOOL:     return "TYPE_BOOL";
        case TOKEN_ANY:      return "TYPE_ANY";
        case TOKEN_TYPE_VOID:     return "TYPE_VOID";
        case TOKEN_STRUCT:        return "TYPE_STRUCT";
        case TOKEN_ENUM:          return "TYPE_ENUM";
        case TOKEN_METHOD:        return "METHOD";
        case TOKEN_END:           return "END";
    }

    printf("%s:%d: compiler-error: Unknown token type '%d'\n", __FILE__, __LINE__, token_type);
    return NULL;
}

bool lex_file(Lexer *lexer) {
    lexer->char_idx    = 0;
    lexer->col         = 1;
    lexer->line_number = 1;

    while (peek_next_char(lexer) != '\0') {
        
        char c = peek_next_char(lexer);

        if (c == ' ' || c == '\t') {
            eat_character(lexer);
            continue;
        }

        if (c == '\n') {
            eat_character(lexer);
            lexer->line_number += 1;
            lexer->col          = 1;
            continue;
        }

        if (c == '/') { // Single line comments
            char next = peek_char(lexer, 1);
            if (next == '/') {
                eat_character(lexer);
                eat_character(lexer);
                next = peek_next_char(lexer);
                while (true) {
                    if (next == '\n') break;
                    if (next == '\0') break;
                    eat_character(lexer);
                    next = peek_next_char(lexer);
                }
                continue;
            }
        }

        if (is_digit(c)) {
            Pos pos_start = get_current_position(lexer);
            eat_character(lexer);
            char next = peek_next_char(lexer);
            while (is_digit(next)) {
                eat_character(lexer);
                next = peek_next_char(lexer);
            }

            if (next == '.' && peek_char(lexer, 1) == '.') {
                // Early stop as the next is a range expr
                make_literal_here(lexer, TOKEN_LITERAL_INTEGER, pos_start);
                continue;
            }

            if (next == '.') {
                eat_character(lexer);
                next = peek_next_char(lexer);

                while (is_digit(next)) {
                    eat_character(lexer);
                    next = peek_next_char(lexer);
                }

                make_literal_here(lexer, TOKEN_LITERAL_FLOAT, pos_start);
            } else {
                make_literal_here(lexer, TOKEN_LITERAL_INTEGER, pos_start);
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

            make_literal_here(lexer, TOKEN_LITERAL_STRING, pos_start);
            continue;
        }

        {
            // Keywords
            Pos pos_start = get_current_position(lexer);
            KeywordMatch keyword = match_keyword(lexer);
            if (keyword.matched) {
                eat_characters(lexer, keyword.length);

                if (keyword.token == TOKEN_LITERAL_TRUE || keyword.token == TOKEN_LITERAL_FALSE) {
                    make_literal_here(lexer, keyword.token, pos_start);
                } else {
                    make_keyword(lexer, keyword.token, pos_start);
                }
                continue;
            }
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
            // Triple character tokens
            TokenType token = is_triple_character_token(lexer);
            if (token) {
                Pos start = get_current_position(lexer);
                eat_characters(lexer, 3);
                make_token(lexer, token, start);
                continue;
            }
        }

        {
            // Double character tokens
            TokenType token = is_double_character_token(lexer);
            if (token) {
                Pos start = get_current_position(lexer);
                eat_characters(lexer, 2);
                make_token(lexer, token, start);
                continue;
            }
        }

        if (is_single_character_token(c)) {
            eat_character(lexer);
            make_single_character_token(lexer, (TokenType)(c));
            continue;
        }

        report_error_here(lexer, "Syntax Error: Unknown character: %d", (int) c);
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

KeywordMatch match_keyword(Lexer *lexer) {
    int i = 0;
    while (true) {
        char c = peek_char(lexer, i);
        if (c == ' ')  break;
        if (c == ';')  break;
        if (c == '(')  break;
        if (c == ')')  break;
        if (c == '}')  break;
        if (c == '[')  break;
        if (c == ']')  break;
        if (c == ',')  break;
        if (c == '.')  break;
        if (c == '=')  break;
        if (c == '\0') break;

        i++;
    }
    
    int keyword_start = lexer->char_idx;
    int keyword_len   = i;

    char *src   = &lexer->file_text[keyword_start];
    char *text  = (char *)(malloc(keyword_len + 1));
    memset(text, '\0', keyword_len + 1);
    memcpy(text, src, keyword_len);

    TokenType token = TOKEN_NONE;
    if (keyword_len == 2) {
        if (strcmp(text, "if") == 0) token = TOKEN_IF;
        if (strcmp(text, "in") == 0) token = TOKEN_IN;
        if (strcmp(text, "u8") == 0) token = TOKEN_TYPE_U8;
        if (strcmp(text, "s8") == 0) token = TOKEN_TYPE_S8;
        if (strcmp(text, "xx") == 0) token = TOKEN_XX;
    }
    if (keyword_len == 3) {
        if (strcmp(text, "new") == 0) token = TOKEN_NEW;
        if (strcmp(text, "int") == 0) token = TOKEN_TYPE_INT;
        if (strcmp(text, "for") == 0) token = TOKEN_FOR;
        if (strcmp(text, "u16") == 0) token = TOKEN_TYPE_U16;
        if (strcmp(text, "u32") == 0) token = TOKEN_TYPE_U32;
        if (strcmp(text, "u64") == 0) token = TOKEN_TYPE_U64;
        if (strcmp(text, "s16") == 0) token = TOKEN_TYPE_S16;
        if (strcmp(text, "s32") == 0) token = TOKEN_TYPE_S32;
        if (strcmp(text, "s64") == 0) token = TOKEN_TYPE_S64;
        if (strcmp(text, "f32") == 0) token = TOKEN_TYPE_F32;
        if (strcmp(text, "f64") == 0) token = TOKEN_TYPE_F64;
        if (strcmp(text, "any") == 0) token = TOKEN_ANY;
    }
    if (keyword_len == 4) {
        if (strcmp(text, "uint") == 0) token = TOKEN_TYPE_UINT;
        if (strcmp(text, "null") == 0) token = TOKEN_LITERAL_NULL;
        if (strcmp(text, "bool") == 0) token = TOKEN_TYPE_BOOL;
        if (strcmp(text, "void") == 0) token = TOKEN_TYPE_VOID;
        if (strcmp(text, "true") == 0) token = TOKEN_LITERAL_TRUE;
        if (strcmp(text, "else") == 0) token = TOKEN_ELSE;
        if (strcmp(text, "enum") == 0) token = TOKEN_ENUM;
        if (strcmp(text, "cast") == 0) token = TOKEN_CAST;
    }
    if (keyword_len == 5) {
        if (strcmp(text, "float") == 0) token = TOKEN_TYPE_FLOAT;
        if (strcmp(text, "print") == 0) token = TOKEN_PRINT;
        if (strcmp(text, "false") == 0) token = TOKEN_LITERAL_FALSE;
        if (strcmp(text, "break") == 0) token = TOKEN_BREAK;
        if (strcmp(text, "while") == 0) token = TOKEN_WHILE;
    }
    if (keyword_len == 6) {
        if (strcmp(text, "struct") == 0) token = TOKEN_STRUCT;
        if (strcmp(text, "method") == 0) token = TOKEN_METHOD;
        if (strcmp(text, "string") == 0) token = TOKEN_TYPE_STRING;
        if (strcmp(text, "return") == 0) token = TOKEN_RETURN;
        if (strcmp(text, "assert") == 0) token = TOKEN_ASSERT;
        if (strcmp(text, "typeof") == 0) token = TOKEN_TYPEOF;
        if (strcmp(text, "sizeof") == 0) token = TOKEN_SIZEOF;
        if (strcmp(text, "import") == 0) token = TOKEN_IMPORT;
    }
    if (keyword_len == 8) {
        if (strcmp(text, "continue") == 0) token = TOKEN_CONTINUE;
    }

    if (token == TOKEN_NONE) {
        return (KeywordMatch){.matched = false};
    } else {
        return (KeywordMatch){.token = token, .length = keyword_len, .matched = true};
    }
}

void make_identifier(Lexer *lexer, Pos pos_start) {
    Pos pos_end = get_current_position(lexer);

    // @Cleanup - this is a bit meh, a lot of work, just to zero terminate the
    //            identifier ...
    int   ident_length = pos_end.input_idx - pos_start.input_idx;
    char *temp_buffer  = (char *)(malloc(ident_length + 1));
    if (temp_buffer == NULL) {
        printf("Internal Compiler Error: Failed to allocate memory for temporary buffer for an identifier");
        exit(1);
    }
    memset(temp_buffer, '\0', ident_length + 1);
    memcpy(temp_buffer, &lexer->file_text[pos_start.input_idx], ident_length);

    char *ident_name   = (char *)(arena_allocate(&lexer->identifier_names, ident_length + 1));
    memcpy(ident_name, temp_buffer, ident_length + 1);
    free(temp_buffer);

    Token token = {0};
    token.type  = TOKEN_LITERAL_IDENTIFIER;
    token.as_value = (As_value) {
        .flags = 0,
        .value = {
            .identifier = { .name = ident_name, .length = ident_length }
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
    
    Pos start        = get_current_position(lexer);
    start.col       -= 1;
    start.input_idx -= 1;

    make_token(lexer, token_type, start);
}

void push_token(Lexer *lexer, Token token) {
    da_append(&lexer->tokens, token);
}

void make_literal_here(Lexer *lexer, TokenType token_type, Pos pos_start) {
    Pos pos_end = get_current_position(lexer);
    make_literal(lexer, token_type, pos_start, pos_end);
}

void make_literal(Lexer *lexer, TokenType token_type, Pos pos_start, Pos pos_end) {
    Token result = {0};
    result.start = pos_start;
    result.end   = pos_end;

    switch(token_type) {
    case TOKEN_LITERAL_INTEGER: {
        const char *src = &lexer->file_text[pos_start.input_idx];
        char val_str[MAX_DIGITS];
        memset(val_str, '\0', MAX_DIGITS);

        strncpy(val_str, src, pos_end.input_idx - pos_start.input_idx);
        unsigned long long val = strtoull(val_str, NULL, 0);

        result.type = token_type;
        if (val > S64_MAX) {
            result.as_value.flags |= VALUE_IS_BIGGER_THAN_SIGNED_64_BIT_INTEGER;
        }

        result.as_value.value.integer = (long long)(val);
        
        break;
    }
    case TOKEN_LITERAL_FLOAT : {
        const char *src = &lexer->file_text[pos_start.input_idx];
        char val_str[MAX_DIGITS];
        memset(val_str, '\0', MAX_DIGITS);

        strncpy(val_str, src, pos_end.input_idx - pos_start.input_idx);
        double val = atof(val_str);

        result.type = token_type;
        result.as_value.value.floating = val;

        break;
    }
    case TOKEN_LITERAL_STRING: {
        // For the token start and end position we save it including the quotes,
        // but don't include them for the value of the string.
        const char *src = &lexer->file_text[pos_start.input_idx + 1];
        int str_len = pos_end.input_idx - pos_start.input_idx - 2;
        char *buffer = (char *)(malloc(str_len + 1));
        memset(buffer, '\0', str_len + 1);
        memcpy(buffer, src, str_len);

        result.type = token_type;
        result.as_value = (As_value) {
            .flags = 0,
            .value = {
                .string = {.data = buffer, .length = str_len}  
            }
        };

        break;
    }
    case TOKEN_LITERAL_TRUE:
    case TOKEN_LITERAL_FALSE: {
        result.type = TOKEN_LITERAL_BOOLEAN;
        result.as_value.value.boolean = token_type == TOKEN_LITERAL_TRUE ? true : false;

        break;
    }
    default:
        printf("Error: Invalid token type with id '%d' \n", token_type);
        exit(1);
    }

    push_token(lexer, result);
}

void eat_characters(Lexer *lexer, int chars_to_eat) {
    for (int i = 0; i < chars_to_eat; i++) {
        eat_character(lexer);
    }
}

void eat_character(Lexer *lexer) {
    lexer->col++;
    lexer->char_idx++;
}

char peek_char(Lexer *lexer, int lookahead) {
    return lexer->file_text[lexer->char_idx + lookahead];
}

char peek_next_char(Lexer *lexer) {
    return peek_char(lexer, 0);
}

bool ends_literal(char c) {
    if (c == ',')  return true;
    if (c == ' ')  return true;
    if (c == ';')  return true;
    if (c == ')')  return true;
    if (c == ',')  return true;
    if (c == '-')  return true;
    if (c == '}')  return true;
    if (c == '\n')  return true;
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
    if (op == '-') return true;
    if (op == '&') return true;
    if (op == '*') return true;
    if (op == '~') return true;
    return false;
}

bool is_binary_operator(TokenType op) {
    if (op == '+') return true;
    if (op == '-') return true;
    if (op == '*') return true;
    if (op == '/') return true;
    if (op == '%') return true;
    
    if (op == '&') return true;
    if (op == '|') return true;
    if (op == '^') return true;
    if (op == TOKEN_BITWISE_SHIFT_LEFT)  return true;
    if (op == TOKEN_BITWISE_SHIFT_RIGHT) return true;

    if (op == '<')                 return true;
    if (op == '>')                 return true;
    if (op == TOKEN_LOGICAL_OR)    return true;
    if (op == TOKEN_LOGICAL_AND)   return true;
    if (op == TOKEN_LESS_EQUAL)    return true;
    if (op == TOKEN_GREATER_EQUAL) return true;
    if (op == TOKEN_DOUBLE_EQUAL)  return true;
    if (op == TOKEN_NOT_EQUAL)     return true;

    if (op == '.') return true;
    if (op == '[') return true;

    return false;
}

TokenType is_triple_character_token(Lexer *lexer) {
    char c1 = peek_next_char(lexer);
    char c2 = peek_char(lexer, 1);
    char c3 = peek_char(lexer, 2);
    if (c1 == '.' && c2 == '.' && c3 == '.') return TOKEN_TRIPLE_DOT;

    return (TokenType)(0);
}

TokenType is_double_character_token(Lexer *lexer) {
    char c    = peek_next_char(lexer);
    char next = peek_char(lexer, 1);
    if (c == '<' && next == '<') return TOKEN_BITWISE_SHIFT_LEFT;
    if (c == '>' && next == '>') return TOKEN_BITWISE_SHIFT_RIGHT;
    if (c == '&' && next == '&') return TOKEN_LOGICAL_AND;
    if (c == '&' && next == '&') return TOKEN_LOGICAL_AND;
    if (c == '|' && next == '|') return TOKEN_LOGICAL_OR;
    if (c == '<' && next == '=') return TOKEN_LESS_EQUAL;
    if (c == '>' && next == '=') return TOKEN_GREATER_EQUAL;
    if (c == '=' && next == '=') return TOKEN_DOUBLE_EQUAL;
    if (c == '!' && next == '=') return TOKEN_NOT_EQUAL;
    if (c == ':' && next == '=') return TOKEN_COLON_EQUAL;
    if (c == ':' && next == ':') return TOKEN_DOUBLE_COLON;
    if (c == '-' && next == '>') return TOKEN_RIGHT_ARROW;
    if (c == '.' && next == '.') return TOKEN_DOUBLE_DOT;
    if (c == '+' && next == '=') return TOKEN_PLUS_EQUAL;
    if (c == '-' && next == '=') return TOKEN_MINUS_EQUAL;
    if (c == '*' && next == '=') return TOKEN_TIMES_EQUAL;
    if (c == '/' && next == '=') return TOKEN_DIVIDE_EQUAL;

    return (TokenType)(0);
}

bool is_single_character_token(char c) {
    if (c == '+') return true;
    if (c == '-') return true;
    if (c == '*') return true;
    if (c == '/') return true;
    if (c == '%') return true;
    if (c == '^') return true;
    if (c == '<') return true;
    if (c == '>') return true;
    if (c == '!') return true;
    if (c == '=') return true;
    if (c == ';') return true;
    if (c == '{') return true;
    if (c == '}') return true;
    if (c == '[') return true;
    if (c == ']') return true;
    if (c == '(') return true;
    if (c == ')') return true;
    if (c == '&') return true;
    if (c == '|') return true;
    if (c == ',') return true;
    if (c == ':') return true;
    if (c == '.') return true;
    if (c == '#') return true;
    return false;
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

char *text_bold(char *text) {
    int text_len = strlen(text);
    char *result = malloc(text_len + 16);
    assert(result);

    sprintf(result, COLOR_WHITE_BOLD"%s"COLOR_RESET, text);
    return result;
}

void report_error_here(Lexer *lexer, const char *message, ...) {
    va_list args;
    va_start(args, message);

    Pos here = get_current_position(lexer);
    report_error_helper(lexer->file_path, lexer->file_text, LABEL_ERROR, here, here, message, args);

    va_end(args);
}

#define MAX_ERROR_LEN 512
void report_error_helper(char *file_path, char *file_text, const char* label, Pos start, Pos end, const char *template, va_list args) {
    char message[MAX_ERROR_LEN];
    vsnprintf(message, MAX_ERROR_LEN, template, args);
    
    if (start.line == -1) {
        // Skip reporting code line of location
        printf("\n" "%s " "%s" ": %s\n", file_path, label, message);
        return;
    }

    // Header
    printf("\n" "%s:%d:%d " "%s" ": %s\n", file_path, start.line, start.col, label, message);

    //
    // Code lines
    //
    int num_lines = (end.line - start.line) + 1;
    bool singleline = num_lines == 1;
    if (singleline) {
        Line line = get_line(file_text, start.line);
        char *start_segment = malloc(start.col);
        memset(start_segment, '\0', start.col);
        memcpy(start_segment, line.str, start.col - 1);

        //  Highlight part
        int highlight_len = end.input_idx - start.input_idx;
        char *highlight_segment = malloc(highlight_len + 1);
        memset(highlight_segment, '\0', highlight_len + 1);
        memcpy(highlight_segment, &line.str[start.col - 1], highlight_len);


        int end_segment_len = line.end_idx - end.input_idx;
        char *end_segment = malloc(end_segment_len + 1);
        memset(end_segment, '\0', end_segment_len + 1);
        memcpy(end_segment, &file_text[end.input_idx], end_segment_len);

        char line_num[10];
        sprintf(line_num, "  %d", start.line);
        printf("%s | %s" "%s" "%s" COLOR_RESET "%s \n", line_num, start_segment, label_color(label), highlight_segment, end_segment);

        // Underline
        for (size_t i = 0; i < strlen(line_num) + 1; i++) printf(" ");
        printf("| ");
        for (int i = 0; i < start.col - 1; i++) printf(" ");
        printf("%s^", label_color(label));
        for (int i = start.col; i < end.col - 1; i++) printf("~");
        printf(COLOR_RESET);
        printf("\n");

        free_line(line);
    } else {
        for (int i = 0; i < num_lines; i++) {
            int line_num = start.line + i;
            Line line = get_line(file_text, line_num);
            if (i == 0) {
                char *start_segment = malloc(start.col);
                memset(start_segment, '\0', start.col);
                memcpy(start_segment, line.str, start.col - 1);

                char *rest = &line.str[start.col - 1];
                printf("  %d | %s" "%s" "%s" COLOR_RESET "\n", line_num, start_segment, label_color(label), rest);
                
                free_line(line);
                continue;
            }
            if (i == num_lines - 1) {
                char *end_segment = malloc(end.col);
                memset(end_segment, '\0', end.col);
                memcpy(end_segment, line.str, end.col - 1);

                char *rest = &line.str[end.col - 1];
                printf("  %d | " "%s" "%s" COLOR_RESET "%s" "\n", line_num, label_color(label), end_segment, rest);

                free_line(line);
                continue;
            }

            printf("  %d | " "%s" "%s" COLOR_RESET "\n", line_num, label_color(label), line.str);

            free_line(line);
        }
    }
}

Line get_line(char *input_str, int line_num) {
    // Seek to the start of the line
    int start_idx = 0;
    int current_line = 1;
    char *c = input_str;
    while (current_line != line_num) {
        if (*c == '\n') current_line++;
        if (*c == '\0') break;
        c++;
        start_idx++;
    }

    // Seek for the end of the line
    int end_idx = start_idx;
    c = &input_str[start_idx];
    while ((*c != '\n') && (*c != '\0')) {
        c++;
        end_idx++;
    }

    int line_len = end_idx - start_idx;
    char *line   = (char *)(malloc(line_len + 1));
    memset(line, '\0', line_len + 1);
    memcpy(line, &input_str[start_idx], line_len);

    return (Line) {
        .str       = line,
        .start_idx = start_idx,
        .end_idx   = end_idx
    };
}

void free_line(Line line) {
    free(line.str);
}

const char *label_color(const char *label) {
    if (label == LABEL_NOTE)    return COLOR_ICE;
    if (label == LABEL_WARNING) return COLOR_MAGENTA;
    if (label == LABEL_ERROR)   return COLOR_RED;

    return COLOR_WHITE;
}
