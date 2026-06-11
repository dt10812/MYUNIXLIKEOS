/* simple.c - Example program to demonstrate gcc compilation */
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    printf("Simple C program compiled inside MYUNIXLIKEOS!\n");
    printf("2 + 3 = %d\n", add(2, 3));
    return 0;
}
