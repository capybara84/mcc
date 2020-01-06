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
int *p;

int fun() {
    int a, b;
    a = 123;
    if (a != 0)
        b = 1;
    else
        b = 2;
    while (a == 1)
        a = a - 1;
}

int f() {
    int a, b;
    a = 123;
    b = 0;
    for (a = 0; a < 10; a = a + 1)
        b = b + a;
    return b;
}

