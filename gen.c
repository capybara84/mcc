#include <assert.h>
#include "mcc.h"


void gen_lval(FILE *fp, const NODE *np)
{
    assert(np->kind == NK_ID);
/*
    if (np->kind != NK_ID)
        error("invalid left value (not variable)");
*/
    assert(np->u.sym);
    fprintf(fp, "    mov rax, rbp\n");
    fprintf(fp, "    sub rax, %d   ; %s\n", np->u.sym->var_num * 8, np->u.sym->id);
    fprintf(fp, "    push rax\n");
}

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
        if (np->symtab) {
            fprintf(fp, "; local symtab\n");
        }
        compile_node(fp, np->u.link.n1);
        break;
    case NK_IF:
        fprintf(fp, ";if (\n");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ";)\n");
        compile_node(fp, np->u.link.n2);
        if (np->u.link.n3) {
            fprintf(fp, ";else\n");
            compile_node(fp, np->u.link.n3);
        }
        break;
    case NK_WHILE:
        fprintf(fp, ";while (\n");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ";)\n");
        compile_node(fp, np->u.link.n2);
        break;
    case NK_FOR:
        fprintf(fp, ";for (\n");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ";\n");
        compile_node(fp, np->u.link.n2);
        fprintf(fp, ";\n");
        compile_node(fp, np->u.link.n3);
        fprintf(fp, ";)\n");
        compile_node(fp, np->u.link.n4);
        break;
    case NK_CONTINUE:
        fprintf(fp, ";continue;\n");
        break;
    case NK_BREAK:
        fprintf(fp, ";break;\n");
        break;
    case NK_RETURN:
        fprintf(fp, ";return\n");
        if (np->u.link.n1) {
            compile_node(fp, np->u.link.n1);
            fprintf(fp, "    pop eax\n");
        }
        break;
    case NK_EXPR:
        compile_node(fp, np->u.link.n1);
        break;
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
    case NK_EQ:
    case NK_NEQ:
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
        compile_node(fp, np->u.link.n1);
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "    pop rdi\n");
        fprintf(fp, "    pop rax\n");
        switch (np->kind) {
        case NK_ADD:
            fprintf(fp, "    add rax, rdi\n");
            break;
        case NK_SUB:
            fprintf(fp, "    sub rax, rdi\n");
            break;
        case NK_MUL:
            fprintf(fp, "    imul rax, rdi\n");
            break;
        case NK_DIV:
            fprintf(fp, "    cqo\n");
            fprintf(fp, "    idiv rdi\n");
            break;
        case NK_EQ:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    sete al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        case NK_NEQ:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    setne al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        case NK_LT:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    setl al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        case NK_GT:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    setg al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        case NK_LE:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    setle al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        case NK_GE:
            fprintf(fp, "    cmp rax, rdi\n");
            fprintf(fp, "    setge al\n");
            fprintf(fp, "    movzb rax, al\n");
            break;
        default: assert(0);
        }
        fprintf(fp, "    push rax\n");
        break;
    case NK_ASSIGN:
        gen_lval(fp, np->u.link.n1);
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "    pop rdi\n");
        fprintf(fp, "    pop rax\n");
        fprintf(fp, "    mov [rax], rdi\n");
        fprintf(fp, "    push rdi\n");
        break;
    case NK_LOR:
        /*TODO*/
    case NK_LAND:
        /*TODO*/
        /*
        fprintf(fp, "(");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, " %s ", node_kind_to_str(np->kind));
        compile_node(fp, np->u.link.n2);
        fprintf(fp, ")");
        */
        break;
    case NK_ADDR:
    case NK_INDIR:
    case NK_MINUS:
    case NK_NOT:
        /*TODO*/
        /*
        fprintf(fp, "(%s", node_kind_to_str(np->kind));
        compile_node(fp, np->u.link.n1);
        fprintf(fp, ")");
        */
        break;
    case NK_ID:
        assert(np->u.sym);
        if (np->u.sym->kind == SK_VAR) {
            if (np->u.sym->var_num != 0) {
                gen_lval(fp, np);
                fprintf(fp, "    pop rax\n");
                fprintf(fp, "    mov rax, [rax]\n");
                fprintf(fp, "    push rax\n");
            } else {
                /* TODO global var */
            }
        } else {
            fprintf(fp, ";FUNC %s\n", np->u.sym->id);
            /*TODO*/
        }
        break;
    case NK_INT_LIT:
        fprintf(fp, "    push %d\n", np->u.num);
        break;
    case NK_CALL:
        /*TODO*/
        /*
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "(");
        compile_node(fp, np->u.link.n2);
        fprintf(fp, ")");
        */
        break;
    case NK_ARG:
        /* TODO
        compile_node(fp, np->u.link.n1);
        if (np->u.link.n2) {
            fprintf(fp, ", ");
            compile_node(fp, np->u.link.n2);
        }
        */
        break;
    }

    return true;
}

bool compile_symbol(FILE *fp, const SYMBOL *sym)
{
    if (sym->kind == SK_FUNC && sym->has_body) {
        if (sym->sclass != SC_STATIC)
            fprintf(fp, ".global %s\n", sym->id);
        if (sym->sclass != SC_EXTERN)
            fprintf(fp, "%s:\n", sym->id);
        if (sym->has_body) {
            fprintf(fp, "    push rbp\n");
            fprintf(fp, "    mov rbp, rsp\n");
            if (sym->var_num > 0)
                fprintf(fp, "    sub rbp, %d\n", sym->var_num * 8);
            if (!compile_node(fp, sym->body_node))
                return false;
            fprintf(fp, "    mov rsp, rbp\n");
            fprintf(fp, "    pop rbp\n");
            fprintf(fp, "    ret\n");
        }
        fprintf(fp, "; -- %s\n", sym->id);
    }
    else if (sym->kind == SK_VAR) {
        fprintf(fp, "%s:\n    .zero 8\n", sym->id);
    }
    return true;
}

