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
extern FILE *g_stdout;
extern FILE *g_stderr;

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
TYPE *dup_type(TYPE *typ);
bool equal_type(const TYPE *tl, const TYPE *tr);

void print_type(const TYPE *typ);


typedef struct node NODE;

typedef enum {
    SK_VAR, SK_FUNC,
} SYMBOL_KIND;

typedef struct symbol SYMBOL;
typedef struct symtab SYMTAB;

struct symbol {
    SYMBOL *next;
    SYMBOL_KIND kind;
    char *id;
    TYPE *type;
    bool has_body;
    NODE *body_node;
    SYMTAB *tab;
};

struct symtab {
    SYMBOL *sym;
    SYMTAB *up;
};

SYMBOL *new_symbol(SYMBOL_KIND kind, char *id, TYPE *type);

bool init_symtab(void);
void term_symtab(void);
SYMTAB *new_symtab(SYMTAB *up);
void enter_function(SYMBOL *sym);
void leave_function(void);
SYMBOL *lookup_symbol_current(const char *id);
SYMBOL *lookup_symbol(const char *id);

void print_symbol(const SYMBOL *sym);
void print_symtab(const SYMTAB *tab);
void print_global_symtab(void);

typedef enum {
    TK_EOF, TK_ID, TK_INT_LIT,
    TK_STATIC, TK_EXTERN, TK_VOID, TK_INT,
    TK_IF, TK_ELSE, TK_WHILE, TK_FOR, TK_CONTINUE, TK_BREAK, TK_RETURN,
    TK_COMMA, TK_SEMI, TK_LPAR, TK_RPAR, TK_BEGIN, TK_END,
    TK_STAR, TK_SLASH, TK_PLUS, TK_MINUS, TK_ASSIGN, TK_LOR, TK_LAND,
    TK_EQ, TK_NEQ, TK_LT, TK_GT, TK_LE, TK_GE, TK_AND, TK_NOT,
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


typedef enum {
    NK_COMPOUND, NK_IF, NK_WHILE, NK_FOR, NK_CONTINUE, NK_BREAK,
    NK_RETURN, NK_EXPR,
    NK_ASSIGN, NK_LOR, NK_LAND,
    NK_EQ, NK_NEQ, NK_LT, NK_GT, NK_LE, NK_GE, NK_ADD, NK_SUB,
    NK_MUL, NK_DIV, NK_ADDR, NK_PTR, NK_MINUS, NK_NOT,
    NK_ID, NK_INT_LIT,
    NK_CALL, NK_ARG,
} NODE_KIND;

struct node {
    NODE_KIND kind;
    union {
        struct {
            NODE *n1;
            NODE *n2;
            NODE *n3;
            NODE *n4;
        } link;
        SYMBOL *sym;
        int num;
    } u;
};

NODE *new_node(NODE_KIND kind);
NODE *new_node1(NODE_KIND kind, NODE *n1);
NODE *new_node2(NODE_KIND kind, NODE *n1, NODE *n2);
NODE *new_node3(NODE_KIND kind, NODE *n1, NODE *n2, NODE *n3);
NODE *new_node4(NODE_KIND kind, NODE *n1, NODE *n2, NODE *n3, NODE *n4);
NODE *link_node(NODE_KIND kind, NODE *node, NODE *top);
NODE *new_node_sym(NODE_KIND kind, SYMBOL *sym);
NODE *new_node_lit(NODE_KIND kind, int num);
void print_node(NODE *np);

typedef struct {
    SCANNER *scan;
    TOKEN token;
} PARSER;

PARSER *open_parser_text(const char *filename, const char *text);
PARSER *open_parser(const char *filename);
bool close_parser(PARSER *pars);
bool parse(PARSER *pars);

bool compile(NODE *np);

#endif
