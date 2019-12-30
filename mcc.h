#ifndef mcc_h__
#define mcc_h__

#define VERSION     "0.0"
#define MAX_IDENT   256

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef int bool;
#define true    1
#define false   0

bool is_verbose_level(int n);
void set_verbose_level(int n);
void *alloc(size_t size);

void verror(const char *filename, int line, const char *s, va_list arg);
void error(const char *filename, int line, const char *s, ...);

bool init_symtab(void);
void term_symtab(void);

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

typedef enum {
    NK_DUMMY,
} NODE_KIND;

typedef struct node NODE;
struct node {
    NODE_KIND kind;
};

typedef struct {
    SCANNER *scan;
    TOKEN token;
} PARSER;

PARSER *open_parser_text(const char *filename, const char *text);
PARSER *open_parser(const char *filename);
bool close_parser(PARSER *pars);
NODE *parse(PARSER *pars);

bool compile_node(NODE *np);

#endif
