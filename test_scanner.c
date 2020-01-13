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
    printf("usage: test_scanner [-h][-dX] filename\n");
    printf("option\n");
    printf("  -h   help\n");
    printf("  -dl  set scanner debug\n");
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
            case 'v':
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
            n += scan_file(argv[i]);
            n_file++;
        }
next: ;
    }
done:
    if (n_file == 0) {
        show_help();
        return 1;
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
