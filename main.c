#include <string.h>
#include "mcc.h"

#define MAX_PATH    256

static void change_filename_ext(char *name, const char *orig, const char *ext)
{
    char *p;
    strcpy(name, orig);
    p = strchr(name, '.');
    if (p)
        strcpy(p, ext);
    else
        strcat(p, ext);
}

static int compile_file(const char *filename)
{
    char asm_name[MAX_PATH+1];
    PARSER *pars;
    int result;

    pars = open_parser(filename);
    if (pars == NULL) {
        fprintf(stderr, "can't open '%s'\n", filename);
        return 1;
    }
    if (setjmp(g_error_jmp_buf) != 0) {
        close_parser(pars);
        return 1;
    }
    result = parse(pars) ? 0 : 1;
    close_parser(pars);
    
    if (is_debug("symbol"))
        print_global_symtab();

    if (result == 0) {
        FILE *fp;
        change_filename_ext(asm_name, filename, ".s");
        fp = fopen(asm_name, "w");
        if (fp == NULL) {
            fprintf(stderr, "can't open '%s'\n", asm_name);
            return 1;
        }
        result = compile_all(fp) ? 0 : 1;
        fclose(fp);
    }
    return result;
}

static void show_help(void)
{
    printf("mcc - mini c compiler v" VERSION "\n");
    printf("usage: mcc [-h][-dX] filename...\n");
    printf("option\n");
    printf("  -h   help\n");
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
        return 1;
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
