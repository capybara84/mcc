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
    SYMBOL *p = (SYMBOL*) alloc(sizeof (SYMBOL));
    p->next = NULL;
    p->kind = kind;
    p->id = id;
    p->type = type;
    return p;
}

SYMTAB *new_symtab(SYMTAB *up)
{
    SYMTAB *tab = (SYMTAB*) alloc(sizeof (SYMTAB));
    tab->sym = NULL;
    tab->up = up;
    return tab;
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
