#include <assert.h>
#include <string.h>
#include "mcc.h"

TYPE g_type_int = { T_INT, NULL, NULL };
TYPE g_type_null = { T_NULL, NULL, NULL };

static SYMTAB *global_table = NULL;
static SYMTAB *current_symtab = NULL;


TYPE *new_type(TYPE_KIND kind, TYPE *ref_typ, TYPE *param)
{
    TYPE *typ = (TYPE*) alloc(sizeof (TYPE));
    typ->kind = kind;
    typ->type = ref_typ;
    typ->param = param;
    return typ;
}

TYPE *dup_type(TYPE *tp)
{
    return new_type(tp->kind, tp->type, tp->param);
}

bool equal_type(const TYPE *tl, const TYPE *tr)
{
    if (tl == NULL && tr == NULL)
        return true;
    if (tl == NULL || tr == NULL)
        return false;
    if (tl->kind == T_INT && tr->kind == T_NULL)
        return true;
    if (tl->kind == T_NULL && tr->kind == T_INT)
        return true;
    if (tl->kind != tr->kind)
        return false;
    if (!equal_type(tl->type, tr->type))
        return false;
    if (!equal_type(tl->param, tr->param))
        return false;
    return true;
}

bool type_is_void(const TYPE *typ)
{
    return (typ != NULL && typ->kind == T_VOID);
}

bool type_is_null(const TYPE *typ)
{
    return (typ != NULL && typ->kind == T_NULL);
}

bool type_is_function(const TYPE *typ)
{
    return (typ != NULL && typ->kind == T_FUNC);
}

bool type_is_int(const TYPE *typ)
{
    return (typ != NULL && (typ->kind == T_INT || typ->kind == T_NULL));
}

bool type_is_pointer(const TYPE *typ)
{
    return (typ != NULL && typ->kind == T_POINTER);
}

TYPE *type_indir(TYPE *typ)
{
    assert(type_is_pointer(typ));
    return typ->type;
}

TYPE *get_func_return_type(TYPE *typ)
{
    assert(type_is_function(typ));
    return typ->type;
}

bool type_can_mul_div(const TYPE *lhs, const TYPE *rhs)
{
    return type_is_int(lhs) && type_is_int(rhs);
}

bool type_can_add(const TYPE *lhs, const TYPE *rhs)
{
    if (type_is_int(lhs) && type_is_int(rhs))
        return true;
    if ((type_is_pointer(lhs) || type_is_function(lhs)) && type_is_int(rhs))
        return true;
    else if ((type_is_pointer(rhs) || type_is_function(rhs))
                && type_is_int(lhs))
        return true;
    return false;
}

bool type_can_sub(const TYPE *lhs, const TYPE *rhs)
{
    if (type_is_int(lhs) && type_is_int(rhs))
        return true;
    if ((type_is_pointer(lhs) || type_is_function(lhs)) && type_is_int(rhs))
        return true;
    return false;
}

bool type_warn_rel(const TYPE *lhs, const TYPE *rhs)
{
    if (type_is_pointer(lhs) && type_is_pointer(rhs)
                && !equal_type(lhs, rhs))
        return true;
    if ((type_is_pointer(lhs) && type_is_function(rhs))
            || (type_is_function(lhs) && type_is_pointer(rhs)))
        return true;
    return false;
}

bool type_can_rel(const TYPE *lhs, const TYPE *rhs)
{
    if (type_is_void(lhs) || type_is_void(rhs))
        return false;
    if (equal_type(lhs, rhs))
        return true;
    if ((type_is_pointer(lhs) && type_is_int(rhs))
            || (type_is_int(lhs) && type_is_pointer(rhs)))
        return true;
    return false;
}

bool type_can_logical(const TYPE *lhs, const TYPE *rhs)
{
    return !type_is_void(lhs) && !type_is_void(rhs);
}

bool type_can_assign(const TYPE *lhs, const TYPE *rhs)
{
    if (type_is_void(lhs) || type_is_void(rhs))
        return false;
    if (equal_type(lhs, rhs))
        return true;
    if (type_is_pointer(lhs) && type_is_pointer(rhs))
        return type_can_assign(lhs->type, rhs->type);
    if (type_is_pointer(lhs) && type_is_function(rhs)
            && equal_type(lhs->type, rhs)) {
        return true;
    }
    if (type_is_pointer(lhs) && type_is_null(rhs))
        return true;
    return false;
}

bool type_warn_assign(const TYPE *lhs, const TYPE *rhs)
{
    if ((type_is_pointer(lhs) && type_is_int(rhs))
            || (type_is_pointer(lhs) && type_is_function(rhs))
            || (type_is_int(lhs) && type_is_pointer(rhs))
            || (type_is_int(lhs) && type_is_function(rhs))) {
        return true;
    }
    if (type_is_pointer(lhs) && type_is_pointer(rhs)
            && !equal_type(lhs, rhs)) {
        return true;
    }
    return false;
}


TYPE *link_param(TYPE *top, TYPE *param)
{
    TYPE *tp, *p;

    tp = new_type(T_PARAM, param, NULL);
    if (top == NULL)
        return tp;
    for (p = top; p->param != NULL; p = p->param)
        ;
    p->param =tp;
    return top;
}

void print_type(const TYPE *typ)
{
    if (typ == NULL)
        printf("NULL");
    else {
        switch (typ->kind) {
        case T_UNKNOWN:
            printf("UNKNOWN");
            break;
        case T_VOID:
            printf("void");
            break;
        case T_NULL:
            printf("null");
            break;
        case T_INT:
            printf("int");
            break;
        case T_POINTER:
            printf("POINTER to ");
            print_type(typ->type);
            break;
        case T_FUNC:
            printf("FUNC <");
            print_type(typ->type);
            printf("> (");
            print_type(typ->param);
            printf(")");
            break;
        case T_PARAM:
            print_type(typ->type);
            if (typ->param != NULL)
                printf(", ");
            print_type(typ->param);
            break;
        }
    }
}


SYMBOL *
new_symbol(SYMBOL_KIND kind, STORAGE_CLASS sc, const char *id, TYPE *type)
{
    SYMBOL *p;

    assert(current_symtab);
    p = (SYMBOL*) alloc(sizeof (SYMBOL));
    p->next = current_symtab->sym;
    current_symtab->sym = p;
    p->sclass = sc;
    p->kind = kind;
    p->id = id;
    p->type = type;
    p->has_body = false;
    p->body_node = NULL;
    p->tab = NULL;

    return p;
}

SYMBOL *lookup_symbol_local(const char *id)
{
    SYMBOL *sym;
    for (sym = current_symtab->sym; sym != NULL; sym = sym->next) {
        if (sym->id == id)
            return sym;
    }
    return NULL;
}

SYMBOL *lookup_symbol(const char *id)
{
    SYMTAB *tab;
    SYMBOL *sym;

    for (tab = current_symtab; tab != NULL; tab = tab->up) {
        for (sym = tab->sym; sym != NULL; sym = sym->next) {
            if (sym->id == id)
                return sym;
        }
    }
    return NULL;
}


SYMTAB *new_symtab(SYMTAB *up)
{
    SYMTAB *tab = (SYMTAB*) alloc(sizeof (SYMTAB));
    tab->sym = NULL;
    tab->up = up;
    return tab;
}

SYMTAB *enter_scope(SYMTAB *up)
{
    SYMTAB *tab = new_symtab(up ? up : current_symtab);
    return current_symtab = tab;
}

void leave_scope(void)
{
    assert(current_symtab->up);
    current_symtab = current_symtab->up;
}

bool init_symtab(void)
{
    global_table = new_symtab(NULL);
    current_symtab = global_table;
    return true;
}

void term_symtab(void)
{
}

const char *get_storage_class_string(STORAGE_CLASS sc)
{
    switch (sc) {
    case SC_DEFAULT:    return "DEFAULT";
    case SC_STATIC:     return "STATIC";
    case SC_EXTERN:     return "EXTERN";
    }
    return NULL;
}

static const char *get_kind_string(SYMBOL_KIND kind)
{
    switch (kind) {
    case SK_VAR:    return "VAR";
    case SK_FUNC:   return "FUNC";
    }
    return NULL;
}

void print_symtab_1(const SYMTAB *tab)
{
    const SYMBOL *sym;
    if (tab == NULL)
        return;
    for (sym = tab->sym; sym != NULL; sym = sym->next) {
        print_symbol(sym);
    }
}

void print_symbol(const SYMBOL *sym)
{
    printf("SYM %s (%s) %s:", sym->id, get_kind_string(sym->kind),
        get_storage_class_string(sym->sclass));
    print_type(sym->type);
    printf("\n");
    if (sym->kind == SK_FUNC && sym->has_body) {
        printf("local tab\n");
        print_symtab_1(sym->tab);
        printf("{\n");
        print_node(sym->body_node);
        printf("}\n");
    }
}

void print_symtab(const SYMTAB *tab)
{
    for (; tab != NULL; tab = tab->up) {
        print_symtab_1(tab);
    }
}

void print_global_symtab(void)
{
    print_symtab(global_table);
}

