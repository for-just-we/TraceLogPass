//
// Created by prophe cheng on 2023/9/21.
//
#include <stdio.h>

void func() {
    FILE* file = fopen("icall_log.txt", "a");
    fprintf(file, "F:%s:%p:%ld\n", "func", func, 1);
    fclose(file);
}

int main() {
    void (*f)() = func;
    FILE* file = fopen("icall_log.txt", "a");
    fprintf(file, "F:%s:%p:%ld\n", "func", f, 1);
    fclose(file);
    f();
    return 0;
}