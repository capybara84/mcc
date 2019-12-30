#include "mcc.h"

static int compile_file(const char *filename)
{
    PARSER *pars;
    NODE *np;

    pars = open_parser(filename);
    if (pars == NULL) {
        fprintf(stderr, "can't open '%s'\n", filename);
        return 1;
    }
    np = parse(pars);
    close_parser(pars);
    if (!compile_node(np))
        return 1;
    return 0;
}

static void show_help(void)
{
    printf("mcc - mini c compiler v" VERSION "\n");
    printf("usage: mcc [-h][-v N] filename...\n");
    printf("option\n");
    printf("  -h       help\n");
    printf("  -v N     set verbose level N\n");
}

static int parse_command_line(int argc, char *argv[])
{
    int i, n = 0;
    int n_file = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'v':
                if (argv[i][2] != 0)
                    set_verbose_level(atoi(&argv[i][2]));
                else if (++i < argc)
                    set_verbose_level(atoi(argv[i]));
                else
                    goto done;
                break;
            default:
                goto done;
            }
        } else {
            n += compile_file(argv[i]);
            n_file++;
        }
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
    int n;

    init_symtab();
    n = parse_command_line(argc, argv);
    term_symtab();

    return n;
}
