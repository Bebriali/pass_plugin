#include <stdio.h>

int functional_c(int *a, int n) {
    int res = 0;
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            res += a[i] * 2;
        } else {
            res -= (a[i] + 2);
        }
    }
    return res;
}

int main() {
    int array[] = {10, 20, 30, 40, 50};
    int n = 5;

    int res_c = functional_c(array, n);

    return 0;
}