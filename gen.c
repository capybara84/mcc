#include <assert.h>
#include "mcc.h"


bool compile_node(FILE *fp, const NODE *np)
{
    if (np == NULL) {
        return true;
    }

    switch (np->kind) {
    case NK_LINK:
        compile_node(fp, np->u.link.n1);
        compile_node(fp, np->u.link.n2);
        break;
    case NK_COMPOUND:
        fprintf(fp, "{\n");
        if (np->symtab) {
            fprintf(fp, "local symtab\n");
        }
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "}\n");
        break;
    case NK_IF:
        fprintf(fp, "if (");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ")\n");
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "\n");
        if (np->u.link.n3) {
            fprintf(fp, "else\n");
            compile_node(fp, np->u.link.n3);
        }
        break;
    case NK_WHILE:
        fprintf(fp, "while (");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ")\n");
        compile_node(fp, np->u.link.n2);
        break;
    case NK_FOR:
        fprintf(fp, "for (");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "; ");
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "; ");
        compile_node(fp, np->u.link.n3);
        fprintf(fp, ")\n");
        compile_node(fp, np->u.link.n4);
        break;
    case NK_CONTINUE:
        fprintf(fp, "continue;\n");
        break;
    case NK_BREAK:
        fprintf(fp, "break;\n");
        break;
    case NK_RETURN:
        fprintf(fp, "return ");
        if (np->u.link.n1)
            compile_node(fp, np->u.link.n1);
        fprintf(fp, ";\n");
        break;
    case NK_EXPR:
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ";\n");
        break;
    case NK_ASSIGN:
    case NK_LOR:
    case NK_LAND:
    case NK_EQ:
    case NK_NEQ:
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
        fprintf(fp, "(");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, " %s ", node_kind_to_str(np->kind));
        compile_node(fp, np->u.link.n2);
        fprintf(fp, ")");
        break;
    case NK_ADDR:
    case NK_INDIR:
    case NK_MINUS:
    case NK_NOT:
        fprintf(fp, "(%s", node_kind_to_str(np->kind));
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ")");
        break;
    case NK_ID:
        assert(np->u.sym);
        fprintf(fp, "%s", np->u.sym->id);
        break;
    case NK_INT_LIT:
        fprintf(fp, "%d", np->u.num);
        break;
    case NK_CALL:
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "(");
        compile_node(fp, np->u.link.n2);
        fprintf(fp, ")");
        break;
    case NK_ARG:
        compile_node(fp, np->u.link.n1);
        if (np->u.link.n2) {
            fprintf(fp, ", ");
            compile_node(fp, np->u.link.n2);
        }
        break;
    }

    return true;
}

