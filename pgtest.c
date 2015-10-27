#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pgalloc.h"

#define LEN 64

// 20151024 - confirmed working with stdlib.h malloc() and free()

typedef struct Node Node;

struct Node {
    long a;
    long b;
    long c;
    long d;
    long e;
    long f;
    long g;
};

static Node **nodes;

static Node *newNode(void) {

    Node *n = pgalloc(sizeof(*n));

    return n;
}

int main(void) {

    pgview();

    nodes = pgalloc(sizeof(*nodes) * LEN);

    for (int i = 0; i < LEN; i++) {

        Node *n = newNode();
        nodes[i] = n;

        pgview();
        //printf("alloc [%d]\n", i);

    }

    pgview();

    for (int i = 0; i < (LEN / 2); i++) {

        Node *n = nodes[i];

        if (n == NULL) {
            printf("Node should not be NULL!\n");
            fflush(stdout);
            assert(n);
        }

        nodes[i] = NULL;
        pgfree(n);
        pgview();
    }

    pgview();

    Node *m = newNode();
    pgview();

    return 0;
}
