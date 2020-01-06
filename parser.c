#include <assert.h>
#include "mcc.h"

#define COUNT_OF(array) (sizeof (array) / sizeof (array[0]))

#ifdef NDEBUG
# define ENTER(fn)      ((void) 0)
# define LEAVE(fn)      ((void) 0)
# define TRACE(fn,s)    ((void) 0)
#else
static int s_indent = 0;
# define ENTER(fn)  \
        if (is_verbose_level(3)) printf("%*sENTER %s\n", s_indent++, "", (fn))
# define LEAVE(fn)  \
        if (is_verbose_level(3)) printf("%*sLEAVE %s\n", --s_indent, "", (fn))
# define TRACE(fn, s)  \
        if (is_verbose_level(3)) printf("%*sTRACE %s: %s\n", s_indent, "", \
                                            (fn), (s))
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

static char *get_id(PARSER *pars)
{
    assert(pars->token == TK_ID);
    return pars->scan->id;
}

static int get_int_lit(PARSER *pars)
{
    assert(pars->token == TK_INT_LIT);
    return pars->scan->num;
}

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
    if (!close_scanner(pars->scan))
        return false;
    free(pars);
    return true;
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

static bool is_declaration(PARSER *pars)
{
    return is_declaration_specifier(pars);
}

static bool is_expression(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_AND, TK_STAR, TK_MINUS, TK_NOT, TK_ID,
                    TK_INT_LIT, TK_LPAR};
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_statement(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_BEGIN, TK_IF, TK_WHILE, TK_FOR,
            TK_WHILE, TK_FOR, TK_CONTINUE, TK_BREAK, TK_RETURN };
    if (is_expression(pars))
        return true;
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_unary_operator(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_AND, TK_STAR, TK_MINUS, TK_NOT };
    return is_token_begin_with(pars, begin_with, COUNT_OF(begin_with));
}

static NODE_KIND unary_token_to_node_kind(TOKEN tok)
{
    switch (tok) {
    case TK_AND:    return NK_ADDR;
    case TK_STAR:   return NK_PTR;
    case TK_MINUS:  return NK_MINUS;
    case TK_NOT:    return NK_NOT;
    default:        assert(0);
    }
    return NK_EXPR;
}

static NODE_KIND token_to_node_kind(TOKEN tok)
{
    switch (tok) {
    case TK_EQ:     return NK_EQ;
    case TK_NEQ:    return NK_NEQ;
    case TK_LT:     return NK_LT;
    case TK_GT:     return NK_GT;
    case TK_LE:     return NK_LE;
    case TK_GE:     return NK_GE;
    case TK_PLUS:   return NK_ADD;
    case TK_MINUS:  return NK_SUB;
    case TK_STAR:   return NK_MUL;
    case TK_SLASH:  return NK_DIV;
    default:        assert(0);
    }
    return NK_EXPR;
}

static NODE *parse_expression(PARSER *pars);

/*
primary_expression
    = IDENTIFIER
    | constant
    | '(' expression ')'
constant
    = INTEGER_CONSTANT
*/
static NODE *parse_primary_expression(PARSER *pars)
{
    NODE *np = NULL;

    ENTER("parse_primary_expression");
    switch (pars->token) {
    case TK_ID:
        TRACE("parse_primary_expression", "ID");
        {
            SYMBOL *sym;
            char *id = get_id(pars);
            sym = lookup_symbol(id);
            next(pars);
            if (sym == NULL) {
                parser_error(pars, "undefined symbol '%s'", id);
            }
            np = new_node_sym(NK_ID, sym);
        }
        break;
    case TK_INT_LIT:
        TRACE("parse_primary_expression", "INT_LIT");
        {
            int n = get_int_lit(pars);
            next(pars);
            np = new_node_lit(NK_INT_LIT, n);
        }
        break;
    case TK_LPAR:
        TRACE("parse_primary_expression", "()");
        next(pars);
        np = parse_expression(pars);
        expect(pars, TK_RPAR);
        break;
    default:
        parser_error(pars, "syntax error (expression)");
        break;
    }
    LEAVE("parse_primary_expression");
    return np;
}

static NODE *parse_assignment_expression(PARSER *pars);

/*
argument_expression_list
    = assignment_expression {',' assignment_expression}
*/
static NODE *parse_argument_expression_list(PARSER *pars)
{
    NODE *np;
    ENTER("parse_argument_expression_list");
    np = parse_assignment_expression(pars);
    while (pars->token == TK_COMMA) {
        next(pars);
        np = new_node2(NK_ARG, np, parse_assignment_expression(pars));
    }
    LEAVE("parse_argument_expression_list");
    return np;
}

/*
postfix_expression
    = primary_expression
    | postfix_expression '(' [argument_expression_list] ')'
*/
static NODE *parse_postfix_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_postfix_expression");
    np = parse_primary_expression(pars);
    if (pars->token == TK_LPAR) {
        NODE *a;
        next(pars);
        if (pars->token != TK_RPAR) {
            a = parse_argument_expression_list(pars);
        } else {
            a = NULL;
        }
        expect(pars, TK_RPAR);
        np = new_node2(NK_CALL, np, a);
    }
    LEAVE("parse_postfix_expression");
    return np;
}

/*
unary_expression
    = postfix_expression
    | unary_operator cast_expression
*/
static NODE *parse_unary_expression(PARSER *pars)
{
    NODE *np;
    NODE_KIND kind = NK_EXPR;
    ENTER("parse_unary_expression");
    if (is_unary_operator(pars)) {
        kind = unary_token_to_node_kind(pars->token);
        next(pars);
    }
    np = parse_postfix_expression(pars);
    if (kind != NK_EXPR)
        np = new_node1(kind, np);
    LEAVE("parse_unary_expression");
    return np;
}

/*
multiplicative_expression
	= unary_expression
	| multiplicative_expression '*' cast_expression
	| multiplicative_expression '/' cast_expression
*/
static NODE *parse_multiplicative_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_multiplicative_expression");
    np = parse_unary_expression(pars);
    while (pars->token == TK_STAR || pars->token == TK_SLASH) {
        NODE_KIND kind = token_to_node_kind(pars->token);
        next(pars);
        np = new_node2(kind, np, parse_unary_expression(pars));
    }
    LEAVE("parse_multiplicative_expression");
    return np;
}

/*
additive_expression
	= multiplicative_expression
	| additive_expression '+' multiplicative_expression
	| additive_expression '-' multiplicative_expression
*/
static NODE *parse_additive_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_additive_expression");
    np = parse_multiplicative_expression(pars);
    while (pars->token == TK_PLUS || pars->token == TK_MINUS) {
        NODE_KIND kind = token_to_node_kind(pars->token);
        next(pars);
        np = new_node2(kind, np, parse_multiplicative_expression(pars));
    }
    LEAVE("parse_additive_expression");
    return np;
}

/*
relational_expression
	= additive_expression
	| relational_expression '<' additive_expression
	| relational_expression '>' additive_expression
	| relational_expression '<=' additive_expression
	| relational_expression '>=' additive_expression
*/
static NODE *parse_relational_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_relational_expression");
    np = parse_additive_expression(pars);
    while (pars->token == TK_LT || pars->token == TK_GT ||
            pars->token == TK_LE || pars->token == TK_GE) {
        NODE_KIND kind = token_to_node_kind(pars->token);
        next(pars);
        np = new_node2(kind, np, parse_additive_expression(pars));
    }
    LEAVE("parse_relational_expression");
    return np;
}

/*
equality_expression
	= relational_expression
	| equality_expression '==' relational_expression
	| equality_expression '!=' relational_expression
*/
static NODE *parse_equality_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_equality_expression");
    np = parse_relational_expression(pars);
    while (pars->token == TK_EQ || pars->token == TK_NEQ) {
        NODE_KIND kind = token_to_node_kind(pars->token);
        next(pars);
        np = new_node2(kind, np, parse_relational_expression(pars));
    }
    LEAVE("parse_equality_expression");
    return np;
}

/*
logical_and_expression
	= equality_expression
	| logical_and_expression '&&' inclusive_or_expression
*/
static NODE *parse_logical_and_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_logical_and_expression");
    np = parse_equality_expression(pars);
    while (pars->token == TK_LAND) {
        next(pars);
        np = new_node2(NK_LAND, np, parse_equality_expression(pars));
    }
    LEAVE("parse_logical_and_expression");
    return np;
}

/*
logical_or_expression
	= logical_and_expression
	| logical_or_expression '||' logical_and_expression
*/
static NODE *parse_logical_or_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_logical_or_expression");
    np = parse_logical_and_expression(pars);
    while (pars->token == TK_LOR) {
        next(pars);
        np = new_node2(NK_LOR, np, parse_logical_and_expression(pars));
    }
    LEAVE("parse_logical_or_expression");
    return np;
}


/*
assignment_expression
	= logical_or_expression
	| unary_expression '=' assignment_expression
*/
static NODE *parse_assignment_expression(PARSER *pars)
{
    NODE *np;
    ENTER("parse_assignment_expression");
    np = parse_logical_or_expression(pars);
    if (pars->token == TK_ASSIGN) {
        /* TODO unary_expression (left value) */
        next(pars);
        np = new_node2(NK_ASSIGN, np, parse_assignment_expression(pars));
    }
    LEAVE("parse_assignment_expression");
    return np;
}

/*
expression
	= assignment_expression
*/
static NODE *parse_expression(PARSER *pars)
{
    NODE *np = NULL;
    ENTER("parse_expression");
    np = parse_assignment_expression(pars);
    LEAVE("parse_expression");
    return np;
}

static void parse_declaration(PARSER *pars);
static NODE *parse_statement(PARSER *pars);

/*
compound_statement
	= '{' [declaration_list] {statement} '}'
declaration_list
    = declaration {declaration}
*/
static NODE *parse_compound_statement(PARSER *pars)
{
    NODE *np = NULL;

    ENTER("parse_compound_statement");
    next(pars); /* skip '{' */
    while (is_declaration(pars))
        parse_declaration(pars);
    /*TODO*/
    while (is_statement(pars)) {
        NODE *p = parse_statement(pars);
        np = link_node(NK_COMPOUND, p, np);
    }
    expect(pars, TK_END);
    LEAVE("parse_compound_statement");
    return np;
}

/*
statement
	= expression_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement

expression_statement
	= [expression] ';'

compound_statement
	= '{' [declaration_list] {statement} '}'

selection_statement
	= IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement

iteration_statement
	= WHILE '(' expression ')' statement
	| FOR '(' [expression] ';' [expression] ';' [expression] ')'
		statement

jump_statement
	= CONTINUE ';'
	| BREAK ';'
	| RETURN [expression] ';'

*/
static NODE *parse_statement(PARSER *pars)
{
    NODE *np = NULL;

    ENTER("parse_statement");
    switch (pars->token) {
    case TK_BEGIN:
        TRACE("parse_statement", "compound");
        np = parse_compound_statement(pars);
        break;
    case TK_IF:
        TRACE("parse_statement", "if");
        {
            NODE *c, *s, *e;
            next(pars);
            expect(pars, TK_LPAR);
            c = parse_expression(pars);
            expect(pars, TK_RPAR);
            s = parse_statement(pars);
            if (pars->token == TK_ELSE) {
                next(pars);
                e = parse_statement(pars);
            } else
                e = NULL;
            np = new_node3(NK_IF, c, s, e);
        }
        break;
    case TK_WHILE:
        TRACE("parse_statement", "while");
        {
            NODE *c, *b;
            next(pars);
            expect(pars, TK_LPAR);
            c = parse_expression(pars);
            expect(pars, TK_RPAR);
            b = parse_statement(pars);
            np = new_node2(NK_WHILE, c, b);
        }
        break;
    case TK_FOR:
        TRACE("parse_statement", "for");
        {
            NODE *e1, *e2, *e3, *b;
            next(pars);
            expect(pars, TK_LPAR);
            if (is_expression(pars))
                e1 = parse_expression(pars);
            else
                e1 = NULL;
            expect(pars, TK_SEMI);
            if (is_expression(pars))
                e2 = parse_expression(pars);
            else
                e2 = NULL;
            expect(pars, TK_SEMI);
            if (is_expression(pars))
                e3 = parse_expression(pars);
            else
                e3 = NULL;
            expect(pars, TK_RPAR);
            b = parse_statement(pars);
            np = new_node4(NK_FOR, e1, e2, e3, b);
        }
        break;
    case TK_CONTINUE:
        TRACE("parse_statement", "continue");
        next(pars);
        expect(pars, TK_SEMI);
        np = new_node(NK_CONTINUE);
        break;
    case TK_BREAK:
        TRACE("parse_statement", "break");
        next(pars);
        expect(pars, TK_SEMI);
        np = new_node(NK_BREAK);
        break;
    case TK_RETURN:
        TRACE("parse_statement", "return");
        {
            NODE *e;
            next(pars);
            if (is_expression(pars))
                e = parse_expression(pars);
            else
                e = NULL;
            expect(pars, TK_SEMI);
            np = new_node1(NK_RETURN, e);
        }
        break;
    default:
        TRACE("parse_statement", "expression");
        {
            NODE *e;
            if (pars->token != TK_SEMI)
                e = parse_expression(pars);
            else
                e = NULL;
            expect(pars, TK_SEMI);
            np = new_node1(NK_EXPR, e);
        }
        break;
    }
    LEAVE("parse_statement");
    return np;
}

/*
declarator
	= {'*'} (IDENTIFIER | '(' declarator ')') ['(' ')']
*/
static void parse_declarator(PARSER *pars, TYPE **pptyp, char **id)
{
    TYPE *typ = NULL;

    ENTER("parse_declarator");

    assert(pptyp);
    assert(*pptyp);
    assert(id);

    while (pars->token == TK_STAR) {
        *pptyp = new_type(T_POINTER, (*pptyp)->sclass, *pptyp);
        next(pars);
    }
    if (pars->token == TK_ID) {
        *id = pars->scan->id;
        next(pars);
    } else if (pars->token == TK_LPAR) {
        typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
        next(pars);
        parse_declarator(pars, &typ, id);
        expect(pars, TK_RPAR);
    } else {
        parser_error(pars, "syntax error");
    }
    if (pars->token == TK_LPAR) {
        next(pars);
        expect(pars, TK_RPAR);
        *pptyp = new_type(T_FUNC, (*pptyp)->sclass, *pptyp);
        if (typ) {
            TYPE *p = typ;
            while (p && p->type && p->type->kind != T_UNKNOWN)
                p = p->type;
            if (p->type) {
                if (p->type->kind == T_UNKNOWN) {
                    p->type = *pptyp;
                    *pptyp = typ;
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
        }
    } else if (typ) {
        /*TODO*/
        parser_error(pars, "syntax error (mcc)");
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
static void parse_declaration_specifier(PARSER *pars, TYPE *typ)
{
    ENTER("parse_declaration_specifier");
    switch (pars->token) {
    case TK_STATIC:
        if (typ->sclass != SC_DEFAULT)
            parser_error(pars, "invalid 'static'");
        typ->sclass = SC_STATIC;
        next(pars);
        break;
    case TK_EXTERN:
        if (typ->sclass != SC_DEFAULT)
            parser_error(pars, "invalid 'extern'");
        typ->sclass = SC_EXTERN;
        next(pars);
        break;
    case TK_VOID:
        if (typ->kind != T_UNKNOWN)
            parser_error(pars, "cannot combine 'void'");
        typ->kind = T_VOID;
        next(pars);
        break;
    case TK_INT:
        if (typ->kind != T_UNKNOWN)
            parser_error(pars, "cannot combine 'int'");
        typ->kind = T_INT;
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
static void parse_declaration_specifiers(PARSER *pars, TYPE *typ)
{
    ENTER("parse_declaration_specifiers");
    parse_declaration_specifier(pars, typ);
    while (is_declaration_specifier(pars)) {
        parse_declaration_specifier(pars, typ);
    }
    LEAVE("parse_declaration_specifiers");
}

/*
declaration
	= declaration_specifiers [declarator_list] ';'
declarator_list
    = declarator {',' declarator}
*/
static void parse_declaration(PARSER *pars)
{
    TYPE *typ;

    ENTER("parse_declaration");

    typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
    parse_declaration_specifiers(pars, typ);

    for (;;) {
        char *id;
        TYPE *ntyp;
        SYMBOL_KIND symkind;

        ntyp = dup_type(typ);
        parse_declarator(pars, &ntyp, &id);
printf("local id:%s type:", id);
print_type(ntyp);
printf("\n");
        /* TODO check */

        symkind = (typ->kind == T_FUNC) ? SK_FUNC : SK_VAR;
        new_symbol(symkind, id, typ);

        if (pars->token != TK_COMMA)
            break;
        next(pars);
    }
    expect(pars, TK_SEMI);
    LEAVE("parse_declaration");
}

/*
external_declaration
    = declaration_specifiers declarator ';'
    | declaration_specifiers declarator compound_statement
*/
static bool parse_external_delaration(PARSER *pars)
{
    SYMBOL *sym;
    TYPE *typ;
    char *id;
    SYMBOL_KIND symkind;
    bool already = false;

    ENTER("parse_external_delaration");

    typ = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
    parse_declaration_specifiers(pars, typ);

    parse_declarator(pars, &typ, &id);

    symkind = (typ->kind == T_FUNC) ? SK_FUNC : SK_VAR;

    /* check same symbol */
    {
        SYMBOL *same = lookup_symbol_current(id);
        if (same) {
            if (same->kind == SK_FUNC && symkind == SK_FUNC) {
                if (!equal_type(same->type, typ)) {
                    parser_error(pars, "'%s' type mismatch", id);
                } else {
                    /* TODO check same is def(or decl) */
                    already = true;
                }
            } else if (same->kind != symkind) {
                parser_error(pars, "'%s' different kind of symbol", id);
            } else {
                parser_error(pars, "'%s' duplicated", id);
            }
        }
    }

    if (pars->token == TK_SEMI) {
        if (!already)
            sym = new_symbol(symkind, id, typ);
        next(pars);
    } else if (pars->token == TK_BEGIN) {
        if (symkind != SK_FUNC)
            parser_error(pars, "invalid function syntax");
        if (!already) {
            NODE *body;
            sym = new_symbol(SK_FUNC, id, typ);
            enter_function(sym);
            body = parse_compound_statement(pars);
            leave_function();
            sym->has_body = true;
            sym->body_node = body;
        } else {
            /*TODO error if already defined */
        }
    } else {
        parser_error(pars, "syntax error");
    }
    LEAVE("parse_external_delaration");
    return true;
}

/*
translation_unit
    = {external_declaration}
*/
bool parse(PARSER *pars)
{
    next(pars);
    while (pars->token != TK_EOF) {
         if (!parse_external_delaration(pars))
            return false;
    }
    return true;
}

