#include "mcc.h"

const char *source =
"abc 123\n"
"static extern void int\n"
"if else while for continue break return \n"
", ; ( ) { } * / + - = || &&\n"
"== != < > <= >= & !";

TOKEN tokens[] =
    { TK_ID, TK_INT_LIT, TK_STATIC, TK_EXTERN, TK_VOID, TK_INT,
        TK_IF, TK_ELSE, TK_WHILE, TK_FOR, TK_CONTINUE, TK_BREAK, TK_RETURN,
        TK_COMMA, TK_SEMI, TK_LPAR, TK_RPAR, TK_BEGIN, TK_END,
        TK_STAR, TK_SLASH, TK_PLUS, TK_MINUS, TK_ASSIGN, TK_LOR, TK_LAND,
        TK_EQ, TK_NEQ, TK_LT, TK_GT, TK_LE, TK_GE, TK_AND, TK_NOT,
        TK_EOF };
#define N_TOKENS    (sizeof (tokens) / sizeof (TOKEN))

int main(void)
{
    SCANNER *scan;
    TOKEN tk;
    int i;
    int result = 0;

    init_symtab();
    scan = open_scanner_text("test", source);
    if (scan == NULL)
        return 1;
    if (setjmp(g_error_jmp_buf) != 0) {
        close_scanner(scan);
        term_symtab();
        return 1;
    }
    i = 0;
    while ((tk = next_token(scan)) != TK_EOF) {
/*
        printf("%s(%d): %s\n", scan->filename, scan->line,
            token_to_string(tk));
*/
        if (tokens[i] == tk) {
            printf("OK %d token %s\n", i, scan_token_to_string(scan, tk));
        } else {
            printf("FAIL %d token %s, expect %s\n", i,
                token_to_string(tk), token_to_string(tokens[i]));
            result++;
        }
        if (i < N_TOKENS)
            i++;
    }
    if (i+1 == N_TOKENS) {
        printf("DONE\n");
    } else {
        printf("FAIL size %d, expect %lu\n", i+1, N_TOKENS);
        result++;
    }
    close_scanner(scan);
    term_symtab();
    return result;
}
