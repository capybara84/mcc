#include <assert.h>
#include <string.h>
#include "mcc.h"

static int s_label_number = 0;
static const char *s_arg_reg32[] = {
    "edi", "esi", "edx", "ecx", "r8d", "r9d",
};

int new_label(void)
{
    return s_label_number++;
}

void gen_header(FILE *fp)
{
    fprintf(fp, ".intel_syntax noprefix\n");
}

const char *var_addr(const SYMBOL *sym)
{
    static char buf[128];
    switch (sym->var_kind) {
    case VK_GLOBAL:
        sprintf(buf, "%s", sym->id);
        break;
    case VK_LOCAL:
        sprintf(buf, "[rbp-%d]", sym->offset + 8);
        break;
    case VK_PARAM:
        if (sym->num < 6)
            strcpy(buf, s_arg_reg32[sym->num]);
        else
            sprintf(buf, "[rbp+%d]", sym->offset + 16 - 6 * 4);
        break;
    default:
        buf[0] = '\0';
        break;
    }
    return buf;
}

void gen_get_var(FILE *fp, const NODE *np)
{
    SYMBOL *sym;
    if (np->kind != NK_ID)
        error(&np->pos, "not variable");
    sym = np->u.sym;
    assert(sym);
    if (sym->var_kind == VK_UNKNOWN)
        error(&np->pos, "invalid variable");
    fprintf(fp, "    mov eax,%s ; %s\n", var_addr(sym), sym->id);
}

void gen_set_var(FILE *fp, const NODE *np)
{
    SYMBOL *sym;
    if (np->kind != NK_ID)
        error(&np->pos, "not variable");
    sym = np->u.sym;
    assert(sym);
    if (sym->var_kind == VK_UNKNOWN)
        error(&np->pos, "invalid variable");
    fprintf(fp, "    mov %s,eax ; %s\n", var_addr(sym), sym->id);
}

bool compile_node(FILE *fp, const NODE *np)
{
    int label1, label2;

    if (np == NULL) {
        return true;
    }

    switch (np->kind) {
    case NK_LINK:
        compile_node(fp, np->u.comp.left);
        compile_node(fp, np->u.comp.right);
        break;
    case NK_COMPOUND:
        fprintf(fp, "; %s(%d)\n", np->pos.filename, np->pos.line);
        compile_node(fp, np->u.comp.left);
        break;
    case NK_IF:
        fprintf(fp, "; %s(%d) IF\n", np->pos.filename, np->pos.line);
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "    cmp rax, 0\n");
        label1 = new_label();
        fprintf(fp, "    je .L%d\n", label1);
        compile_node(fp, np->u.link.n2);
        label2 = new_label();
        fprintf(fp, "    jmp .L%d\n", label2);
        if (np->u.link.n3) {
            fprintf(fp, ".L%d:\n", label1);
            compile_node(fp, np->u.link.n3);
        }
        fprintf(fp, ".L%d:\n", label2);
        break;
    case NK_WHILE:
        fprintf(fp, "; %s(%d) WHILE\n", np->pos.filename, np->pos.line);
        label1 = new_label();
        fprintf(fp, ".L%d:\n", label1);
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "    cmp rax, 0\n");
        label2 = new_label();
        fprintf(fp, "    je .L%d\n", label2);
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "    jmp .L%d\n", label1);
        fprintf(fp, ".L%d:\n", label2);
        break;
    case NK_FOR:
        fprintf(fp, "; %s(%d) FOR\n", np->pos.filename, np->pos.line);
        compile_node(fp, np->u.link.n1);
        label1 = new_label();
        fprintf(fp, ".L%d:\n", label1);
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "    cmp rax, 0\n");
        label2 = new_label();
        fprintf(fp, "    je .L%d\n", label2);
        compile_node(fp, np->u.link.n4);
        compile_node(fp, np->u.link.n3);
        fprintf(fp, "    jmp .L%d\n", label1);
        fprintf(fp, ".L%d:\n", label2);
        break;
    case NK_CONTINUE:
        fprintf(fp, "; %s(%d) CONTINUE\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_BREAK:
        fprintf(fp, "; %s(%d) BREAK\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_RETURN:
        fprintf(fp, "; %s(%d) RETURN\n", np->pos.filename, np->pos.line);
        if (np->u.link.n1) {
            compile_node(fp, np->u.link.n1);
            fprintf(fp, "    mov rsp, rbp\n");
            fprintf(fp, "    pop rbp\n");
            fprintf(fp, "    ret\n");
        }
        break;
    case NK_EXPR:
        fprintf(fp, "; %s(%d) EXPR ", np->pos.filename, np->pos.line);
        fprint_node(fp, 0, np);
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
        compile_node(fp, np->u.link.n2);
        fprintf(fp, "    push rax\n");
        compile_node(fp, np->u.link.n1);
        fprintf(fp, "    pop rdi\n");
        switch (np->kind) {
        case NK_ADD:
            /*TODO bit */
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
        break;
    case NK_ASSIGN:
        compile_node(fp, np->u.link.n2);
        gen_set_var(fp, np->u.link.n1);
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
            gen_get_var(fp, np);
        } else {
            fprintf(fp, ";FUNC %s\n", np->u.sym->id);
            /*TODO*/
            assert(0);
        }
        break;
    case NK_INT_LIT:
        fprintf(fp, "    mov eax,%d\n", np->u.num);
        break;
    case NK_CALL:
        compile_node(fp, np->u.link.n2);
        if (np->u.link.n1->kind == NK_ID) {
            fprintf(fp, "    call %s\n", np->u.link.n1->u.sym->id);
        } else {
            /*TODO*/
        }
        break;
    case NK_ARG:
        compile_node(fp, np->u.arg.right);
        compile_node(fp, np->u.arg.left);
        if (np->u.arg.num > 5) {
            /*TODO bit */
            fprintf(fp, "    push eax\n");
        } else {
            /*TODO bit */
            fprintf(fp, "    mov %s,eax\n", s_arg_reg32[np->u.arg.num]);
        }
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
            int buf_size;
            fprintf(fp, "    push rbp\n");
            fprintf(fp, "    mov rbp, rsp\n");
            buf_size = calc_arg_size(sym) * 4;
            fprintf(fp, "    sub rbp, %d\n", sym->offset + buf_size + 8);
            if (buf_size > 0) {
                int i;
                for (i = 0; i < 6; i++) /*TODO*/
                    fprintf(fp, "    mov [rbp-%d],%s\n",
                                        (i+1)*4, s_arg_reg32[i]); /*TODO bit*/
            }
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

