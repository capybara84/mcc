#include "mcc.h"

NODE *new_node(NODE_KIND kind)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    return np;
}

void print_node(NODE *np)
{
    if (np == NULL) {
        printf("node NULL\n");
    } else {
        switch (np->kind) {
        case NK_DUMMY:
            printf("DUMMY\n");
            break;
        }
    }
}

