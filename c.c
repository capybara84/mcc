int *test(int *p, int n)
{
    return p + n;
}

int main()
{
    int *p;
    p = 0;
    test(p, 10);
    return 0;
}
