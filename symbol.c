#include "mcc.h"

TYPE type_void = { SC_DEFAULT, T_VOID, NULL };
TYPE type_int = { SC_DEFAULT, T_INT, NULL };

TYPE *new_type(TYPE_KIND kind, TYPE *ref_typ)
{
    TYPE *typ = (TYPE*) alloc(sizeof (TYPE));
    typ->sclass = SC_DEFAULT;
    typ->kind = kind;
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

void print_type(const TYPE *typ)
{
    if (typ == NULL)
        printf("NULL");
    else {
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
            print_type(typ->type);
            break;
        case T_FUNC:
            printf("FUNC ");
            print_type(typ->type);
            printf("PARAM ()\n");
            break;
        }
    }
}

bool init_symtab(void)
{
    return true;
}

void term_symtab(void)
{
}
