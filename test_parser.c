#include "mcc.h"

const char *source =
"int a;\n"
"void foo();\n"
"void bar() {}\n"
;


int main(void)
{
    PARSER *pars;
    NODE *np;

    set_verbose_level(3);
    init_symtab();
    pars = open_parser_text("test", source);
    if (pars == NULL)
        return 1;
    np = parse(pars);
    close_parser(pars);
    print_node(np);
    term_symtab();
    return 0;
}
