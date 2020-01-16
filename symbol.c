#include <assert.h>
#include <string.h>
#include "mcc.h"

TYPE g_type_int = { T_INT, NULL, NULL };
TYPE g_type_null = { T_NULL, NULL, NULL };

static SYMTAB *global_table = NULL;
static SYMTAB *current_symtab = NULL;
static SYMBOL *current_function = NULL;

TYPE *new_type(TYPE_KIND kind, TYPE *ref_typ, PARAM *param)
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
    {
        PARAM *pl = tl->param;
        PARAM *pr = tr->param;
        while (pl != NULL && pr != NULL) {
            if (!equal_type(pl->type, pr->type))
                return false;
            pl = pl->next;
            pr = pr->next;
        }
        if (pl != NULL || pr != NULL)
            return false;
    }
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

PARAM *link_param(PARAM *top, TYPE *typ, char *id)
{
    PARAM *param = (PARAM*) alloc(sizeof (PARAM));
    PARAM *p;
    param->next = NULL;
    param->id = id;
    param->type = typ;
    if (top == NULL)
        return param;
    for (p = top; p->next != NULL; p = p->next)
        ;
    p->next = param;
    return top;
}


void fprint_type(FILE *fp, const TYPE *typ)
{
    PARAM *p;
    if (typ == NULL) {
    } else {
        switch (typ->kind) {
        case T_UNKNOWN:
            fprintf(fp, "UNKNOWN");
            break;
        case T_VOID:
            fprintf(fp, "void");
            break;
        case T_NULL:
            fprintf(fp, "null");
            break;
        case T_INT:
            fprintf(fp, "int");
            break;
        case T_POINTER:
            fprintf(fp, "POINTER to ");
            fprint_type(fp, typ->type);
            break;
        case T_FUNC:
            fprintf(fp, "FUNC <");
            fprint_type(fp, typ->type);
            fprintf(fp, "> (");
            for (p = typ->param; p != NULL; p = p->next) {
                fprint_type(fp, p->type);
                if (p->next != NULL)
                    fprintf(fp, ", ");
            }
            fprintf(fp, ")");
            break;
        }
    }
}

void print_type(const TYPE *typ)
{
    fprint_type(stdout, typ);
}

/* LP64 */
int type_size(const TYPE *typ)
{
    if (typ == NULL)
        return 0;
    switch (typ->kind) {
    case T_UNKNOWN:
        assert(0);
        break;
    case T_VOID:
        assert(0);
        break;
    case T_NULL:
        return 8;
    case T_INT:
        return 4;
    case T_POINTER:
        return 8;
    case T_FUNC:
        return 8;
    }
    return 0;
}



SYMBOL *
new_symbol(SYMBOL_KIND kind, VAR_KIND var_kind, STORAGE_CLASS sc, const char *id,
            TYPE *type, int offset)
{
    SYMBOL *p;

    assert(current_symtab);
    p = (SYMBOL*) alloc(sizeof (SYMBOL));
    p->next = current_symtab->sym;
    current_symtab->sym = p;
    p->sclass = sc;
    p->kind = kind;
    p->var_kind = var_kind;
    p->id = id;
    p->type = type;
    p->has_body = false;
    p->body_node = NULL;
    p->tab = NULL;
    p->offset = offset;
    p->num = 0;
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

SYMTAB *enter_scope(void)
{
    SYMTAB *tab = new_symtab(current_symtab);
    return current_symtab = tab;
}

void leave_scope(void)
{
    assert(current_symtab->up);
    current_symtab = current_symtab->up;
}

SYMTAB *enter_function(SYMBOL *sym)
{
    current_function = sym;
    return enter_scope();
}

void leave_function(void)
{
    leave_scope();
    current_function = NULL;
}

int get_func_local_var_size(void)
{
    assert(current_function);
    return current_function->offset;
}

void set_func_local_var_size(int size)
{
    assert(current_function);
    current_function->offset = size;
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

static const char *get_var_kind_string(VAR_KIND var_kind)
{
    switch (var_kind) {
    case VK_UNKNOWN:    return "UNKNOWN";
    case VK_GLOBAL:     return "GLOBAL";
    case VK_LOCAL:      return "LOCAL";
    case VK_PARAM:      return "PARAM";
    }
    return NULL;
}

void fprint_symbol(FILE *fp, int indent, const SYMBOL *sym)
{
    fprintf(fp, "%*sSYM %s %s %s(%d) %s:", indent, "",
        sym->id, get_kind_string(sym->kind),
        get_var_kind_string(sym->var_kind),
        sym->offset, get_storage_class_string(sym->sclass));
    fprint_type(fp, sym->type);
    fprintf(fp, "\n");
    if (sym->kind == SK_FUNC && sym->has_body) {
        indent += 2;
        fprintf(fp, "%*slocal tab\n", indent, "");
        fprint_symtab_1(fp, indent, sym->tab);
        fprint_node(fp, indent, sym->body_node);
    }
}

void fprint_symtab_1(FILE *fp, int indent, const SYMTAB *tab)
{
    const SYMBOL *sym;
    if (tab == NULL)
        return;
    for (sym = tab->sym; sym != NULL; sym = sym->next) {
        fprint_symbol(fp, indent, sym);
    }
}

void print_symtab(const SYMTAB *tab)
{
    for (; tab != NULL; tab = tab->up) {
        fprint_symtab_1(stdout, 0, tab);
    }
}

void print_global_symtab(void)
{
    print_symtab(global_table);
}

int calc_arg_size(const SYMBOL *sym)
{
    return 6;   /*TODO*/
}

bool compile_symtab(FILE *fp, const SYMTAB *tab)
{
    const SYMBOL *sym;
    if (tab == NULL)
        return true;
    for (sym = tab->sym; sym != NULL; sym = sym->next) {
        if (sym->kind == SK_VAR && !compile_symbol(fp, sym))
            return false;
    }
    for (sym = tab->sym; sym != NULL; sym = sym->next) {
        if (sym->kind == SK_FUNC && !compile_symbol(fp, sym))
            return false;
    }
    return true;
}

bool compile_all(FILE *fp)
{
    gen_header(fp);
    return compile_symtab(fp, global_table);
}
