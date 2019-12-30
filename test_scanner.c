#include "mcc.h"

const char *source =
"abc 123";

TOKEN tokens[] =
    { TK_ID, TK_INT_LIT, TK_ID,
        TK_EOF };
#define N_TOKENS    (sizeof (tokens) / sizeof (TOKEN))

int main(void)
{
    SCANNER *scan;
    TOKEN tk;
    int i;

    init_symtab();
    scan = open_scanner_text("test", source);
    if (scan == NULL)
        return 1;
    i = 0;
    while ((tk = next_token(scan)) != TK_EOF) {
        printf("%s(%d): %s\n", scan->filename, scan->line,
            token_to_string(tk));
        if (tokens[i] == tk) {
            printf("OK %d token %s\n", i, token_to_string(tk));
        } else {
            printf("FAIL %d token %s, expect %s\n", i,
                token_to_string(tk), token_to_string(tokens[i]));
        }
        if (i < N_TOKENS)
            i++;
    }
    if (i+1 == N_TOKENS) {
        printf("DONE\n");
    } else {
        printf("FAIL size %d, expect %lu\n", i+1, N_TOKENS);
    }
    close_scanner(scan);
    term_symtab();
    return 0;
}
