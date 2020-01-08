void bar();

void foo()
{
    int *p;
    int *q;

    p = bar;
    q = &bar;
}
