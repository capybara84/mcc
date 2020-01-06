#include "mcc.h"

int parse_test(void)
{
    PARSER *pars;
    int result = 0;

    init_symtab();
    pars = open_parser("test_parser1.c");
    if (pars == NULL)
        return 1;
    if (setjmp(g_error_jmp_buf) != 0) {
        close_parser(pars);
        result++;
    } else {
        parse(pars);
        close_parser(pars);
    }
    print_global_symtab();
    term_symtab();
    return result;
}

int main(void)
{
/*
    set_verbose_level(3);
*/
    return parse_test();
}
