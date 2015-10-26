#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pgalloc.h"

#define LEN 64

// 20151024 - confirmed working with stdlib.h malloc() and free()

typedef struct Node Node;

struct Node {
    int x;
    int y;
};

static Node **nodes;

static Node *newNode(int x, int y) {

    Node *n = pgalloc(sizeof(*n));

    n->x = x;
    n->y = y;

    return n;
}

int main(void) {

    pgview();

    nodes = pgalloc(sizeof(*nodes) * LEN);

    for (int i = 0; i < LEN; i++) {

        Node *n = newNode(i, (i+1));
        nodes[i] = n;

        printf("alloc [%d]\n", i);

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

    Node *m = newNode(5,5);
    pgview();

    return 0;
}
