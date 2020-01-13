#include <string.h>
#include "mcc.h"

jmp_buf g_error_jmp_buf;

static struct debug {
    struct debug *next;
    const char *name;
} *s_debug = NULL;

bool is_debug(const char *s)
{
    struct debug *d;
    for (d = s_debug; d != NULL; d = d->next)
        if (strcmp(d->name, s) == 0)
            return true;
    return false;
}

void set_debug(const char *s)
{
    if (!is_debug(s)) {
        struct debug *d = (struct debug*) alloc(sizeof (struct debug));
        d->name = s;
        d->next = s_debug;
        s_debug = d;
    }
}


void *alloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "out of memory\n");
        abort();
    }
    return p;
}

void vwarning(const POS *pos, const char *s, va_list ap)
{
    fprintf(stdout, "%s(%d):warning:", pos->filename, pos->line);
    vfprintf(stdout, s, ap);
    fprintf(stdout, "\n");
}

void verror(const POS *pos, const char *s, va_list ap)
{
    fprintf(stdout, "%s(%d):error:", pos->filename, pos->line);
    vfprintf(stdout, s, ap);
    fprintf(stdout, "\n");
    longjmp(g_error_jmp_buf, 1);
}

void error(const POS *pos, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(pos, s, ap);
    va_end(ap);
}

