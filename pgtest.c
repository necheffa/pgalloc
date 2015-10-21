#include "pgalloc.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {

    char *str = pgalloc(30);
    char *str1 = "Hello World!\0";
    char *test = pgalloc(13);
    char *test1 = "123456789\0";
    char *sp = str;

    int *num[5];

    if (str == NULL) {
        printf("NULL\n");
        return 0;
    }

    while(*sp++ = *str1++);

    sp = test;
    while(*sp++ = *test1++);

    printf("[%s]\n", str);
    printf("[%s]\n", test);

    for (int i = 0; i < 5; i++) {
        num[i] = (int *) pgalloc(sizeof(int));
        *(num[i]) = rand();
    }


    pgview();

    pgfree(str);
    pgfree(test);

    pgfree(num[1]);
    pgfree(num[2]);

    pgview();

    return 0;
}
