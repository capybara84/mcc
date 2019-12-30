#ifndef mcc_h__
#define mcc_h__

#define VERSION "0.0"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef int bool;
#define true    1
#define false   0

bool is_verbose_level(int n);
void set_verbose_level(int n);
void *alloc(size_t size);

void verror(const char *filename, int line, const char *s, va_list arg);
void error(const char *filename, int line, const char *s, ...);

bool init_symtab(void);
void term_symtab(void);

#endif
