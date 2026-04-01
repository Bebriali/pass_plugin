#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[]) {
    int n = 0;
    int* array = NULL;
    if (argc == 1) {
        n = 5;
        array = (int*) calloc(n, sizeof(int));
        for (int i = 0; i < n; i++) {
            array[i] = 10 * i;
        }

        for (size_t i = 0; i < n; i++) {
            array[i] = 10 * i;
        }
    }
    else if (strcmp(argv[1], "--my-data") == 0) {
        scanf("%d", &n);
        array = (int*) calloc(n, sizeof(int));
        for (size_t i = 0; i < n; i++)
        {
            scanf("%d", &array[i]);
        }
    }
    else if (strcmp(argv[1], "--help") == 0) {
        printf("--my-data for typing your own array in format: \n\t");
        printf("<n> - number of array elements\n\t");
        printf("<array> - with a space between numbers\n");
    }
    else {
        printf("print --help to learn your capabilities\n");
    }

    int res_c = functional_c(array, n);

    return 0;
}