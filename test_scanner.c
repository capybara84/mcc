#include "mcc.h"

int scan_file(const char *filename)
{
    SCANNER *scan;
    TOKEN tk;

    scan = open_scanner(filename);
    if (scan == NULL) {
        printf("can't open '%s'\n", filename);
        return 1;
    }
    if (setjmp(g_error_jmp_buf) != 0) {
        close_scanner(scan);
        return 1;
    }
    while ((tk = next_token(scan)) != TK_EOF) {
        printf("%s(%d): %s\n", scan->pos.filename, scan->pos.line,
                token_to_string(tk));
    }
    close_scanner(scan);
    return 0;
}

static void show_help(void)
{
    printf("test_scanner\n");
    printf("usage: test_scanner [-h][-v N] filename\n");
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
            n += scan_file(argv[i]);
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
    int result;
    init_symtab();
    result = parse_command_line(argc, argv);
    term_symtab();
    return result;
}
