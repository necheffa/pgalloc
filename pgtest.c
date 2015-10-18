#include "pgalloc.h"
#include <stdio.h>

int main(void) {

    char *str = pgalloc(30);
    char *str1 = "Hello World!\0";
    char *test = pgalloc(13);
    char *test1 = "123456789\0";
    char *sp = str;

    if (str == NULL) {
        printf("NULL\n");
        return 0;
    }

    while(*sp++ = *str1++);

    sp = test;
    while(*sp++ = *test1++);

    printf("[%s]\n", str);
    printf("[%s]\n", test);


    pgview();

    pgfree(str);
    pgfree(test);

    pgview();

    return 0;
}
