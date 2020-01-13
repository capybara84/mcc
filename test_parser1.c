/*
external_declaration
    = declaration_specifiers declarator ';'
    | declaration_specifiers declarator compound_statement

declaration_specifiers
    = declaration_specifier {declaration_specifier}

declaration_specifier
	= storage_class_specifier | type_specifier

storage_class_specifier
    = STATIC | EXTERN

type_specifier
	= VOID | INT

declarator
	= {'*'} (IDENTIFIER | '(' declarator ')') ['(' [parameter_list] ')']

abstract_declarator
    =  '*' {'*'}
    = {'*'} '(' abstract_declarator ')' ['(' [parameter_list] ')']

parameter_list
	= parameter_declaration {',' parameter_declaration}

parameter_declaration
	= declaration_specifiers declarator
    | declaration_specifiers [abstract_declarator]

*/
int a;
void foo();
int bar() {}
static int b;
extern void baz();
int *p;
int **pp;
void *ptr;
int (*pfn)();
int (**ppfn)();
static int **spp;

void f() {}
