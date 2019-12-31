#include "mcc.h"

TYPE *new_type(TYPE_KIND kind, STORAGE_CLASS sclass, TYPE *ref_typ)
{
    TYPE *typ = (TYPE*) alloc(sizeof (TYPE));
    typ->kind = kind;
    typ->sclass = sclass;
    typ->type = ref_typ;
    return typ;
}

static void print_storage_class(STORAGE_CLASS sc)
{
    switch (sc) {
    case SC_DEFAULT:
        printf("default ");
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

bool init_symtab(void)
{
    return true;
}

void term_symtab(void)
{
}
