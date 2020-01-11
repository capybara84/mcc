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

void vwarning(const char *filename, int line, const char *s, va_list arg);
void verror(const char *filename, int line, const char *s, va_list arg);
void error(const char *filename, int line, const char *s, ...);

typedef enum {
    T_UNKNOWN, T_VOID, T_NULL, T_INT, T_POINTER, T_FUNC
} TYPE_KIND;

typedef struct type TYPE;

typedef struct param PARAM;

struct type {
    TYPE_KIND kind;
    TYPE *type;
    PARAM *param;
};

struct param {
    PARAM *next;
    char *id;
    TYPE *type;
};

extern TYPE g_type_int;
extern TYPE g_type_null;

TYPE *new_type(TYPE_KIND kind, TYPE *typ, PARAM *param);
TYPE *dup_type(TYPE *typ);
bool equal_type(const TYPE *tl, const TYPE *tr);
bool type_is_void(const TYPE *typ);
bool type_is_null(const TYPE *typ);
bool type_is_function(const TYPE *typ);
bool type_is_int(const TYPE *typ);
bool type_is_pointer(const TYPE *typ);
TYPE *type_indir(TYPE *typ);
TYPE *get_func_return_type(TYPE *typ);
bool type_can_mul_div(const TYPE *lhs, const TYPE *rhs);
bool type_can_add(const TYPE *lhs, const TYPE *rhs);
bool type_can_sub(const TYPE *lhs, const TYPE *rhs);
bool type_warn_rel(const TYPE *lhs, const TYPE *rhs);
bool type_can_rel(const TYPE *lhs, const TYPE *rhs);
bool type_can_logical(const TYPE *lhs, const TYPE *rhs);
bool type_can_assign(const TYPE *lhs, const TYPE *rhs);
bool type_warn_assign(const TYPE *lhs, const TYPE *rhs);
PARAM *link_param(PARAM *top, TYPE *typ, char *id);
void fprint_type(FILE *fp, const TYPE *typ);
void print_type(const TYPE *typ);


typedef struct node NODE;

typedef enum {
    SC_DEFAULT, SC_STATIC, SC_EXTERN
} STORAGE_CLASS;

typedef enum {
    SK_VAR, SK_FUNC,
} SYMBOL_KIND;

typedef struct symbol SYMBOL;
typedef struct symtab SYMTAB;

struct symbol {
    SYMBOL *next;
    STORAGE_CLASS sclass;
    SYMBOL_KIND kind;
    const char *id;
    TYPE *type;
    bool has_body;
    NODE *body_node;
    SYMTAB *tab;
    int var_num;
};

struct symtab {
    SYMBOL *sym;
    SYMTAB *up;
};

SYMBOL *new_symbol(SYMBOL_KIND kind, STORAGE_CLASS sc,
                    const char *id, TYPE *type, int var_num);
SYMBOL *lookup_symbol_local(const char *id);
SYMBOL *lookup_symbol(const char *id);
SYMTAB *new_symtab(SYMTAB *up);
SYMTAB *enter_scope(void);
void leave_scope(void);
SYMTAB *enter_function(SYMBOL *sym);
void leave_function(void);
int get_func_var_num(void);
bool init_symtab(void);
void term_symtab(void);

const char *get_storage_class_string(STORAGE_CLASS sc);
void print_symbol(const SYMBOL *sym);
void fprint_symtab_1(FILE *fp, const SYMTAB *tab);
void print_symtab_1(const SYMTAB *tab);
void print_symtab(const SYMTAB *tab);
void print_global_symtab(void);

bool compile_all(FILE *fp);

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
    NK_LINK,
    NK_COMPOUND, NK_IF, NK_WHILE, NK_FOR, NK_CONTINUE, NK_BREAK,
    NK_RETURN, NK_EXPR,
    NK_ASSIGN, NK_LOR, NK_LAND,
    NK_EQ, NK_NEQ, NK_LT, NK_GT, NK_LE, NK_GE, NK_ADD, NK_SUB,
    NK_MUL, NK_DIV, NK_ADDR, NK_INDIR, NK_MINUS, NK_NOT,
    NK_ID, NK_INT_LIT,
    NK_CALL, NK_ARG,
} NODE_KIND;

struct node {
    NODE_KIND kind;
    TYPE *type;
    SYMTAB *symtab; /*TODO move inside union */
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

NODE *new_node(NODE_KIND kind, TYPE *typ);
NODE *new_node1(NODE_KIND kind, TYPE *typ, NODE *n1);
NODE *new_node2(NODE_KIND kind, TYPE *typ, NODE *n1, NODE *n2);
NODE *new_node3(NODE_KIND kind, TYPE *typ, NODE *n1, NODE *n2, NODE *n3);
NODE *new_node4(NODE_KIND kind, TYPE *typ,
                    NODE *n1, NODE *n2, NODE *n3, NODE *n4);
NODE *link_node(NODE_KIND kind, NODE *node, NODE *top);
NODE *new_node_sym(NODE_KIND kind, SYMBOL *sym);
NODE *new_node_int(NODE_KIND kind, int num);
const char *node_kind_to_str(NODE_KIND kind);
bool node_can_take_addr(const NODE *np);
void fprint_node(FILE *fp, const NODE *np);
void print_node(const NODE *np);

typedef struct {
    SCANNER *scan;
    TOKEN token;
} PARSER;

PARSER *open_parser_text(const char *filename, const char *text);
PARSER *open_parser(const char *filename);
bool close_parser(PARSER *pars);
bool parse(PARSER *pars);

void gen_header(FILE *fp);
bool compile_node(FILE *fp, const NODE *np);
bool compile_symbol(FILE *fp, const SYMBOL *sym);


#endif
