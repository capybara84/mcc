#include "mcc.h"

NODE *new_node(NODE_KIND kind)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    return np;
}

NODE *new_node_sym(NODE_KIND kind, SYMBOL *sym, TYPE *typ)
{
    NODE *np = new_node(kind);
    np->u.sym.sym = sym;
    np->u.sym.type = typ;
    return np;
}

NODE *node_link(NODE_KIND kind, NODE *top, NODE *n)
{
    NODE *np = new_node(kind);
    np->u.bin.left = n;
    np->u.bin.right = NULL;
    if (top == NULL) {
        top = np;
    } else {
        NODE *p;
        for (p = top; p->u.bin.right != NULL; p = p->u.bin.right)
            ;
        p->u.bin.right = np;
    }
    return top;
}

void print_node(NODE *np)
{
    if (np == NULL) {
/*
        printf("node NULL\n");
*/
    } else {
        switch (np->kind) {
        case NK_DECL_LINK:
/*
            printf("decl link left:\n");
*/
            print_node(np->u.bin.left);
/*
            printf("decl link right:\n");
*/
            print_node(np->u.bin.right);
            break;
        case NK_FUNC_DECL:
            printf("FUNC DECL %s:", np->u.sym.sym->id);
            print_type(np->u.sym.type);
            printf("\n");
            break;
        case NK_VAR_DECL:
            printf("VAR DECL %s: ", np->u.sym.sym->id);
            print_type(np->u.sym.type);
            printf("\n");
            break;
        }
    }
}

