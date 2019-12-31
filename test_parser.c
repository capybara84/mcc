#include "mcc.h"

const char *tests[] =
{
    "int a;",
    "void foo();",
    "int bar() {}",
    "static int b;",
    "extern void baz();",
    "int *p;",
    "int **pp;",
    "void *ptr;",
    "int (*pfn)();",
    "int (**ppfn)();",
    "static int **p;",
    "int (*p)();",
    "int **(**p)();",
    "int (*p);",
};
#define N_TESTS (sizeof (tests) / sizeof (tests[0]))

int main(void)
{
    int i;
    PARSER *pars;
    NODE *np;

    set_verbose_level(3);
    init_symtab();
    for (i = 0; i < N_TESTS; i++) {
        printf("-----------------\n");
        printf("test %d: %s\n", i, tests[i]);
        pars = open_parser_text("text", tests[i]);
        if (pars == NULL)
            return 1;
        np = parse(pars);
        close_parser(pars);
        print_node(np);
    }
    term_symtab();
    return 0;
}
