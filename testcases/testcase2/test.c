//
// Created by prophe cheng on 2023/9/22.
//
#include <stddef.h>

typedef void (*functype)(int);
typedef int (*functype2)(long);

static void f1(int x) { x = 1; }
static void f2(int x) { x = 2; }
static int g1(long x) { return x; }
static int g2(long x) { return x + 1; }

functype get_one(char c) {
    if (c == '1')
        return f1;
    if (c == '2')
        return &f2;
    if (c == '3')
        return (void (*)(int)) g1;
    if (c == '4')
        return (void (*)(int)) &g2;
    return NULL;
}

int main(int argc, const char* argv[]) {
    void (*f)(int);
    f = get_one(argv[1][0]);
    if (f)
        f(1);
    return 0;
}