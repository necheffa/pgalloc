#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pgalloc.h"

#define LEN 96

// 20151024 - confirmed working with stdlib.h malloc() and free()

typedef struct Node Node;

/* use doubles because IEEE 754 guarantees a 64 bit size
 * this will make my calculations a little easier
 * as I will only have to worry about 32 bit vs 64 bit word size
 * in my PageHeader, not the allocated blocks themselves
 */
struct Node {
    double a;
    double b;
    double c;
    double d;
    double e;
    double f;
    double g;
};

static Node **nodes;

static Node *newNode(void) {

    Node *n = pgalloc(sizeof(*n));

    return n;
}

int main(void) {

    printf("first\n");
    pgview();
    printf("\n");

    nodes = pgalloc(sizeof(*nodes) * LEN);

    for (int i = 0; i < LEN; i++) {

        Node *n = newNode();
        nodes[i] = n;

        //pgview();
        //printf("\n");

    }

    printf("after alloc loop\n");
    pgview();
    printf("\n");

    for (int i = 0; i < (LEN / 2); i++) {

        Node *n = nodes[i];

        if (n == NULL) {
            printf("Node should not be NULL!\n");
            fflush(stdout);
            assert(n);
        }

        nodes[i] = NULL;
        pgfree(n);
        //pgview();
        //printf("\n");
    }

    printf("after free loop\n");
    pgview();
    printf("\n");

    Node *m = newNode();
    printf("after loose alloc\n");
    pgview();
    printf("\n");

    return 0;
}
