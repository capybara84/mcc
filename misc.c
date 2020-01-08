#include "mcc.h"

jmp_buf g_error_jmp_buf;

static int s_verbose_level = 0;

bool is_verbose_level(int n)
{
    return s_verbose_level >= n;
}

void set_verbose_level(int n)
{
    s_verbose_level = n;
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

void vwarning(const char *filename, int line, const char *s, va_list ap)
{
    fprintf(stdout, "%s(%d):warning:", filename, line);
    vfprintf(stdout, s, ap);
    fprintf(stdout, "\n");
}

void verror(const char *filename, int line, const char *s, va_list ap)
{
    fprintf(stdout, "%s(%d):error:", filename, line);
    vfprintf(stdout, s, ap);
    fprintf(stdout, "\n");
    longjmp(g_error_jmp_buf, 1);
}

void error(const char *filename, int line, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(filename, line, s, ap);
    va_end(ap);
}

