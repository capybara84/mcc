int g;
int x;

int f() {
    int a, b;
    a = 123;
    b = 0;
    for (a = 0; a < 10; a = a + 1)
        b = b + a;
    return b;
}
