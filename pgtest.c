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
 *
 * 8 doubles * 8 bytes = 64
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

    printf("Initial pgview() before any allocation requests:\n");
    pgview();
    printf("\n");

    nodes = pgalloc(sizeof(*nodes) * LEN);

    for (int i = 0; i < LEN; i++) {

        Node *n = newNode();
        nodes[i] = n;
    }

    printf("pgview() after allocating an array and nodes:\n");
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
    }

    printf("pgview() after freeing some nodes:\n");
    pgview();
    printf("\n");

    Node *m = newNode();
    printf("pgview() after allocating an additional node, should use previously freed block:\n");
    pgview();
    printf("\n");

    return 0;
}
