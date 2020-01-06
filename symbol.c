#include <assert.h>
#include <string.h>
#include "mcc.h"

static SYMTAB *global_table = NULL;
static SYMTAB *current_symtab = NULL;


TYPE *new_type(TYPE_KIND kind, STORAGE_CLASS sclass, TYPE *ref_typ)
{
    TYPE *typ = (TYPE*) alloc(sizeof (TYPE));
    typ->kind = kind;
    typ->sclass = sclass;
    typ->type = ref_typ;
    return typ;
}

TYPE *dup_type(TYPE *tp)
{
    TYPE *np = new_type(T_UNKNOWN, SC_DEFAULT, NULL);
    memcpy(np, tp, sizeof (TYPE));
    return np;
}

bool equal_type(const TYPE *tl, const TYPE *tr)
{
    if (tl == NULL && tr == NULL)
        return true;
    if (tl == NULL || tr == NULL)
        return false;
    if (tl->kind != tr->kind)
        return false;
    if (tl->sclass != tr->sclass)
        return false;
    if (!equal_type(tl->type, tr->type))
        return false;
    return true;
}

static void print_storage_class(STORAGE_CLASS sc)
{
    switch (sc) {
    case SC_DEFAULT:
/*
        printf("default ");
*/
        break;
    case SC_STATIC:
        printf("static ");
        break;
    case SC_EXTERN:
        printf("extern ");
        break;
    }
}

static void print_type_sclass(const TYPE *typ, bool sclass)
{
    if (typ == NULL)
        printf("NULL");
    else {
        if (sclass)
            print_storage_class(typ->sclass);
        switch (typ->kind) {
        case T_UNKNOWN:
            printf("UNKNOWN ");
            break;
        case T_VOID:
            printf("void ");
            break;
        case T_INT:
            printf("int ");
            break;
        case T_POINTER:
            printf("POINTER to ");
            print_type_sclass(typ->type, false);
            break;
        case T_FUNC:
            printf("FUNC <");
            print_type_sclass(typ->type, true);
            printf("> PARAM () ");
            break;
        }
    }
}

void print_type(const TYPE *typ)
{
    print_type_sclass(typ, true);
}


SYMBOL *new_symbol(SYMBOL_KIND kind, char *id, TYPE *type)
{
    SYMBOL *p;

    assert(current_symtab);
    p = (SYMBOL*) alloc(sizeof (SYMBOL));
    p->next = current_symtab->sym;
    current_symtab->sym = p;
    p->kind = kind;
    p->id = id;
    p->type = type;
    p->body = NULL;
    return p;
}

SYMBOL *lookup_symbol_current(const char *id)
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

void enter_function(SYMBOL *sym)
{
    sym->tab = new_symtab(current_symtab);
    current_symtab = sym->tab;
}

void leave_function(void)
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

const char *get_kind_string(SYMBOL_KIND kind)
{
    switch (kind) {
    case SK_VAR:    return "VAR";
    case SK_FUNC:   return "FUNC";
    default:
        assert(0);
    }
    return "";
}

void print_symtab_1(const SYMTAB *tab)
{
    const SYMBOL *sym;
    for (sym = tab->sym; sym != NULL; sym = sym->next) {
        print_symbol(sym);
    }
}

void print_symbol(const SYMBOL *sym)
{
    printf("SYM %s (%s):", sym->id, get_kind_string(sym->kind));
    print_type(sym->type);
    printf("\n");
    if (sym->kind == SK_FUNC) {
        printf("local tab\n");
        print_symtab_1(sym->tab);
        printf("{\n");
        print_node(sym->body);
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

