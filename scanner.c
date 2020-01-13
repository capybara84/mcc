#include <string.h>
#include <ctype.h>
#include "mcc.h"

/* string buffer / number parse buffer */
#define MAX_BUFFER  256

typedef struct ident {
    struct ident *next;
    char *id;
} IDENT;

static IDENT *s_ident_top = NULL;

char* new_ident(const char *s)
{
    IDENT *p = (IDENT*) alloc(sizeof (IDENT));
    p->id = strdup(s);
    p->next = s_ident_top;
    s_ident_top = p;
    return p->id;
}

void print_ident(void)
{
    IDENT *id;
    printf("print ident [\n");
    for (id = s_ident_top; id != NULL; id = id->next)
        printf(" %p:%s\n", id, id->id);
    printf("]\n");
}

char *intern(const char *s)
{
    IDENT *id = s_ident_top;
/*
    print_ident();
*/
    for (; id != NULL; id = id->next)
        if (strcmp(id->id, s) == 0)
            return id->id;
    return new_ident(s);
}

SCANNER *open_scanner_text(const char *filename, const char *text)
{
    SCANNER *s = (SCANNER*) alloc(sizeof (SCANNER));
    s->source = text;
    s->size = strlen(text);
    s->current = 0;
    s->ch = ' ';
    s->pos.line = 1;
    s->pos.filename = filename;
    s->num = 0;
    s->id = NULL;
    return s;
}

SCANNER *open_scanner(const char *filename)
{
    FILE *fp;
    size_t size;
    char *s;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return NULL;

    if (fseek(fp, 0, SEEK_END) != 0)
        return NULL;

    size = ftell(fp);

    if (fseek(fp, 0, SEEK_SET) != 0)
        return NULL;

    s = (char*) alloc((size+1) * sizeof (char));
    if (fread(s, size, 1, fp) != 1) {
        fclose(fp);
        free(s);
        return NULL;
    }
    s[size] = '\0';
    fclose(fp);
    return open_scanner_text(filename, s);
}

bool close_scanner(SCANNER *s)
{
    if (s == NULL)
        return false;
/*
    print_ident();
*/
    free(s);
    return true;
}

static int is_white(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'
        || ch == '\f' || ch == '\v';
}

static int is_alpha(int ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static int is_alnum(int ch)
{
    return is_alpha(ch) || isdigit(ch);
}

static int next_char(SCANNER *scan)
{
    if (scan->ch == '\n')
        scan->pos.line++;
    scan->ch = scan->source[scan->current++];

    if (is_debug("scanner"))
        printf("%s(%d):next_char: '%c'\n",
                scan->pos.filename, scan->pos.line, scan->ch);

    return scan->ch;
}

static TOKEN scan_id(SCANNER *scan)
{
    char buffer[MAX_IDENT+1];
    int i = 0;
    while (is_alnum(scan->ch)) {
        if (i < MAX_IDENT)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    /*TODO keywords */
    if (strcmp(buffer, "static") == 0) return TK_STATIC;
    if (strcmp(buffer, "extern") == 0) return TK_EXTERN;
    if (strcmp(buffer, "void") == 0) return TK_VOID;
    if (strcmp(buffer, "int") == 0) return TK_INT;
    if (strcmp(buffer, "if") == 0) return TK_IF;
    if (strcmp(buffer, "else") == 0) return TK_ELSE;
    if (strcmp(buffer, "while") == 0) return TK_WHILE;
    if (strcmp(buffer, "for") == 0) return TK_FOR;
    if (strcmp(buffer, "continue") == 0) return TK_CONTINUE;
    if (strcmp(buffer, "break") == 0) return TK_BREAK;
    if (strcmp(buffer, "return") == 0) return TK_RETURN;
    scan->id = intern(buffer);
    return TK_ID;
}

static TOKEN scan_num(SCANNER *scan)
{
    char buffer[MAX_BUFFER+1];
    int i = 0;
    while (isdigit(scan->ch)) {
        if (i < MAX_BUFFER)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    scan->num = atoi(buffer);
    /* TODO unsigned, long, float, double */
    return TK_INT_LIT;
}

static void skip_comment(SCANNER *scan)
{
    next_char(scan);    /* skip '*' */
    for (;;) {
        if (scan->ch == EOF) {
            error(&scan->pos, "unterminated comment");
            return;
        }
        if (scan->ch != '*')
            next_char(scan);
        else if (next_char(scan) == '/') {
            next_char(scan);
            break;
        }
    }
}

TOKEN next_token(SCANNER *scan)
{
    for (;;) {
        while (is_white(scan->ch))
            next_char(scan);
        if (scan->ch == '\0')
            return TK_EOF;
        if (is_alpha(scan->ch))
            return scan_id(scan);
        if (isdigit(scan->ch))
            return scan_num(scan);
        switch (scan->ch) {
        case '#':
            /* skip line */
            while (scan->ch != EOF && scan->ch != '\n')
                next_char(scan);
            continue;
        case '/':
            if (next_char(scan) == '*') {
                skip_comment(scan);
                continue;
            }
            return TK_SLASH;
        case ',':   next_char(scan);    return TK_COMMA;
        case ';':   next_char(scan);    return TK_SEMI;
        case '(':   next_char(scan);    return TK_LPAR;
        case ')':   next_char(scan);    return TK_RPAR;
        case '{':   next_char(scan);    return TK_BEGIN;
        case '}':   next_char(scan);    return TK_END;
        case '*':   next_char(scan);    return TK_STAR;
        case '+':   next_char(scan);    return TK_PLUS;
        case '-':   next_char(scan);    return TK_MINUS;
        case '|':
            if (next_char(scan) == '|') {
                next_char(scan);
                return TK_LOR;
            }
            break;
        case '&':
            if (next_char(scan) == '&') {
                next_char(scan);
                return TK_LAND;
            }
            return TK_AND;
        case '=':
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_EQ;
            }
            return TK_ASSIGN;
        case '!':
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_NEQ;
            }
            return TK_NOT;
        case '<':
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_LE;
            }
            return TK_LT;
        case '>':
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_GE;
            }
            return TK_GT;
        default:
            break;
        }
        error(&scan->pos,
            (isprint(scan->ch) ? "illegal character '%c'"
                              : "illegal character (code=%02d)"), scan->ch);
        next_char(scan);
    }
}

const char *token_to_string(TOKEN tk)
{
    switch (tk) {
    case TK_EOF:        return "<EOF>";
    case TK_ID:         return "<ID>";
    case TK_INT_LIT:    return "<INT LIT>";
    case TK_STATIC:     return "static";
    case TK_EXTERN:     return "extern";
    case TK_VOID:       return "void";
    case TK_INT:        return "int";
    case TK_IF:         return "if";
    case TK_ELSE:       return "else";
    case TK_WHILE:      return "while";
    case TK_FOR:        return "for";
    case TK_CONTINUE:   return "continue";
    case TK_BREAK:      return "break";
    case TK_RETURN:     return "return";
    case TK_COMMA:      return ",";
    case TK_SEMI:       return ";";
    case TK_LPAR:       return "(";
    case TK_RPAR:       return ")";
    case TK_BEGIN:      return "{";
    case TK_END:        return "}";
    case TK_STAR:       return "*";
    case TK_SLASH:      return "/";
    case TK_PLUS:       return "+";
    case TK_MINUS:      return "-";
    case TK_ASSIGN:     return "=";
    case TK_LOR:        return "||";
    case TK_LAND:       return "&&";
    case TK_EQ:         return "==";
    case TK_NEQ:        return "!=";
    case TK_LT:         return "<";
    case TK_GT:         return ">";
    case TK_LE:         return "<=";
    case TK_GE:         return ">=";
    case TK_AND:        return "&";
    case TK_NOT:        return "!";
    }
    return "";
}

const char *scan_token_to_string(SCANNER *scan, TOKEN tk)
{
    static char buffer[MAX_BUFFER+1];
    switch (tk) {
    case TK_ID:
        return scan->id;
    case TK_INT_LIT:
        sprintf(buffer, "%d", scan->num);
        return buffer;
    default:
        break;
    }
    return token_to_string(tk);
}

