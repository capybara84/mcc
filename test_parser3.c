void bar();

void foo()
{
    void (*pf)();
    int *p;
    int *q;

    p = q;
    p = bar;
    q = &bar;

    pf = bar;
    pf = &bar;

    pf < bar;

    p = 0;
    p = 1;
}
