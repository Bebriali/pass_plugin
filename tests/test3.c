#include <stdio.h>

int add(int a, int b);
int mul(int a, int b);

int main() {
    printf("%d", add(5, mul(3, 1)));
    printf("%d", mul(5, 3));
    return 0;
}

int add(int a, int b) {
  return a + b;
}

int mul(int a, int b) {
  return a * b;
}