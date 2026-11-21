// saad fmt: formats Saad code, so that all code looks alike. See docs/formatting.md
//
// It works on the tokens of the lexer and the comments and blank lines between them, not on the AST, so it is fast,
// and formats a file that has errors of types or names (but not one that can't be lexed). The tokens are never
// changed, only the space between them, so the code means the same after formatting. To be sure, the formatted text
// is lexed again, and a file whose tokens would change is left as it is (format_source() gives an error).
//
// The rules:
// * Lines are kept where they are. Two blank lines or more become one, and blank lines after a { at the end of a
//   line, before a } at the start of a line and at the start and end of the file go away.
// * Indentation is 4 spaces for each level. A line inside brackets that were opened on an earlier line is one
//   level deeper (brackets opened on the same line count once). A line that goes on with the statement of the line
//   before, as that line doesn't end with ; { } or , is one level deeper too.
// * Spaces between tokens: one around binary operators and after , ; and :, none inside () and [], none around .
//   and .., none after a unary operator. Where the tokens alone don't say which it is, e.g < of a comparison or of
//   List<int>, or the { of a block or of Point{1, 2}, a space is kept when there was one.
// * Columns that are lined up stay lined up: in lines one after the other that have the same separator (the : of a
//   declaration, = or := or ::, or a comment at the end), where one line lines it up with extra spaces, all of them
//   are lined up with the longest one. Otherwise there is one space.

#include <string.h>
#include <stdlib.h>

typedef enum FormatElementKind {
    FORMAT_TOKEN,
    FORMAT_COMMENT,
} FormatElementKind;

typedef struct FormatElement {
    FormatElementKind kind;
    TokenType         type;       // Of a token
    const char       *text;       // In the source, without a newline at the end
    int               length;
    int               newlines;   // Before it
    int               spaces;     // Spaces and tabs right before it, on its line
    int               column;     // Where it started in the source, from 0
    bool              multiline;  // A string over more lines
} FormatElement;

typedef enum SeparatorKind {
    SEPARATOR_NONE,
    SEPARATOR_DECLARATION,   // name: Type
    SEPARATOR_ASSIGNMENT,    // a = b, a := b, a += b
    SEPARATOR_CONSTANT,      // A :: b
    SEPARATOR_ARM,           // The => or { of an arm of a switch, e.g case 1 => a
} SeparatorKind;

typedef struct FormatLine {
    int           indent;
    char         *text;            // Without the indentation
    int           length;
    int           capacity;
    bool          blank;
    bool          comment_only;
    bool          attribute_only;  // e.g @private
    bool          arm_only;        // case 1, without => or {, of an arm with more values
    bool          multiline;
    bool          in_struct;       // Inside the { of a struct or a union
    bool          in_braces;       // Inside another {, e.g of a block or a struct literal
    bool          starts_with_attribute;
    bool          aligned;         // Lined up as members or fields, which are lined up always
    SeparatorKind separator;
    TokenType     separator_token; // e.g = or +=. Only lines with the same one are lined up
    int           separator_at;    // Where the spaces that line it up go
    int           separator_column; // Of the separator in the source, or of the type after the : of a declaration
    int           separator_spaces; // The spaces that lined it up in the source
    int           comment_at;      // Where the code before the comment at the end ends, or -1
    int           comment_column;  // Of the comment in the source
} FormatLine;

typedef struct FormatBracket {
    TokenType type;
    int       open_indent;   // Of the line it is opened on
    int       inner_indent;  // Of the lines inside it
    bool      is_struct;     // The { of the members of a struct or union
} FormatBracket;

typedef struct Formatter {
    FormatElement *elements;
    int            count;

    FormatLine    *lines;
    int            line_count;
    int            line_capacity;

    FormatBracket *brackets;
    int            bracket_count;
    int            bracket_capacity;
} Formatter;

static void format_line_append(FormatLine *line, const char *text, int length) {
    if (line->length + length + 1 > line->capacity) {
        int capacity = line->capacity ? line->capacity * 2 : 128;
        while (capacity < line->length + length + 1) capacity *= 2;
        line->text = realloc(line->text, capacity);
        line->capacity = capacity;
    }
    memcpy(line->text + line->length, text, length);
    line->length += length;
    line->text[line->length] = '\0';
}

static FormatLine *format_new_line(Formatter *f) {
    if (f->line_count == f->line_capacity) {
        f->line_capacity = f->line_capacity ? f->line_capacity * 2 : 256;
        f->lines = realloc(f->lines, f->line_capacity * sizeof(FormatLine));
    }
    FormatLine *line = &f->lines[f->line_count++];
    memset(line, 0, sizeof(FormatLine));
    line->comment_at = -1;
    return line;
}

static void format_push_bracket(Formatter *f, FormatBracket bracket) {
    if (f->bracket_count == f->bracket_capacity) {
        f->bracket_capacity = f->bracket_capacity ? f->bracket_capacity * 2 : 64;
        f->brackets = realloc(f->brackets, f->bracket_capacity * sizeof(FormatBracket));
    }
    f->brackets[f->bracket_count++] = bracket;
}

static bool is_opener(TokenType t) { return t == '(' || t == '[' || t == '{'; }
static bool is_closer(TokenType t) { return t == ')' || t == ']' || t == '}'; }

static bool is_literal_token(TokenType t) {
    return t == TOKEN_LITERAL_INTEGER || t == TOKEN_LITERAL_FLOAT || t == TOKEN_LITERAL_STRING || t == TOKEN_LITERAL_IDENTIFIER ||
           t == TOKEN_LITERAL_BOOLEAN || t == TOKEN_LITERAL_FALSE || t == TOKEN_LITERAL_TRUE || t == TOKEN_LITERAL_NULL;
}

// A literal that is a value, not a name, e.g 2 or "a"
static bool is_value_literal(TokenType t) {
    return t == TOKEN_LITERAL_INTEGER || t == TOKEN_LITERAL_FLOAT || t == TOKEN_LITERAL_STRING || t == TOKEN_LITERAL_BOOLEAN ||
           t == TOKEN_LITERAL_FALSE || t == TOKEN_LITERAL_TRUE || t == TOKEN_LITERAL_NULL;
}

static bool is_type_keyword(TokenType t) {
    return (t >= TOKEN_TYPE_UINT && t <= TOKEN_TYPE_VOID) || t == TOKEN_STRING || t == TOKEN_ANY;
}

// A keyword that is a word, e.g return or struct, but not a type like int
static bool is_keyword_token(const FormatElement *e) {
    if (e->kind != FORMAT_TOKEN) return false;
    TokenType t = e->type;
    if (t < 128 || is_literal_token(t) || is_type_keyword(t)) return false;
    // The keywords are written with letters, the operators with signs
    char c = e->text[0];
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// A keyword that is used like a function, e.g typeof(x)
static bool is_call_keyword(TokenType t) {
    return t == TOKEN_TYPEOF || t == TOKEN_SIZEOF || t == TOKEN_ASSERT || t == TOKEN_NEW;
}

// The token ends an operand, so an operator after it is binary, e.g the - of a - b
static bool ends_operand(const FormatElement *e) {
    if (e->kind != FORMAT_TOKEN) return false;
    return is_literal_token(e->type) || is_type_keyword(e->type) || e->type == ')';
}

static bool is_always_binary(TokenType t) {
    switch ((int)t) {
    case '=': case '/': case '%': case '|': case '^':
    case TOKEN_COLON_EQUAL: case TOKEN_DOUBLE_COLON:
    case TOKEN_PLUS_EQUAL: case TOKEN_MINUS_EQUAL: case TOKEN_TIMES_EQUAL: case TOKEN_DIVIDE_EQUAL:
    case TOKEN_MODULO_EQUAL: case TOKEN_XOR_EQUAL: case TOKEN_OR_EQUAL: case TOKEN_AND_EQUAL:
    case TOKEN_SHIFT_LEFT_EQUAL: case TOKEN_SHIFT_RIGHT_EQUAL:
    case TOKEN_DOUBLE_EQUAL: case TOKEN_NOT_EQUAL: case TOKEN_LESS_EQUAL: case TOKEN_GREATER_EQUAL:
    case TOKEN_LOGICAL_AND: case TOKEN_LOGICAL_OR: case TOKEN_DOUBLE_QUESTION:
    case TOKEN_RIGHT_ARROW: case TOKEN_FAT_ARROW: case TOKEN_BITWISE_SHIFT_LEFT:
        return true;
    default:
        return false;
    }
}

static bool is_assignment(TokenType t) {
    switch ((int)t) {
    case '=': case TOKEN_COLON_EQUAL:
    case TOKEN_PLUS_EQUAL: case TOKEN_MINUS_EQUAL: case TOKEN_TIMES_EQUAL: case TOKEN_DIVIDE_EQUAL:
    case TOKEN_MODULO_EQUAL: case TOKEN_XOR_EQUAL: case TOKEN_OR_EQUAL: case TOKEN_AND_EQUAL:
    case TOKEN_SHIFT_LEFT_EQUAL: case TOKEN_SHIFT_RIGHT_EQUAL:
        return true;
    default:
        return false;
    }
}

// + - * & that can be binary or unary
static bool is_ambiguous_operator(TokenType t) {
    return t == '+' || t == '-' || t == '*' || t == '&';
}

typedef enum Spacing { SPACING_NONE, SPACING_ONE, SPACING_KEEP } Spacing;

// If the two texts written together would be lexed as other tokens, e.g - - as --, or / / as a comment
static bool would_merge(const FormatElement *a, const FormatElement *b) {
    char x = a->text[a->length - 1];
    char y = b->text[0];
    bool word_x = (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || (x >= '0' && x <= '9') || x == '_';
    bool word_y = (y >= 'a' && y <= 'z') || (y >= 'A' && y <= 'Z') || (y >= '0' && y <= '9') || y == '_';
    if (word_x && word_y) return true;
    if (b->kind == FORMAT_COMMENT) return false;
    static const char *pairs[] = { "->", "=>", "==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/=", "%=",
                                   "^=", "|=", "&=", "<<", ">>", "::", ":=", "..", "??", "?.", "//", ".0", "0.", NULL };
    for (int i = 0; pairs[i]; i++) {
        if (pairs[i][0] == x && pairs[i][1] == y) return true;
    }
    if (x == '.' && y >= '0' && y <= '9') return true;
    return false;
}

// The space between two tokens on a line. cur_binary says if cur is an operator taken as binary, prev_unary if prev
// was taken as unary, and prev_keep if it couldn't be told, e.g the * of []*int or of xs[i] * 2
static Spacing spacing_between(const FormatElement *prev, bool prev_unary, bool prev_keep, const FormatElement *cur, bool cur_binary,
                               TokenType next_type, bool prev_compare) {
    TokenType p = prev->type;
    TokenType c = cur->type;

    if (c == ',' || c == ';' || c == ')' || c == ']') return SPACING_NONE;
    if (c == '}') return p == ';' ? SPACING_ONE : SPACING_KEEP;
    if (p == '(' || p == '[') return SPACING_NONE;
    if (p == '{') return SPACING_KEEP;

    if (p == '.' || p == TOKEN_QUESTION_DOT || p == TOKEN_DOUBLE_DOT || p == TOKEN_TRIPLE_DOT || p == '@' || p == '#') return SPACING_NONE;
    if (prev_keep) return SPACING_KEEP;
    if (prev_unary) return SPACING_NONE;

    if (c == TOKEN_DOUBLE_DOT || c == TOKEN_QUESTION_DOT) return SPACING_NONE;
    if (c == '.' && (ends_operand(prev) || p == ']' || is_type_keyword(p))) return SPACING_NONE;

    if (p == ',' || p == ';') return SPACING_ONE;
    if (c == ':') return SPACING_NONE;
    if (p == ':') return SPACING_ONE;

    // < and > of a comparison, or of type arguments, e.g List<int>. Next to a value or after ) or ] it is a comparison
    if (c == '<' || c == '>') {
        if (is_value_literal(p) || p == ')' || p == ']') return SPACING_ONE;
        if (cur->length == 1 && is_value_literal(next_type)) return SPACING_ONE;
        return SPACING_KEEP;
    }
    if (p == '<' || p == '>') {
        if (is_value_literal(c) || prev_compare) return SPACING_ONE;
        return SPACING_KEEP;
    }
    if (c == TOKEN_BITWISE_SHIFT_RIGHT || p == TOKEN_BITWISE_SHIFT_RIGHT) return SPACING_KEEP;

    // * and & after ] are the pointer of a type, e.g []*int, or a product, e.g xs[i] * 2
    if ((c == '*' || c == '&') && p == ']') return SPACING_KEEP;

    if (is_always_binary(c) || (is_ambiguous_operator(c) && cur_binary)) return SPACING_ONE;
    if (is_always_binary(p) || (is_ambiguous_operator(p) && !prev_unary)) return SPACING_ONE;

    if (c == '(') {
        if (ends_operand(prev) || p == ']' || is_type_keyword(p) || is_call_keyword(p)) return SPACING_NONE;
        if (is_keyword_token(prev)) return SPACING_ONE;
        return SPACING_KEEP;
    }
    if (c == '[') {
        if (ends_operand(prev) || p == ']') return SPACING_NONE;
        if (is_keyword_token(prev)) return SPACING_ONE;
        return SPACING_KEEP;
    }
    if (c == '{') {
        if (p == ')') return SPACING_ONE;
        if (is_keyword_token(prev) && !is_call_keyword(p)) return SPACING_ONE;
        return SPACING_KEEP;
    }

    bool cur_word  = is_literal_token(c) || is_type_keyword(c) || is_keyword_token(cur);
    if (p == ']') {
        if (is_keyword_token(cur)) return SPACING_ONE;
        if (cur_word || c == '?' || c == '(' || c == '[') return SPACING_NONE;
        return SPACING_KEEP;
    }
    bool prev_word = is_literal_token(p) || is_type_keyword(p) || is_keyword_token(prev);
    if (prev_word && cur_word) return SPACING_ONE;
    if (p == ')' && cur_word) return SPACING_ONE;
    if (p == '}' && is_keyword_token(cur)) return SPACING_ONE;

    // The start of an operand after a keyword, e.g return -x or case .a
    if (is_keyword_token(prev) && !is_call_keyword(p)) return SPACING_ONE;
    return SPACING_KEEP;
}

// The elements of the source: its tokens, and the comments between them
static bool format_scan(Formatter *f, Lexer *lexer, const char *text) {
    int capacity = lexer->tokens.count * 2 + 16;
    f->elements = malloc(capacity * sizeof(FormatElement));
    f->count = 0;

    int at = 0;
    int line_start = 0;
    for (int i = 0; i <= lexer->tokens.count; i++) {
        Token *token = i < lexer->tokens.count ? &((Token *)lexer->tokens.items)[i] : NULL;
        if (token && token->type == TOKEN_END) token = NULL;
        int gap_end = token ? token->start.input_idx : (int)strlen(text);

        // The gap: spaces, newlines and comments
        int newlines = 0, spaces = 0;
        int k = at;
        while (k < gap_end) {
            char ch = text[k];
            if (ch == '\n') { newlines++; spaces = 0; line_start = k + 1; k++; continue; }
            if (ch == ' ' || ch == '\t') { spaces++; k++; continue; }
            if (ch == '/' && k + 1 < gap_end && text[k + 1] == '/') {
                int start = k;
                while (k < gap_end && text[k] != '\n') k++;
                int end = k;
                while (end > start && (text[end - 1] == ' ' || text[end - 1] == '\t')) end--;
                if (f->count == capacity) { capacity *= 2; f->elements = realloc(f->elements, capacity * sizeof(FormatElement)); }
                f->elements[f->count++] = (FormatElement){ .kind = FORMAT_COMMENT, .text = text + start, .length = end - start, .newlines = newlines, .spaces = spaces, .column = start - line_start };
                newlines = 0;
                spaces = 0;
                continue;
            }
            return false; // Something the lexer skipped, e.g a character it doesn't know
        }
        if (token == NULL) break;

        if (f->count == capacity) { capacity *= 2; f->elements = realloc(f->elements, capacity * sizeof(FormatElement)); }
        int length = token->end.input_idx - token->start.input_idx;
        const char *start = text + token->start.input_idx;
        f->elements[f->count++] = (FormatElement){
            .kind = FORMAT_TOKEN, .type = token->type, .text = start, .length = length,
            .newlines = newlines, .spaces = spaces, .column = token->start.input_idx - line_start,
            .multiline = memchr(start, '\n', length) != NULL,
        };
        at = token->end.input_idx;
        for (int c = token->start.input_idx; c < token->end.input_idx; c++) {
            if (text[c] == '\n') line_start = c + 1;
        }
    }
    return true;
}

// Writes the elements into lines, with their indentation and the spaces between the tokens
static void format_lines(Formatter *f) {
    FormatLine *line = NULL;
    const FormatElement *prev = NULL;      // On the line
    bool prev_unary = false;
    bool prev_keep  = false;
    bool struct_pending = false;           // A struct or union keyword came, and its { not yet
    bool prev_compare = false;             // prev is a < or > that is a comparison
    const FormatElement *last_code = NULL; // The last token of the code before
    bool last_line_is_attribute = false;   // The line of last_code starts with @ or #
    bool line_starts_with_attribute = false;
    int  line_bracket_count = 0;           // Brackets open at the start of the line
    int  tokens_on_line = 0;
    bool first_is_identifier = false;
    bool first_is_arm = false;
    bool only_names = false;               // The line has only names and commas so far, e.g x, y
    bool first_is_dereference = false;     // e.g *(p as *int) = 1
    bool in_attributes = false;            // The line starts with @, and no name came after the attributes yet

    for (int i = 0; i < f->count; i++) {
        const FormatElement *e = &f->elements[i];
        bool new_line = line == NULL || e->newlines > 0;

        if (new_line) {
            if (line != NULL && e->newlines >= 2) {
                // One blank line at most, and none after { or before }
                bool after_opener = last_code && last_code->type == '{' && tokens_on_line > 0 && prev == last_code;
                bool before_closer = e->kind == FORMAT_TOKEN && e->type == '}';
                if (!after_opener && !before_closer) {
                    FormatLine *blank = format_new_line(f);
                    blank->blank = true;
                }
            }
            if (line != NULL && tokens_on_line > 0) {
                last_line_is_attribute = line_starts_with_attribute;
            }

            int indent = 0;
            if (e->kind == FORMAT_TOKEN && is_closer(e->type) && f->bracket_count > 0) {
                indent = f->brackets[f->bracket_count - 1].open_indent;
            } else {
                FormatBracket *top = f->bracket_count > 0 ? &f->brackets[f->bracket_count - 1] : NULL;
                indent = top ? top->inner_indent : 0;
                bool in_block = top == NULL || top->type == '{';
                bool goes_on = last_code && last_code->type != ';' && last_code->type != '{' && last_code->type != '}' && last_code->type != ',';
                bool is_arm  = e->kind == FORMAT_TOKEN && e->type == TOKEN_CASE;   // e.g the lines of case 1, 2, 3 =>
                if (in_block && goes_on && !last_line_is_attribute && !is_arm) indent++;
            }

            line = format_new_line(f);
            line->indent = indent;
            if (f->bracket_count > 0) {
                FormatBracket *top = &f->brackets[f->bracket_count - 1];
                line->in_struct = top->is_struct;
                line->in_braces = top->type == '{' && !top->is_struct;
            }
            prev = NULL;
            prev_unary = false;
            prev_keep  = false;
            tokens_on_line = 0;
            line_bracket_count = f->bracket_count;
            first_is_identifier = false;
            first_is_arm = false;
            first_is_dereference = false;
            in_attributes = false;
            only_names = true;
            line_starts_with_attribute = false;
        }

        if (e->kind == FORMAT_COMMENT) {
            if (prev != NULL) {
                line->comment_at     = line->length;
                line->comment_column = e->column;
                format_line_append(line, " ", 1);
            } else {
                line->comment_only = true;
            }
            format_line_append(line, e->text, e->length);
            prev = e;
            continue;
        }

        // A token
        bool cur_compare = false;
        bool binary = is_ambiguous_operator(e->type) && prev != NULL && prev->kind == FORMAT_TOKEN &&
                      (ends_operand(prev) || (prev->type == ']' && (e->type == '+' || e->type == '-')));
        if (prev != NULL && prev->kind == FORMAT_TOKEN) {
            TokenType next_type = i + 1 < f->count && f->elements[i + 1].kind == FORMAT_TOKEN && f->elements[i + 1].newlines == 0 ? f->elements[i + 1].type : TOKEN_NONE;
            Spacing spacing = spacing_between(prev, prev_unary, prev_keep, e, binary, next_type, prev_compare);
            // A < or > that is surely a comparison gets a space after it too
            if (e->type == '<' || e->type == '>') cur_compare = spacing == SPACING_ONE;
            bool space = spacing == SPACING_ONE || (spacing == SPACING_KEEP && e->spaces > 0);
            // Tokens that were written together are lexed the same together, but ones that had a space between them
            // may not be, e.g - -x
            if (!space && e->spaces > 0 && would_merge(prev, e)) space = true;

            // Where the spaces of a lined up assignment or constant go, before the operator
            bool top_level = f->bracket_count == line_bracket_count;
            if (line->separator == SEPARATOR_NONE && top_level && (first_is_identifier || (first_is_dereference && is_assignment(e->type))) &&
                (is_assignment(e->type) || e->type == TOKEN_DOUBLE_COLON)) {
                line->separator        = e->type == TOKEN_DOUBLE_COLON ? SEPARATOR_CONSTANT : SEPARATOR_ASSIGNMENT;
                line->separator_token  = e->type;
                line->separator_at     = line->length;
                line->separator_column = e->column;
                line->separator_spaces = e->spaces;
            }
            // The arms of a switch, e.g case .a => 1; or case .a { ... }
            if (line->separator == SEPARATOR_NONE && top_level && first_is_arm && (e->type == TOKEN_FAT_ARROW || e->type == '{')) {
                line->separator        = SEPARATOR_ARM;
                line->separator_token  = e->type;
                line->separator_at     = line->length;
                line->separator_column = e->column;
                line->separator_spaces = e->spaces;
            }
            if (space) format_line_append(line, " ", 1);
        }

        format_line_append(line, e->text, e->length);
        if (e->multiline) line->multiline = true;

        // A declaration lines up after its :, e.g name: Type or x, y: f64
        if (e->type == ':' && only_names && tokens_on_line > 0 && line->separator == SEPARATOR_NONE &&
            i + 1 < f->count && f->elements[i + 1].newlines == 0 && f->elements[i + 1].kind == FORMAT_TOKEN) {
            line->separator        = SEPARATOR_DECLARATION;
            line->separator_token  = ':';
            line->separator_at     = line->length;
            line->separator_column = f->elements[i + 1].column;
            line->separator_spaces = f->elements[i + 1].spaces;
        }
        if (e->type != TOKEN_LITERAL_IDENTIFIER && e->type != ',') only_names = false;

        // After attributes, e.g @private Limit :: 10; or @embed table: Table<T>;
        if (in_attributes && tokens_on_line > 0 && e->type == TOKEN_LITERAL_IDENTIFIER && f->bracket_count == line_bracket_count &&
            prev && prev->type != '@' && prev->type != '.') {
            in_attributes = false;
            first_is_identifier = true;
            only_names = true;
        }
        if (tokens_on_line == 0) {
            first_is_identifier = e->type == TOKEN_LITERAL_IDENTIFIER;
            first_is_dereference = e->type == '*' || e->type == '(';
            in_attributes = e->type == '@';
            line->starts_with_attribute = e->type == '@';
            first_is_arm = e->type == TOKEN_CASE || e->type == TOKEN_ELSE || e->type == TOKEN_IF;
            if (e->type == TOKEN_CASE) line->arm_only = true;
            line_starts_with_attribute = e->type == '@' || e->type == '#';
            if (e->type == '@' && line->separator == SEPARATOR_NONE) line->attribute_only = true;
        }
        tokens_on_line++;

        // Brackets
        if (is_opener(e->type)) {
            FormatBracket bracket = { .type = e->type, .open_indent = line->indent, .inner_indent = line->indent + 1,
                                      .is_struct = e->type == '{' && struct_pending };
            if (e->type == '{') struct_pending = false;
            format_push_bracket(f, bracket);
        } else if (is_closer(e->type) && f->bracket_count > 0) {
            f->bracket_count--;
            if (f->bracket_count < line_bracket_count) line_bracket_count = f->bracket_count;
        }

        // The * of method * (x: int), or of []*int, keeps the space after it, if it had one
        bool after_bracket = (e->type == '*' || e->type == '&') && prev && (prev->type == ']' || prev->type == TOKEN_METHOD);
        prev_keep  = after_bracket;
        prev_unary = !after_bracket && ((is_ambiguous_operator(e->type) && !binary) || e->type == '!' || e->type == '~' || e->type == '?');
        if (e->type == TOKEN_STRUCT || e->type == TOKEN_UNION) struct_pending = true;
        if (e->type == ';') struct_pending = false;
        prev_compare = cur_compare;
        prev = e;
        last_code = e;
    }

    // An attribute line has only @ names and their arguments, and no separator
    for (int i = 0; i < f->line_count; i++) {
        FormatLine *l = &f->lines[i];
        if (l->attribute_only && (l->separator != SEPARATOR_NONE || l->text[l->length - 1] == ';')) l->attribute_only = false;
    }
}

static void format_insert_spaces(FormatLine *l, int at, int count) {
    if (count <= 0) return;
    char spaces[256];
    while (count > 0) {
        int n = count > 256 ? 256 : count;
        memset(spaces, ' ', n);
        format_line_append(l, spaces, n);
        memmove(l->text + at + n, l->text + at, l->length - n - at);
        memcpy(l->text + at, spaces, n);
        count -= n;
    }
}

// Members of a struct (name: Type;) and fields of a struct literal on lines of their own (name: value,) are lined up
// always: the lines one after the other at the same indentation, which comment and attribute lines don't end
static bool is_member_or_field(FormatLine *l) {
    if (l->separator != SEPARATOR_DECLARATION || l->multiline || l->starts_with_attribute) return false;
    if (l->in_struct) return true;
    // In a block, name: Type; declares a variable. A field of a literal ends with a , or is the last one
    return l->in_braces && l->length > 0 && l->text[l->comment_at >= 0 ? l->comment_at - 1 : l->length - 1] != ';';
}

static void format_align_members(Formatter *f) {
    for (int i = 0; i < f->line_count; i++) {
        FormatLine *first = &f->lines[i];
        if (!is_member_or_field(first)) continue;
        int end = i;
        int widest = first->separator_at;
        for (int j = i + 1; j < f->line_count; j++) {
            FormatLine *l = &f->lines[j];
            if (l->blank) break;
            if (l->indent == first->indent && ((l->separator == SEPARATOR_NONE && (l->comment_only || l->attribute_only)) || l->starts_with_attribute)) continue;
            if (!is_member_or_field(l) || l->indent != first->indent || l->in_struct != first->in_struct) break;
            if (l->separator_at > widest) widest = l->separator_at;
            end = j;
        }
        for (int k = i; k <= end; k++) {
            FormatLine *l = &f->lines[k];
            if (!is_member_or_field(l)) continue;
            int pad = widest - l->separator_at;
            format_insert_spaces(l, l->separator_at, pad);
            if (l->comment_at >= l->separator_at) l->comment_at += pad;
            l->separator_at = widest;
            l->aligned = true;
        }
        i = end;
    }
}

// Lines up what was lined up in the source. See the rules at the top
static void format_align(Formatter *f) {
    format_align_members(f);

    // Separators: the lines one after the other that had the same separator in the same column, at the same
    // indentation. Lines between them that don't line anything up don't end such a group: comment and attribute lines,
    // the value of an arm with more values, e.g case 1, and lines whose separator has one space before it
    for (int i = 0; i < f->line_count; i++) {
        FormatLine *first = &f->lines[i];
        if (first->separator == SEPARATOR_NONE || first->multiline || first->aligned) continue;

        int end = i;
        int widest = first->separator_at;
        for (int j = i + 1; j < f->line_count; j++) {
            FormatLine *l = &f->lines[j];
            if (l->blank) break;
            if (l->indent == first->indent && l->separator == SEPARATOR_NONE && (l->comment_only || l->attribute_only || (l->arm_only && first->separator == SEPARATOR_ARM))) continue;
            bool same = l->separator == first->separator && l->separator_token == first->separator_token && l->indent == first->indent &&
                        !l->multiline && l->separator_column == first->separator_column;
            if (!same) {
                if (l->indent == first->indent && l->separator != SEPARATOR_NONE && l->separator_spaces <= 1 && !l->multiline) continue;
                break;
            }
            if (l->separator_at > widest) widest = l->separator_at;
            end = j;
        }
        if (end == i) continue;

        for (int k = i; k <= end; k++) {
            FormatLine *l = &f->lines[k];
            bool member = l->separator == first->separator && l->separator_token == first->separator_token &&
                          l->indent == first->indent && l->separator_column == first->separator_column;
            if (!member) continue;
            int pad = widest - l->separator_at;
            format_insert_spaces(l, l->separator_at, pad);
            if (l->comment_at >= l->separator_at) l->comment_at += pad;
            l->separator_at = widest;
        }
        i = end;
    }

    // Comments at the end of lines stay in their column, unless the code before them is too long. The comments of the
    // lines around that were in the same column move with them
    for (int i = 0; i < f->line_count; i++) {
        FormatLine *first = &f->lines[i];
        if (first->comment_at < 0 || first->multiline) continue;

        int end = i;
        int column = first->comment_column;
        if (first->indent * 4 + first->comment_at + 1 > column) column = first->indent * 4 + first->comment_at + 1;
        for (int j = i + 1; j < f->line_count; j++) {
            FormatLine *l = &f->lines[j];
            if (l->comment_at < 0 || l->multiline || l->comment_column != first->comment_column) break;
            if (l->indent * 4 + l->comment_at + 1 > column) column = l->indent * 4 + l->comment_at + 1;
            end = j;
        }

        for (int k = i; k <= end; k++) {
            FormatLine *l = &f->lines[k];
            int pad = column - (l->indent * 4 + l->comment_at + 1);
            format_insert_spaces(l, l->comment_at + 1, pad);
        }
        i = end;
    }
}

static char *format_join(Formatter *f, int *length) {
    // Without blank lines at the start and the end
    int first = 0, last = f->line_count - 1;
    while (first <= last && f->lines[first].blank) first++;
    while (last >= first && f->lines[last].blank) last--;

    int size = 1;
    for (int i = first; i <= last; i++) size += f->lines[i].indent * 4 + f->lines[i].length + 1;
    char *out = malloc(size);
    int n = 0;
    for (int i = first; i <= last; i++) {
        FormatLine *l = &f->lines[i];
        if (!l->blank && l->length > 0) {
            for (int k = 0; k < l->indent * 4; k++) out[n++] = ' ';
            // No spaces at the end of a line, but those inside a string over more lines are kept
            int len = l->length;
            while (len > 0 && l->text[len - 1] == ' ') len--;
            memcpy(out + n, l->text, len);
            n += len;
        }
        out[n++] = '\n';
    }
    out[n] = '\0';
    *length = n;
    return out;
}

static void format_free(Formatter *f) {
    for (int i = 0; i < f->line_count; i++) free(f->lines[i].text);
    free(f->lines);
    free(f->elements);
    free(f->brackets);
}

// Lexes a text. Gives false when it has characters the lexer doesn't know, or a string that doesn't end
static bool format_lex(Lexer *lexer, const char *path, char *text) {
    *lexer = lexer_init();
    lexer->file_text = text;
    lexer->file_path = (char *)path;
    int muted_before = diagnostics.muted;
    diagnostics.muted++;
    bool ok = lex_file(lexer) && !lexer->had_errors;
    diagnostics.muted = muted_before;
    return ok;
}

static bool same_tokens(Lexer *a, const char *text_a, Lexer *b, const char *text_b) {
    if (a->tokens.count != b->tokens.count) return false;
    for (int i = 0; i < a->tokens.count; i++) {
        Token *x = &((Token *)a->tokens.items)[i];
        Token *y = &((Token *)b->tokens.items)[i];
        if (x->type != y->type) return false;
        int lx = x->end.input_idx - x->start.input_idx;
        int ly = y->end.input_idx - y->start.input_idx;
        if (lx != ly || memcmp(text_a + x->start.input_idx, text_b + y->start.input_idx, lx) != 0) return false;
    }
    return true;
}

// The formatted text of a source, or NULL with the reason in 'error'
char *format_source(const char *path, const char *source, int *length, const char **error) {
    // Without \r of Windows line ends, which are written as \n
    int source_length = (int)strlen(source);
    char *text = malloc(source_length + 1);
    int n = 0;
    for (int i = 0; i < source_length; i++) {
        if (source[i] == '\r' && i + 1 < source_length && source[i + 1] == '\n') continue;
        text[n++] = source[i];
    }
    text[n] = '\0';

    Lexer lexer;
    if (!format_lex(&lexer, path, text)) {
        *error = "it has a syntax error the lexer reports (saad check shows it)";
        free(text);
        return NULL;
    }

    Formatter f = {0};
    if (!format_scan(&f, &lexer, text)) {
        *error = "it has characters that are not a part of Saad";
        format_free(&f);
        free(text);
        return NULL;
    }
    format_lines(&f);
    format_align(&f);
    char *out = format_join(&f, length);
    format_free(&f);

    // The tokens must be the same
    Lexer check;
    if (!format_lex(&check, path, out) || !same_tokens(&lexer, text, &check, out)) {
        *error = "Compiler Error: formatting would change its tokens, so it is left as it is. Please report it";
        free(out);
        free(text);
        return NULL;
    }
    free(text);
    return out;
}

//
// The command: saad fmt [paths] [--check] [--stdout]
//

typedef struct FormatRun {
    bool check;      // Only say which files aren't formatted
    bool to_stdout;  // Write the formatted text instead of changing the file
    int  formatted;  // Files changed, or not formatted with --check
    int  failed;
    int  files;
} FormatRun;

static void format_file(FormatRun *run, const char *path) {
    run->files++;
    char *source = read_entire_file((char *)path);
    if (source == NULL) {
        fprintf(stderr, "%s: can't be read\n", path);
        run->failed++;
        return;
    }
    int length = 0;
    const char *error = NULL;
    char *out = format_source(path, source, &length, &error);
    if (out == NULL) {
        fprintf(stderr, "%s: not formatted, as %s\n", path, error);
        run->failed++;
        if (run->to_stdout) fputs(source, stdout); // An editor gets the text back as it was
        free(source);
        return;
    }

    if (run->to_stdout) {
        fwrite(out, 1, length, stdout);
    } else if (strcmp(out, source) != 0) {
        run->formatted++;
        if (run->check) {
            printf("%s\n", path);
        } else {
            FILE *file = fopen(path, "wb");
            if (file == NULL || fwrite(out, 1, length, file) != (size_t)length) {
                fprintf(stderr, "%s: can't be written\n", path);
                run->failed++;
            } else {
                printf("Formatted %s\n", path);
            }
            if (file) fclose(file);
        }
    }
    free(out);
    free(source);
}

// The .sd files of a folder and the folders in it, but not hidden folders and build
static void format_folder(FormatRun *run, const char *dir, int depth) {
    if (depth > 32) return;
    DIR *d = opendir(dir);
    if (d == NULL) return;

    // In the order of their names, so the output is the same every time
    char **names = NULL;
    int count = 0, capacity = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (strcmp(entry->d_name, "build") == 0) continue;
        if (count == capacity) { capacity = capacity ? capacity * 2 : 32; names = realloc(names, capacity * sizeof(char *)); }
        names[count++] = strdup(entry->d_name);
    }
    closedir(d);
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(names[j], names[i]) < 0) { char *t = names[i]; names[i] = names[j]; names[j] = t; }
        }
    }

    for (int i = 0; i < count; i++) {
        char *path = strcmp(dir, ".") == 0 ? strdup(names[i]) : join_path(dir, names[i]);
        struct stat info;
        if (stat(path, &info) == 0) {
            if (S_ISDIR(info.st_mode)) format_folder(run, path, depth + 1);
            else if (path_has_extension(path, "sd")) format_file(run, path);
        }
        free(path);
        free(names[i]);
    }
    free(names);
}

int format_command(int argc, char const *argv[]) {
    FormatRun run = {0};
    const char *paths[256];
    int path_count = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--check") == 0)  { run.check = true; continue; }
        if (strcmp(argv[i], "--stdout") == 0) { run.to_stdout = true; continue; }
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            printf("Error: Unknown option '%s'\n\n", argv[i]);
            return 2;
        }
        if (path_count < 256) paths[path_count++] = argv[i];
    }
    if (path_count == 0) paths[path_count++] = ".";

    for (int i = 0; i < path_count; i++) {
        struct stat info;
        if (stat(paths[i], &info) != 0) {
            fprintf(stderr, "%s: no such file or folder\n", paths[i]);
            run.failed++;
            continue;
        }
        if (S_ISDIR(info.st_mode)) {
            if (run.to_stdout) {
                fprintf(stderr, "%s: --stdout formats one file, not a folder\n", paths[i]);
                run.failed++;
                continue;
            }
            format_folder(&run, paths[i], 0);
        } else {
            format_file(&run, paths[i]);
        }
    }

    if (run.check && run.formatted > 0) {
        printf("\n%d of %d files are not formatted. saad fmt formats them\n", run.formatted, run.files);
        return 1;
    }
    return run.failed > 0 ? 1 : 0;
}
