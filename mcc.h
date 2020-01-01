#ifndef mcc_h__
#define mcc_h__

#define VERSION     "0.0"
#define MAX_IDENT   256

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

typedef int bool;
#define true    1
#define false   0

extern jmp_buf g_error_jmp_buf;

bool is_verbose_level(int n);
void set_verbose_level(int n);
void *alloc(size_t size);

void verror(const char *filename, int line, const char *s, va_list arg);
void error(const char *filename, int line, const char *s, ...);

typedef enum {
    SC_DEFAULT, SC_STATIC, SC_EXTERN
} STORAGE_CLASS;

typedef enum {
    T_UNKNOWN, T_VOID, T_INT, T_POINTER, T_FUNC,
} TYPE_KIND;

typedef struct type TYPE;

struct type {
    TYPE_KIND kind;
    STORAGE_CLASS sclass;
    TYPE *type;
};

TYPE *new_type(TYPE_KIND kind, STORAGE_CLASS sclass, TYPE *typ);

void print_type(const TYPE *typ);

typedef enum {
    SK_VAR, SK_FUNC,
} SYMBOL_KIND;

typedef struct symbol SYMBOL;

struct symbol {
    SYMBOL *next;
    SYMBOL_KIND kind;
    char *id;
    TYPE *type;
};

typedef struct symtab SYMTAB;

struct symtab {
    SYMBOL *sym;
    SYMTAB *up;
};

SYMBOL *new_symbol(SYMBOL_KIND kind, char *id, TYPE *type);

bool init_symtab(void);
void term_symtab(void);
SYMTAB *new_symtab(SYMTAB *up);

typedef enum {
    TK_EOF, TK_ID, TK_INT_LIT,
    TK_STATIC, TK_EXTERN, TK_VOID, TK_INT,
    TK_SEMI, TK_LPAR, TK_RPAR, TK_BEGIN, TK_END,
    TK_STAR,
} TOKEN;

typedef struct {
    const char *source;
    int size;
    int pos;
    int ch;
    int line;
    const char *filename;
    int num;
    char *id;
} SCANNER;

SCANNER *open_scanner_text(const char *filename, const char *text);
SCANNER *open_scanner(const char *filename);
bool close_scanner(SCANNER *scan);
TOKEN next_token(SCANNER *scan);
char *intern(const char *s);
const char *token_to_string(TOKEN tk);
const char *scan_token_to_string(SCANNER *scan, TOKEN tk);

typedef struct symbol SYMBOL;

typedef enum {
    NK_DECL_LINK, NK_FUNC_DECL, NK_VAR_DECL,
} NODE_KIND;

typedef struct node NODE;
struct node {
    NODE_KIND kind;
    union {
        struct {
            NODE *left;
            NODE *right;
        } bin;
        struct {
            SYMBOL *sym;
            TYPE *type;
        } sym;
    } u;
};

NODE *new_node_sym(NODE_KIND kind, SYMBOL *sym, TYPE *typ);
NODE *node_link(NODE_KIND kind, NODE *top, NODE *n);
void print_node(NODE *np);

typedef struct {
    SCANNER *scan;
    TOKEN token;
} PARSER;

PARSER *open_parser_text(const char *filename, const char *text);
PARSER *open_parser(const char *filename);
bool close_parser(PARSER *pars);
NODE *parse(PARSER *pars);

bool compile(NODE *np);

#endif
