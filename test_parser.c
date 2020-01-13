#include "mcc.h"

int compile_file(const char *filename)
{
    PARSER *pars;
    int result = 0;

    init_symtab();
    pars = open_parser(filename);
    if (pars == NULL) {
        printf("can't open '%s'\n", filename);
        return 1;
    }
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

static void show_help(void)
{
    printf("test_parser\n");
    printf("usage: test_parser [-h][-dX] filename\n");
    printf("option\n");
    printf("  -h       help\n");
    printf("  -dl  set scanner debug\n");
    printf("  -dp  set parser debug\n");
    printf("  -ds  set symbol debug\n");
}

static int parse_command_line(int argc, char *argv[])
{
    struct {
        char option;
        char *debug;
    } options[] = {
        { 'l', "scanner" },
        { 'p', "parser" },
        { 's', "symbol" },
    };
    const int N_OPTIONS = sizeof (options) / sizeof (options[1]);
    int i, j, n = 0;
    int n_file = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'd':
                for (j = 0; j < N_OPTIONS; j++) {
                    if (options[j].option == argv[i][2]) {
                        set_debug(options[j].debug);
                        goto next;
                    }
                }
                show_help();
                return 1;
            default:
                goto done;
            }
        } else {
            n += compile_file(argv[i]);
            n_file++;
        }
next: ;
    }
done:
    if (n_file == 0) {
        show_help();
        return 0;
    }
    return n;
}

int main(int argc, char *argv[])
{
    return parse_command_line(argc, argv);
}
