#include "mcc.h"

#define COUNT_OF(array) (sizeof (array) / sizeof (array[0]))

#ifdef NDEBUG
# define ENTER(fn)      ((void) 0)
# define LEAVE(fn)      ((void) 0)
# define TRACE(fn)      ((void) 0)
#else
static int s_indent = 0;
# define ENTER(fn)  \
        if (is_verbose_level(3)) printf("%*sENTER %s\n", s_indent++, "", (fn))
# define LEAVE(fn)  \
        if (is_verbose_level(3)) printf("%*sLEAVE %s\n", --s_indent, "", (fn))
# define TRACE(fn)  \
        if (is_verbose_level(3)) printf("%*sTRACE %s\n", s_indent, "", (fn))
#endif

#ifdef NDEBUG
#define next(pars)  ((pars)->token = next_token((pars)->scan))
#else
static TOKEN next(PARSER *pars)
{
    pars->token = next_token(pars->scan);
    if (is_verbose_level(2)) {
        printf("%s(%d): %s\n", pars->scan->filename, pars->scan->line,
            scan_token_to_string(pars->scan, pars->token));
    }
    return pars->token;
}
#endif

PARSER *open_parser_text(const char *filename, const char *text)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner_text(filename, text);
    if (pars->scan == NULL) {
        close_parser(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

PARSER *open_parser(const char *filename)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner(filename);
    if (pars->scan == NULL) {
        close_parser(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

bool close_parser(PARSER *pars)
{
    if (pars == NULL)
        return false;
    return close_scanner(pars->scan);
}

static void parser_error(PARSER *pars, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(pars->scan->filename, pars->scan->line, s, ap);
    va_end(ap);
    exit(1);
}

static bool expect(PARSER *pars, TOKEN tk)
{
    if (pars->token == tk) {
        next(pars);
        return true;
    }
    parser_error(pars, "missing token %s", scan_token_to_string(pars->scan, tk));
    return false;
}


static bool is_token_begin_with(PARSER *pars, TOKEN array[], int count)
{
    int i;
    for (i = 0; i < count; i++)
        if (pars->token == array[i])
            return true;
    return false;
}

static bool is_declaration_specifier(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_STATIC, TK_EXTERN,
            TK_VOID, TK_INT, };
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}


/*
compound_statement
	= '{' '}'
*/
static void parse_compound_statement(PARSER *pars)
{
    ENTER("parse_compound_statement");
    next(pars); /* skip '{' */
    expect(pars, TK_END);
    LEAVE("parse_compound_statement");
}

/*
declarator
	= {'*'} (IDENTIFIER | '(' declarator ')') ['(' ')']
*/
static void parse_declarator(PARSER *pars)
{
    ENTER("parse_declarator");
    while (pars->token == TK_STAR) {
        next(pars);
    }
    if (pars->token == TK_ID) {
        next(pars);
    } else if (pars->token == TK_LPAR) {
        next(pars);
        parse_declarator(pars);
        expect(pars, TK_RPAR);
    } else
        parser_error(pars, "syntax error");
    if (pars->token == TK_LPAR) {
        next(pars);
        expect(pars, TK_RPAR);
    }
    LEAVE("parse_declarator");
}

/*
declaration_specifier
	= storage_class_specifier | type_specifier
storage_class_specifier
    = STATIC | EXTERN
type_specifier
	= VOID | INT
*/
static void parse_declaration_specifier(PARSER *pars)
{
    ENTER("parse_declaration_specifier");
    switch (pars->token) {
    case TK_STATIC:
    case TK_EXTERN:
    case TK_VOID:
    case TK_INT:
        next(pars);
        break;
    default:
        parser_error(pars, "syntax error");
    }
    LEAVE("parse_declaration_specifier");
}


/*
declaration_specifiers
    = declaration_specifier {declaration_specifier}
*/
static void parse_declaration_specifiers(PARSER *pars)
{
    ENTER("parse_declaration_specifiers");
    parse_declaration_specifier(pars);
    while (is_declaration_specifier(pars)) {
        parse_declaration_specifier(pars);
    }
    LEAVE("parse_declaration_specifiers");
}

/*
external_declaration
    = declaration_specifiers declarator ';'
    | declaration_specifiers declarator compound_statement
*/
static void parse_external_delaration(PARSER *pars)
{
    ENTER("parse_external_delaration");
    parse_declaration_specifiers(pars);
    parse_declarator(pars);
    if (pars->token == TK_SEMI) {
        next(pars);
    } else if (pars->token == TK_BEGIN) {
        parse_compound_statement(pars);
    } else
        parser_error(pars, "syntax error");
    LEAVE("parse_external_delaration");
}

/*
translation_unit
    = {external_declaration}
*/
NODE *parse(PARSER *pars)
{
    NODE *np = NULL;

    next(pars);
    while (pars->token != TK_EOF) {
        parse_external_delaration(pars);
    }
    return np;
}

