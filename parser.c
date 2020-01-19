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
        if (is_debug("parser_scope")) \
            printf("%*sENTER %s\n", s_indent++, "", (fn))
# define LEAVE(fn)  \
        if (is_debug("parser_scope")) \
            printf("%*sLEAVE %s\n", --s_indent, "", (fn))
# define TRACE(fn, s)  \
        if (is_debug("parser_scope")) \
            printf("%*sTRACE %s: %s\n", s_indent, "", (fn), (s))
#endif

#ifdef NDEBUG
#define next(pars)  ((pars)->token = next_token((pars)->scan))
#else
static TOKEN next(PARSER *pars)
{
    pars->token = next_token(pars->scan);
    if (is_debug("parser")) {
        printf("%s(%d): %s\n",
            pars->scan->pos.filename, pars->scan->pos.line,
            scan_token_to_string(pars->scan, pars->token));
    }
    return pars->token;
}
#endif

static const POS *get_pos(const PARSER *pars)
{
    return &pars->scan->pos;
}

static POS copy_pos(const PARSER *pars)
{
    return pars->scan->pos;
}

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

static void parser_warning(PARSER *pars, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    vwarning(&pars->scan->pos, s, ap);
    va_end(ap);
}

static void parser_error(PARSER *pars, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(&pars->scan->pos, s, ap);
    va_end(ap);
    exit(1);
}

static bool expect(PARSER *pars, TOKEN tk)
{
    if (pars->token == tk) {
        next(pars);
        return true;
    }
    parser_error(pars,
        "missing token %s", scan_token_to_string(pars->scan, tk));
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
    case TK_STAR:   return NK_INDIR;
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
            if (sym == NULL) {
                parser_error(pars, "undefined symbol '%s'", id);
            }
            np = new_node_sym(NK_ID, get_pos(pars), sym);
            next(pars);
            if (is_debug("parser")) {
                printf("sym:%s:", id);
                print_type(sym->type);
                printf("\n");
            }
        }
        break;
    case TK_INT_LIT:
        TRACE("parse_primary_expression", "INT_LIT");
        {
            int n = get_int_lit(pars);
            np = new_node_int(NK_INT_LIT, get_pos(pars), n);
            next(pars);
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
    POS pos;
    int n_arg = 0;

    ENTER("parse_argument_expression_list");
    pos = copy_pos(pars);
    np = link_arg_node(n_arg++, &pos, parse_assignment_expression(pars), NULL);
    while (pars->token == TK_COMMA) {
        POS pos = copy_pos(pars);
        next(pars);
        np = link_arg_node(n_arg++, &pos,
                        parse_assignment_expression(pars), np);
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
        POS pos = copy_pos(pars);
        next(pars);
        if (pars->token != TK_RPAR) {
            a = parse_argument_expression_list(pars);
        } else {
            a = NULL;
        }
        expect(pars, TK_RPAR);
        if (!type_is_function(np->type))
            parser_error(pars, "not function call");
        np = new_node2(NK_CALL, &pos,
                        get_func_return_type(np->type), np, a);
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
    POS pos = copy_pos(pars);
    ENTER("parse_unary_expression");
    if (is_unary_operator(pars)) {
        kind = unary_token_to_node_kind(pars->token);
        next(pars);
    }
    np = parse_postfix_expression(pars);
    switch (kind) {
    case NK_ADDR:
        if (!node_can_take_addr(np))
            parser_error(pars, "cannot take the address");
        np = new_node1(NK_ADDR, &pos,
                        new_type(T_POINTER, np->type, NULL), np);
        break;
    case NK_INDIR:
        if (!type_is_pointer(np->type))
            parser_error(pars, "cannot indirection");
        np = new_node1(NK_INDIR, &pos,
                        type_indir(np->type), np);
        break;
    case NK_MINUS:
        if (!type_is_int(np->type))
            parser_error(pars, "invalid type to unary");
        np = new_node1(NK_MINUS, &pos,
                        &g_type_int, np);
    case NK_NOT:
        if (!type_is_int(np->type))
            parser_error(pars, "invalid type to unary");
        np = new_node1(NK_NOT, &pos,
                        &g_type_int, np);
        break;
    case NK_EXPR:
        break;
    default:
        assert(0);
    }
    LEAVE("parse_unary_expression");
    return np;
}

static TYPE *type_check_bin(PARSER *pars, NODE_KIND kind, TYPE *lhs, TYPE *rhs)
{
    switch (kind) {
    case NK_MUL:
    case NK_DIV:
        if (type_can_mul_div(lhs, rhs))
            return lhs;
        break;
    case NK_ADD:
        if (type_can_add(lhs, rhs))
            return lhs;
        break;
    case NK_SUB:
        if (type_can_sub(lhs, rhs))
            return lhs;
        break;
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
    case NK_EQ:
    case NK_NEQ:
        if (type_warn_rel(lhs, rhs)) {
            parser_warning(pars, "incompatible pointer types");
            return &g_type_int;
        }
        if (type_can_rel(lhs, rhs))
            return &g_type_int;
        break;
    case NK_LAND:
    case NK_LOR:
        if (type_can_logical(lhs, rhs))
            return &g_type_int;
        parser_error(pars, "type error");
        return &g_type_int;
    case NK_ASSIGN:
        if (type_can_assign(lhs, rhs))
            return lhs;
        if (type_warn_assign(lhs, rhs)) {
            parser_warning(pars, "incompatible pointer types");
            return lhs;
        }
        break;
    default: assert(0);
    }

    if (is_debug("parser")) {
        printf("lhs:"); print_type(lhs); printf("\n");
        printf("rhs:"); print_type(rhs); printf("\n");
    }
    parser_error(pars, "'%s' type mismatch", node_kind_to_str(kind));
    return &g_type_int;
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_unary_expression(pars);
        typ = type_check_bin(pars, kind, np->type, rhs->type);
        np = new_node2(kind, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_multiplicative_expression(pars);
        typ = type_check_bin(pars, kind, np->type, rhs->type);
        np = new_node2(kind, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_additive_expression(pars);
        typ = type_check_bin(pars, kind, np->type, rhs->type);
        np = new_node2(kind, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_relational_expression(pars);
        typ = type_check_bin(pars, kind, np->type, rhs->type);
        np = new_node2(kind, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_equality_expression(pars);
        typ = type_check_bin(pars, NK_LAND, np->type, rhs->type);
        np = new_node2(NK_LAND, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        rhs = parse_logical_and_expression(pars);
        typ = type_check_bin(pars, NK_LOR, np->type, rhs->type);
        np = new_node2(NK_LOR, &pos, typ, np, rhs);
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
        NODE *rhs;
        TYPE *typ;
        POS pos = copy_pos(pars);
        next(pars);
        if (np->kind != NK_ID)
            parser_error(pars, "invalid left value");
        rhs = parse_assignment_expression(pars);
        typ = type_check_bin(pars, NK_ASSIGN, np->type, rhs->type);
        np = new_node2(NK_ASSIGN, &pos, typ, np, rhs);
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

static void parse_declaration(PARSER *pars, int *offset);
static NODE *parse_statement(PARSER *pars);

/*
compound_statement
	= '{' [declaration_list] {statement} '}'
declaration_list
    = declaration {declaration}
*/
static NODE *parse_compound_statement(PARSER *pars, int offset)
{
    NODE *np = new_node(NK_COMPOUND, get_pos(pars), NULL);

    ENTER("parse_compound_statement");

    next(pars); /* skip '{' */
    while (is_declaration(pars))
        parse_declaration(pars, &offset);
    while (is_statement(pars)) {
        NODE *p;
        POS pos = copy_pos(pars);
        p = parse_statement(pars);
        np->u.comp.left = link_node(NK_LINK, &pos, p, np->u.comp.left);
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
    SYMTAB *tab;

    ENTER("parse_statement");
    switch (pars->token) {
    case TK_BEGIN:
        TRACE("parse_statement", "compound");
        tab = enter_scope();
        np = parse_compound_statement(pars, get_func_local_var_size());
        assert(np->kind == NK_COMPOUND);
        np->u.comp.symtab = tab;
        leave_scope();
        break;
    case TK_IF:
        TRACE("parse_statement", "if");
        {
            NODE *c, *s, *e;
            POS pos = copy_pos(pars);
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
            np = new_node3(NK_IF, &pos, NULL, c, s, e);
        }
        break;
    case TK_WHILE:
        TRACE("parse_statement", "while");
        {
            NODE *c, *b;
            POS pos = copy_pos(pars);
            next(pars);
            expect(pars, TK_LPAR);
            c = parse_expression(pars);
            expect(pars, TK_RPAR);
            b = parse_statement(pars);
            np = new_node2(NK_WHILE, &pos, NULL, c, b);
        }
        break;
    case TK_FOR:
        TRACE("parse_statement", "for");
        {
            NODE *e1, *e2, *e3, *b;
            POS pos = copy_pos(pars);
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
            np = new_node4(NK_FOR, &pos, NULL, e1, e2, e3, b);
        }
        break;
    case TK_CONTINUE:
        TRACE("parse_statement", "continue");
        {
            POS pos = copy_pos(pars);
            next(pars);
            expect(pars, TK_SEMI);
            np = new_node(NK_CONTINUE, &pos, NULL);
        }
        break;
    case TK_BREAK:
        TRACE("parse_statement", "break");
        {
            POS pos = copy_pos(pars);
            next(pars);
            expect(pars, TK_SEMI);
            np = new_node(NK_BREAK, &pos, NULL);
        }
        break;
    case TK_RETURN:
        TRACE("parse_statement", "return");
        {
            NODE *e;
            POS pos = copy_pos(pars);
            next(pars);
            if (is_expression(pars))
                e = parse_expression(pars);
            else
                e = NULL;
            expect(pars, TK_SEMI);
            np = new_node1(NK_RETURN, &pos, e ? e->type : NULL, e);
        }
        break;
    default:
        TRACE("parse_statement", "expression");
        {
            NODE *e;
            POS pos = copy_pos(pars);
            if (pars->token != TK_SEMI)
                e = parse_expression(pars);
            else
                e = NULL;
            expect(pars, TK_SEMI);
            np = new_node1(NK_EXPR, &pos, NULL, e);
        }
        break;
    }
    LEAVE("parse_statement");
    return np;
}

static PARAM *parse_parameter_list(PARSER *pars);

/*
param_declarator
	= {'*'} [IDENTIFIER | '(' param_declarator ')']
        ['(' [parameter_list] ')']
*/
static void parse_param_declarator(PARSER *pars, TYPE **pptyp, char **id)
{
    PARAM *param_list = NULL;
    TYPE *typ = NULL;

    ENTER("parse_param_declarator");

    assert(pptyp);
    assert(*pptyp);

    while (pars->token == TK_STAR) {
        *pptyp = new_type(T_POINTER, *pptyp, NULL);
        next(pars);
    }
    if (pars->token == TK_ID) {
        *id = get_id(pars);
        next(pars);
    } else if (pars->token == TK_LPAR) {
        typ = new_type(T_UNKNOWN, NULL, NULL);
        next(pars);
        parse_param_declarator(pars, &typ, id);
        expect(pars, TK_RPAR);
    }
    if (pars->token == TK_LPAR) {
        next(pars);
        if (pars->token != TK_RPAR) {
            param_list = parse_parameter_list(pars);
        }
        expect(pars, TK_RPAR);
        *pptyp = new_type(T_FUNC, *pptyp, param_list);
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

    LEAVE("parse_param_declarator");
}

static void
parse_declaration_specifiers(PARSER *pars, STORAGE_CLASS *sc, TYPE *typ);

/*
parameter_declaration
	= declaration_specifiers [param_declarator]
*/
static TYPE *parse_parameter_declaration(PARSER *pars, char **id)
{
    STORAGE_CLASS sc;
    TYPE *typ;

    ENTER("parse_parameter_declaration");

    typ = new_type(T_UNKNOWN, NULL, NULL);
    sc = SC_DEFAULT;
    parse_declaration_specifiers(pars, &sc, typ);
    /* TODO check sc */

    if (pars->token != TK_COMMA && pars->token != TK_RPAR)
        parse_param_declarator(pars, &typ, id);

    LEAVE("parse_parameter_declaration");
    return typ;
}

/*
parameter_list
	= parameter_declaration {',' parameter_declaration}
*/
static PARAM *parse_parameter_list(PARSER *pars)
{
    PARAM *param_list;
    TYPE *typ;
    char *id = NULL;

    ENTER("parse_parameter_list");
    typ = parse_parameter_declaration(pars, &id);
    param_list = link_param(NULL, typ, id);
    while (pars->token == TK_COMMA) {
        next(pars);
        typ = parse_parameter_declaration(pars, &id);
        param_list = link_param(param_list, typ, id);
    }
    LEAVE("parse_parameter_list");
    return param_list;
}


/*
declarator
	= {'*'} (IDENTIFIER | '(' declarator ')') ['(' [parameter_list] ')']
*/
static PARAM *parse_declarator(PARSER *pars, TYPE **pptyp, char **id)
{
    PARAM *param_list = NULL;
    TYPE *typ = NULL;

    ENTER("parse_declarator");

    assert(pptyp);
    assert(*pptyp);
    assert(id);

    while (pars->token == TK_STAR) {
        *pptyp = new_type(T_POINTER, *pptyp, NULL);
        next(pars);
    }
    if (pars->token == TK_ID) {
        *id = get_id(pars);
        next(pars);
    } else if (pars->token == TK_LPAR) {
        typ = new_type(T_UNKNOWN, NULL, NULL);
        next(pars);
        param_list = parse_declarator(pars, &typ, id);
        expect(pars, TK_RPAR);
    } else {
        parser_error(pars, "syntax error");
    }
    if (pars->token == TK_LPAR) {
        next(pars);
        if (pars->token != TK_RPAR) {
            param_list = parse_parameter_list(pars);
        }
        expect(pars, TK_RPAR);
        *pptyp = new_type(T_FUNC, *pptyp, param_list);
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
    return param_list;
}

/*
declaration_specifier
	= storage_class_specifier | type_specifier
storage_class_specifier
    = STATIC | EXTERN
type_specifier
	= VOID | INT
*/
static void
parse_declaration_specifier(PARSER *pars, STORAGE_CLASS* sc, TYPE *typ)
{
    ENTER("parse_declaration_specifier");
    switch (pars->token) {
    case TK_STATIC:
        if (*sc != SC_DEFAULT)
            parser_error(pars, "invalid 'static'");
        *sc = SC_STATIC;
        next(pars);
        break;
    case TK_EXTERN:
        if (*sc != SC_DEFAULT)
            parser_error(pars, "invalid 'extern'");
        *sc = SC_EXTERN;
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
static void
parse_declaration_specifiers(PARSER *pars, STORAGE_CLASS *sc, TYPE *typ)
{
    ENTER("parse_declaration_specifiers");
    parse_declaration_specifier(pars, sc, typ);
    while (is_declaration_specifier(pars)) {
        parse_declaration_specifier(pars, sc, typ);
    }
    LEAVE("parse_declaration_specifiers");
}

/*
declaration
	= declaration_specifiers [declarator_list] ';'
declarator_list
    = declarator {',' declarator}
*/
static void parse_declaration(PARSER *pars, int *offset)
{
    STORAGE_CLASS sc;
    TYPE *typ;

    ENTER("parse_declaration");

    typ = new_type(T_UNKNOWN, NULL, NULL);
    sc = SC_DEFAULT;
    parse_declaration_specifiers(pars, &sc, typ);

    for (;;) {
        char *id;
        TYPE *ntyp;
        SYMBOL_KIND symkind;
        bool already = false;

        ntyp = dup_type(typ);
        parse_declarator(pars, &ntyp, &id);


        if (is_debug("parser")) {
            printf("local id:%s %d %s type:",
                    id, *offset, get_storage_class_string(sc));
            print_type(ntyp);
            printf("\n");
        }

        symkind = (ntyp->kind == T_FUNC) ? SK_FUNC : SK_VAR;
        if (symkind == SK_FUNC) {
            SYMBOL *same = lookup_symbol(id);
            if (same) {
                if (same->kind != SK_FUNC) {
                    parser_error(pars, "'%s' differenct kind of symbol", id);
                } else if (!equal_type(same->type, ntyp)) {
                    parser_error(pars, "'%s' type mismatch", id);
                } else
                    already = true;
            }
        } else {
            SYMBOL *same = lookup_symbol_local(id);
            if (same) {
                parser_error(pars, "'%s' duplicated", id);
            }
        }
        if (!already) {
            int num = *offset;
            VAR_KIND varkind = (symkind == SK_FUNC) ? VK_UNKNOWN : VK_LOCAL;
            (*offset) += type_size(ntyp);
            set_func_local_var_size(*offset);
            new_symbol(symkind, varkind, sc, id, ntyp, num);
        }

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
    STORAGE_CLASS sc;
    SYMBOL *sym = NULL;
    TYPE *typ;
    char *id;
    SYMBOL_KIND symkind;
    PARAM *param_list;

    ENTER("parse_external_delaration");

    typ = new_type(T_UNKNOWN, NULL, NULL);
    sc = SC_DEFAULT;
    parse_declaration_specifiers(pars, &sc, typ);

    param_list = parse_declarator(pars, &typ, &id);

    symkind = (typ->kind == T_FUNC) ? SK_FUNC : SK_VAR;

    /* check same symbol */
    sym = lookup_symbol(id);
    if (sym) {
        if (sym->kind == SK_FUNC && symkind == SK_FUNC) {
            if (!equal_type(sym->type, typ)) {
                parser_error(pars, "'%s' type mismatch", id);
            }
        } else if (sym->kind != symkind) {
            parser_error(pars, "'%s' different kind of symbol", id);
        } else {
            parser_error(pars, "'%s' duplicated", id);
        }
    }

    if (pars->token == TK_SEMI) {
        if (sym == NULL) {
            VAR_KIND var_kind = (symkind == SK_FUNC) ? VK_UNKNOWN : VK_GLOBAL;
            sym = new_symbol(symkind, var_kind, sc, id, typ, 0);
        }
        next(pars);
    } else if (pars->token == TK_BEGIN) {
        NODE *body;
        PARAM *p;
        int offset, num;
        if (symkind != SK_FUNC)
            parser_error(pars, "invalid function syntax");
        if (sym) {
            if (sym->has_body)
                parser_error(pars, "'%s' redefinition", id);
        } else
            sym = new_symbol(SK_FUNC, VK_UNKNOWN, sc, id, typ, 0);
        sym->tab = enter_function(sym);
        offset = 0;
        num = 0;
        for (p = param_list; p != NULL; p = p->next) {
            SYMBOL *sym = new_symbol(SK_VAR, VK_PARAM, SC_DEFAULT,
                                        p->id, p->type, offset);
            sym->num = num++;
            offset += type_size(p->type);
            if (is_debug("parser")) {
                printf("param id:%s type:", p->id);
                print_type(p->type);
                printf("\n");
            }
        }
        body = parse_compound_statement(pars, 0);
        /*TODO calc local table size */
        sym->has_body = true;
        sym->body_node = body;
        leave_function();
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

