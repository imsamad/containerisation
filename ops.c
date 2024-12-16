#include <stdio.h>

int ops(int (*op)(int, int), int a, int b) {
    return op(a,b);
}

int add(int a, int b) {
    return a + b;
}

int main() {
    int res = ops(add, 10, 20);

    printf("Res: %d",res);
    return 0;
}
