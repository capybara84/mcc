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
    "int a;\n"
    "void foo();\n"
    "int b;",
    "int foo() {\n"
    " int a,b;\n"
    " a = 123;\n"
    " if (a != 0) {} else {}\n"
    " while (a == 1)\n"
    "  a = a - 1;\n"
    "}\n"
};
#define N_TESTS (sizeof (tests) / sizeof (tests[0]))

int parse_test(void)
{
    int i;
    PARSER *pars;
    NODE *np;
    int result = 0;

    init_symtab();
    for (i = 0; i < N_TESTS; i++) {
        printf("-----------------\n");
        printf("test %d: %s\n", i, tests[i]);
        pars = open_parser_text("text", tests[i]);
        if (pars == NULL)
            return 1;
        if (setjmp(g_error_jmp_buf) != 0) {
            close_parser(pars);
            result++;
        } else {
            np = parse(pars);
            close_parser(pars);
            printf("NODE:");
            print_node(np);
        }
    }
    term_symtab();
    printf("result = %d\n", result);
    return result;
}

int main(void)
{
/*
    set_verbose_level(3);
*/
    return parse_test();
}
